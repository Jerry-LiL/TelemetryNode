#include "app.h"
#include "board.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char **g_argv;

static void load_dotenv(const char *path)
{
    FILE *file = fopen(path, "r");
    char line[256];

    if (file == NULL) {
        return;
    }
    while (fgets(line, (int)sizeof line, file) != NULL) {
        char *start = line;
        char *eq;
        char *end;

        while ((*start == ' ') || (*start == '\t')) {
            start++;
        }
        if ((*start == '#') || (*start == '\n') || (*start == '\0')) {
            continue;
        }
        eq = strchr(start, '=');
        if (eq == NULL) {
            continue;
        }
        *eq = '\0';
        end = eq + 1;
        while ((*end != '\0') && (*end != '\n') && (*end != '\r')) {
            end++;
        }
        *end = '\0';
        if (getenv(start) == NULL) {
            setenv(start, eq + 1, 0);
        }
    }
    fclose(file);
}

static int env_u32(const char *name, int fallback)
{
    const char *value = getenv(name);
    if ((value == NULL) || (value[0] == '\0')) {
        return fallback;
    }
    return atoi(value);
}

static void iwdg_reset(void)
{
    int count = env_u32("TN_BOOT_COUNT", 1) + 1;
    char buf[16];
    char exe[512];
    ssize_t n;

    snprintf(buf, sizeof buf, "%d", count);
    setenv("TN_BOOT_COUNT", buf, 1);
    setenv("TN_BOOT_REASON", "iwdg", 1);
    printf("IWDG_RESET count=%d\n", count);
    fflush(stdout);

    n = readlink("/proc/self/exe", exe, sizeof exe - 1u);
    if (n > 0) {
        exe[n] = '\0';
        execv(exe, g_argv);
    }
    execv(g_argv[0], g_argv);
    perror("execv");
    _exit(1);
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [--link PATH|tcp://ADDR:PORT] [--baud N] [--node ID]\n",
            argv0);
}

int main(int argc, char **argv)
{
    const char *link;
    int baud;
    int node;
    int boot;
    const char *reason;
    int i;

    g_argv = argv;
    load_dotenv(".env");
    link = getenv("TN_UART_LINK");
    baud = env_u32("TN_UART_BAUD", 115200);
    node = env_u32("TN_NODE_ID", 1);
    boot = env_u32("TN_BOOT_COUNT", 1);
    reason = getenv("TN_BOOT_REASON");
    if (link == NULL) {
        link = "/tmp/telemetry-node-uart";
    }
    if ((reason == NULL) || (reason[0] == '\0')) {
        reason = "power";
    }

    for (i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--link") == 0) && (i + 1 < argc)) {
            link = argv[++i];
        } else if ((strcmp(argv[i], "--baud") == 0) && (i + 1 < argc)) {
            baud = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "--node") == 0) && (i + 1 < argc)) {
            node = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("BOOT reason=%s count=%d node=%d\n", reason, boot, node);
    fflush(stdout);

    app_set_boot((uint16_t)boot, (uint8_t)node, reason);
    app_init();
    if (board_uart_open(link, (uint32_t)baud) != 0) {
        fprintf(stderr, "failed to open UART endpoint %s\n", link);
        return 1;
    }
    board_on_iwdg_reset(iwdg_reset);
    board_iwdg_start(1500u);
    app_create_tasks();
    board_iwdg_refresh();
    vTaskStartScheduler();
    return 0;
}
