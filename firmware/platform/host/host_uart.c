#define _GNU_SOURCE

#include "host_uart.h"

#include "uart_link.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#include <stdatomic.h>

static int s_fd = -1;
static int s_listen = -1;
static int s_slave = -1;
static atomic_int s_tx_armed;
static pthread_t s_thread;

static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int open_pty(const char *link_path)
{
    char *slave_name;
    struct termios term;
    int master;

    master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        return -1;
    }
    if ((grantpt(master) != 0) || (unlockpt(master) != 0)) {
        close(master);
        return -1;
    }
    slave_name = ptsname(master);
    if (slave_name == NULL) {
        close(master);
        return -1;
    }

    s_slave = open(slave_name, O_RDWR | O_NOCTTY);
    if (s_slave < 0) {
        close(master);
        return -1;
    }
    if (tcgetattr(s_slave, &term) == 0) {
        cfmakeraw(&term);
        (void)tcsetattr(s_slave, TCSANOW, &term);
    }

    unlink(link_path);
    if (symlink(slave_name, link_path) != 0) {
        close(s_slave);
        close(master);
        s_slave = -1;
        return -1;
    }
    if (set_nonblock(master) != 0) {
        close(s_slave);
        close(master);
        s_slave = -1;
        return -1;
    }

    printf("UART_PTY=%s\n", slave_name);
    printf("UART_LINK=%s\n", link_path);
    fflush(stdout);
    return master;
}

static int open_tcp(const char *spec)
{
    const char *colon;
    char host[64];
    int port;
    struct sockaddr_in addr;
    int yes = 1;

    /* spec is "tcp://ADDR:PORT". */
    if (strncmp(spec, "tcp://", 6) != 0) {
        return -1;
    }
    colon = strrchr(spec + 6, ':');
    if ((colon == NULL) || (colon == spec + 6)) {
        return -1;
    }
    if ((size_t)(colon - (spec + 6)) >= sizeof host) {
        return -1;
    }
    memcpy(host, spec + 6, (size_t)(colon - (spec + 6)));
    host[colon - (spec + 6)] = '\0';
    port = atoi(colon + 1);
    if ((port <= 0) || (port > 65535)) {
        return -1;
    }

    s_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen < 0) {
        return -1;
    }
    (void)setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (strcmp(host, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(s_listen);
        s_listen = -1;
        return -1;
    }
    if ((bind(s_listen, (struct sockaddr *)&addr, sizeof addr) != 0) || (listen(s_listen, 1) != 0)) {
        close(s_listen);
        s_listen = -1;
        return -1;
    }
    if (set_nonblock(s_listen) != 0) {
        close(s_listen);
        s_listen = -1;
        return -1;
    }

    printf("UART_LINK=%s\n", spec);
    fflush(stdout);
    return -2; /* listening; the thread accepts into s_fd */
}

static void close_client(void)
{
    if (s_fd >= 0) {
        close(s_fd);
        s_fd = -1;
    }
}

static void accept_client(void)
{
    struct pollfd pfd;
    int client;

    if (s_listen < 0) {
        return;
    }
    pfd.fd = s_listen;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 20) <= 0) {
        return;
    }
    client = accept(s_listen, NULL, NULL);
    if (client < 0) {
        return;
    }
    if (set_nonblock(client) != 0) {
        close(client);
        return;
    }
    s_fd = client;
}

static void drain_tx(void)
{
    int guard = 128;

    while (guard-- > 0) {
        uint8_t byte;
        ssize_t wrote;

        if (uart_link_isr_pop_tx(&byte) != 0) {
            atomic_store(&s_tx_armed, 0);
            /* A producer may have queued a byte and armed us between the empty pop and the clear. */
            if (uart_link_isr_pop_tx(&byte) != 0) {
                return;
            }
            atomic_store(&s_tx_armed, 1);
        }
        wrote = write(s_fd, &byte, 1);
        if (wrote == 1) {
            continue;
        }
        if ((wrote < 0) && (errno == EAGAIN || errno == EINTR)) {
            /* The byte is already out of the FIFO. Keep it locally by pushing it back via a blocking retry. */
            for (;;) {
                wrote = write(s_fd, &byte, 1);
                if (wrote == 1) {
                    break;
                }
                if ((wrote < 0) && (errno == EINTR)) {
                    continue;
                }
                if ((wrote < 0) && (errno == EAGAIN)) {
                    struct pollfd pfd;
                    pfd.fd = s_fd;
                    pfd.events = POLLOUT;
                    (void)poll(&pfd, 1, 5);
                    continue;
                }
                close_client();
                return;
            }
            continue;
        }
        close_client();
        return;
    }
}

static void *uart_thread(void *arg)
{
    (void)arg;

    for (;;) {
        struct pollfd pfd;
        uint8_t buf[64];
        ssize_t got;
        ssize_t i;

        if (s_fd < 0) {
            if (s_listen >= 0) {
                accept_client();
            } else {
                usleep(2000);
            }
            continue;
        }

        pfd.fd = s_fd;
        pfd.events = POLLIN;
        if (atomic_load(&s_tx_armed) != 0) {
            pfd.events = (short)(pfd.events | POLLOUT);
        }
        if (poll(&pfd, 1, 2) < 0) {
            if (errno == EINTR) {
                continue;
            }
            close_client();
            continue;
        }
        if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            close_client();
            continue;
        }
        if ((pfd.revents & POLLIN) != 0) {
            got = read(s_fd, buf, sizeof buf);
            if (got > 0) {
                for (i = 0; i < got; i++) {
                    uart_link_isr_rx(buf[i]);
                }
            } else if ((got == 0) || (errno != EAGAIN && errno != EINTR)) {
                close_client();
                continue;
            }
        }
        if ((s_fd >= 0) && (atomic_load(&s_tx_armed) != 0)) {
            drain_tx();
        }
    }
    return NULL;
}

void host_uart_arm_tx(void)
{
    atomic_store(&s_tx_armed, 1);
}

int host_uart_open(const char *endpoint, uint32_t baud)
{
    int fd;

    (void)baud;
    if (endpoint == NULL) {
        return -1;
    }

    if (strncmp(endpoint, "tcp://", 6) == 0) {
        fd = open_tcp(endpoint);
        if (fd == -1) {
            return -1;
        }
    } else {
        fd = open_pty(endpoint);
        if (fd < 0) {
            return -1;
        }
        s_fd = fd;
    }

    if (pthread_create(&s_thread, NULL, uart_thread, NULL) != 0) {
        return -1;
    }
    return 0;
}
