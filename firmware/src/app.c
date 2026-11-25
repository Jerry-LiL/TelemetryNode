#include "app.h"

#include "bme280.h"
#include "board.h"
#include "can_codec.h"
#include "can_sim.h"
#include "frame.h"
#include "icm42688.h"
#include "payload.h"
#include "uart_link.h"
#include "wdg.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_WORDS      512
#define ENV_PERIOD_MS    200u
#define IMU_PERIOD_MS    20u
#define WDG_PERIOD_MS    20u
#define WDG_TIMEOUT_MS   400u
#define Q_ENV            8u
#define Q_IMU            16u
#define Q_EVT            8u

#define PRIO_TELEM       2
#define PRIO_CMD         3
#define PRIO_SENSOR      4
#define PRIO_SUPERVISOR  5

#define EVT_WDG     1u
#define EVT_LOG     2u
#define EVT_HEALTH  3u

#define LOG_INFO  1u
#define LOG_WARN  2u

typedef struct {
    uint8_t kind;
    uint8_t task_id;
    uint8_t severity;
    uint8_t code;
    uint16_t count;
    uint32_t value;
    char text[40];
} app_event_t;

static QueueHandle_t s_env_q;
static QueueHandle_t s_imu_q;
static QueueHandle_t s_evt_q;
static QueueSetHandle_t s_set;
static TaskHandle_t s_task[TN_TASK_COUNT];
static TaskHandle_t s_cmd_task;

static atomic_uint s_last_kick[TN_TASK_COUNT];
static atomic_uint s_stall_until[TN_TASK_COUNT];
static atomic_uint s_drops;
static atomic_uint s_glitch;
static atomic_uint s_recoveries;

static uint16_t s_seq;
static uint16_t s_boot_count = 1u;
static uint8_t s_node_id = 1u;
static char s_boot_reason[16] = "power";

static void delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static int stalled(tn_task_id id)
{
    uint32_t until = atomic_load(&s_stall_until[id]);
    if ((until == 0u) || (until == 0xFFFFFFFFu)) {
        return until == 0xFFFFFFFFu;
    }
    return (int32_t)(until - board_millis()) > 0;
}

static void kick(tn_task_id id)
{
    atomic_store(&s_last_kick[id], board_millis());
}

static int post_event(const app_event_t *event)
{
    if (xQueueSend(s_evt_q, event, 0) != pdTRUE) {
        atomic_fetch_add(&s_drops, 1u);
        return -1;
    }
    return 0;
}

static void post_log(uint8_t severity, uint8_t code, const char *text)
{
    app_event_t event;
    memset(&event, 0, sizeof event);
    event.kind = EVT_LOG;
    event.severity = severity;
    event.code = code;
    if (text != NULL) {
        snprintf(event.text, sizeof event.text, "%s", text);
    }
    (void)post_event(&event);
}

static void env_task(void *arg)
{
    bme280_t dev;
    int ready = 0;
    (void)arg;

    for (;;) {
        env_sample_t sample;

        if (stalled(TN_TASK_ENV) != 0) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }
        if (ready == 0) {
            ready = (bme280_init(&dev, BME280_I2C_ADDR_SDO_LOW, delay_ms) == 0);
            if (ready == 0) {
                post_log(LOG_WARN, 5, "bme280 init failed");
                kick(TN_TASK_ENV);
                vTaskDelay(pdMS_TO_TICKS(ENV_PERIOD_MS));
                continue;
            }
        }
        if (bme280_read(&dev, &sample) == 0) {
            sample.time_ms = board_millis();
            if (xQueueSend(s_env_q, &sample, 0) != pdTRUE) {
                atomic_fetch_add(&s_drops, 1u);
            }
        }
        kick(TN_TASK_ENV);
        vTaskDelay(pdMS_TO_TICKS(ENV_PERIOD_MS));
    }
}

static void imu_task(void *arg)
{
    icm42688_t dev;
    int ready = 0;
    (void)arg;

    for (;;) {
        imu_sample_t sample;

        if (stalled(TN_TASK_IMU) != 0) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }
        if (ready == 0) {
            ready = (icm42688_init(&dev, delay_ms) == 0);
            if (ready == 0) {
                post_log(LOG_WARN, 6, "icm42688 init failed");
                kick(TN_TASK_IMU);
                vTaskDelay(pdMS_TO_TICKS(IMU_PERIOD_MS));
                continue;
            }
        }
        if (icm42688_read(&dev, &sample) == 0) {
            sample.time_ms = board_millis();
            if (xQueueSend(s_imu_q, &sample, 0) != pdTRUE) {
                atomic_fetch_add(&s_drops, 1u);
            }
        }
        kick(TN_TASK_IMU);
        vTaskDelay(pdMS_TO_TICKS(IMU_PERIOD_MS));
    }
}

static int send_frame(uint8_t type, const uint8_t *payload, uint8_t len)
{
    frame_t frame;
    uint8_t wire[FRAME_MAX_PAYLOAD + 9u];
    int corrupt;
    int n;

    memset(&frame, 0, sizeof frame);
    frame.type = type;
    frame.seq = s_seq++;
    frame.len = len;
    if ((payload != NULL) && (len > 0u)) {
        memcpy(frame.payload, payload, len);
    }
    corrupt = (atomic_exchange(&s_glitch, 0u) != 0u) && (len > 0u);
    n = frame_encode(wire, sizeof wire, &frame, corrupt);
    if (n < 0) {
        return -1;
    }
    return uart_link_send(wire, (size_t)n);
}

static void mirror_can(const can_frame_t *can)
{
    can_frame_view_t view;
    uint8_t payload[PAYLOAD_CAN_LEN];

    view.id = can->id;
    view.dlc = can->dlc;
    memcpy(view.data, can->data, sizeof view.data);
    if (payload_pack_can(payload, sizeof payload, &view) == PAYLOAD_CAN_LEN) {
        (void)send_frame(FRAME_CAN, payload, PAYLOAD_CAN_LEN);
    }
}

static void publish_can(const can_frame_t *can)
{
    if (can_sim_transmit(can) == 0) {
        mirror_can(can);
    }
}

static void handle_env(const env_sample_t *sample)
{
    uint8_t payload[PAYLOAD_ENV_LEN];
    can_frame_t can;

    if (payload_pack_env(payload, sizeof payload, sample) != PAYLOAD_ENV_LEN) {
        return;
    }
    (void)send_frame(FRAME_ENV, payload, PAYLOAD_ENV_LEN);
    if (can_pack_env(sample, &can) == 0) {
        publish_can(&can);
    }
}

static void handle_imu(const imu_sample_t *sample)
{
    uint8_t payload[PAYLOAD_IMU_LEN];
    can_frame_t can[2];

    if (payload_pack_imu(payload, sizeof payload, sample) != PAYLOAD_IMU_LEN) {
        return;
    }
    (void)send_frame(FRAME_IMU, payload, PAYLOAD_IMU_LEN);
    if (can_pack_imu(sample, can) == 2) {
        publish_can(&can[0]);
        publish_can(&can[1]);
    }
}

static void handle_event(const app_event_t *event)
{
    if (event->kind == EVT_WDG) {
        wdg_event_t wdg;
        uint8_t payload[PAYLOAD_WDG_LEN];
        wdg.task_id = event->task_id;
        wdg.cause = WDG_CAUSE_STALL;
        wdg.recovery_count = event->count;
        wdg.stale_ms = event->value;
        if (payload_pack_wdg(payload, sizeof payload, &wdg) == PAYLOAD_WDG_LEN) {
            (void)send_frame(FRAME_WDG, payload, PAYLOAD_WDG_LEN);
        }
    } else if (event->kind == EVT_LOG) {
        uint8_t payload[FRAME_MAX_PAYLOAD];
        size_t text_len = strlen(event->text);
        if (text_len > (sizeof payload - 2u)) {
            text_len = sizeof payload - 2u;
        }
        payload[0] = event->severity;
        payload[1] = event->code;
        memcpy(payload + 2, event->text, text_len);
        (void)send_frame(FRAME_LOG, payload, (uint8_t)(text_len + 2u));
    } else if (event->kind == EVT_HEALTH) {
        can_frame_t can;
        if (can_pack_health(s_boot_count, event->count, event->value, &can) == 0) {
            publish_can(&can);
        }
    }
}

static void telem_task(void *arg)
{
    int announced = 0;
    (void)arg;

    for (;;) {
        QueueSetMemberHandle_t member;
        env_sample_t env;
        imu_sample_t imu;
        app_event_t event;

        if (stalled(TN_TASK_TELEM) != 0) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        if (announced == 0) {
            char text[40];
            snprintf(text, sizeof text, "boot %s node %u", s_boot_reason, (unsigned)s_node_id);
            {
                app_event_t boot;
                memset(&boot, 0, sizeof boot);
                boot.kind = EVT_LOG;
                boot.severity = LOG_INFO;
                boot.code = 1;
                snprintf(boot.text, sizeof boot.text, "%s", text);
                handle_event(&boot);
            }
            announced = 1;
        }

        member = xQueueSelectFromSet(s_set, pdMS_TO_TICKS(20));
        if (member == s_env_q) {
            if (xQueueReceive(s_env_q, &env, 0) == pdTRUE) {
                handle_env(&env);
            }
        } else if (member == s_imu_q) {
            if (xQueueReceive(s_imu_q, &imu, 0) == pdTRUE) {
                handle_imu(&imu);
            }
        } else if (member == s_evt_q) {
            if (xQueueReceive(s_evt_q, &event, 0) == pdTRUE) {
                handle_event(&event);
            }
        }
        kick(TN_TASK_TELEM);
    }
}

static int task_from_name(const char *name, tn_task_id *id)
{
    if (strcmp(name, "env") == 0) {
        *id = TN_TASK_ENV;
    } else if (strcmp(name, "imu") == 0) {
        *id = TN_TASK_IMU;
    } else if (strcmp(name, "telem") == 0) {
        *id = TN_TASK_TELEM;
    } else if (strcmp(name, "supervisor") == 0) {
        *id = TN_TASK_SUPERVISOR;
    } else {
        return -1;
    }
    return 0;
}

static void handle_line(char *line)
{
    char name[16];
    unsigned ms = 0;
    int fields;
    tn_task_id id;

    if (line[0] == '\0') {
        return;
    }
    if (strcmp(line, "PING") == 0) {
        post_log(LOG_INFO, 2, "PONG");
        return;
    }
    if (strcmp(line, "GLITCH") == 0) {
        atomic_store(&s_glitch, 1u);
        return;
    }
    if (strcmp(line, "STATS") == 0) {
        char text[40];
        snprintf(text, sizeof text, "drops=%u ovf=%u rec=%u",
                 atomic_load(&s_drops),
                 uart_link_rx_overflows(),
                 atomic_load(&s_recoveries));
        post_log(LOG_INFO, 3, text);
        return;
    }

    fields = sscanf(line, "STALL %15s %u", name, &ms);
    if (fields < 1) {
        post_log(LOG_WARN, 9, "bad command");
        return;
    }
    if (task_from_name(name, &id) != 0) {
        post_log(LOG_WARN, 9, "bad task");
        return;
    }
    if (fields == 1) {
        atomic_store(&s_stall_until[id], 0xFFFFFFFFu);
    } else {
        atomic_store(&s_stall_until[id], board_millis() + ms);
    }
}

static void cmd_task(void *arg)
{
    char line[64];
    size_t used = 0;
    (void)arg;

    for (;;) {
        uint8_t byte;
        if (uart_link_try_recv(&byte) != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            line[used] = '\0';
            handle_line(line);
            used = 0;
            continue;
        }
        if (used + 1u >= sizeof line) {
            used = 0;
            continue;
        }
        line[used++] = (char)byte;
    }
}

static void recover(tn_task_id id)
{
    uint32_t now = board_millis();
    uint32_t stale = now - atomic_load(&s_last_kick[id]);
    app_event_t event;
    TaskFunction_t fn = NULL;
    const char *name = "task";
    UBaseType_t prio = PRIO_SENSOR;

    atomic_store(&s_stall_until[id], 0u);
    if (s_task[id] != NULL) {
        vTaskDelete(s_task[id]);
        s_task[id] = NULL;
        /* Idle frees the old pthread. Give it a tick before the replacement is created. */
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (id == TN_TASK_ENV) {
        fn = env_task;
        name = "env";
        prio = PRIO_SENSOR;
    } else if (id == TN_TASK_IMU) {
        fn = imu_task;
        name = "imu";
        prio = PRIO_SENSOR;
    } else if (id == TN_TASK_TELEM) {
        fn = telem_task;
        name = "telem";
        prio = PRIO_TELEM;
    } else {
        return;
    }

    atomic_store(&s_last_kick[id], board_millis());
    if (xTaskCreate(fn, name, STACK_WORDS, NULL, prio, &s_task[id]) != pdPASS) {
        board_log("task restart failed");
        return;
    }

    atomic_fetch_add(&s_recoveries, 1u);
    memset(&event, 0, sizeof event);
    event.kind = EVT_WDG;
    event.task_id = (uint8_t)id;
    event.count = (uint16_t)atomic_load(&s_recoveries);
    event.value = stale;
    (void)post_event(&event);
}

static void supervisor_task(void *arg)
{
    uint32_t health_at = 0;
    (void)arg;

    for (;;) {
        uint32_t kicks[TN_TASK_COUNT];
        uint32_t now;
        uint32_t stale;
        int id;

        if (stalled(TN_TASK_SUPERVISOR) != 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        now = board_millis();
        for (id = 0; id < TN_TASK_COUNT; id++) {
            kicks[id] = atomic_load(&s_last_kick[id]);
        }
        stale = wdg_stale_mask(kicks, now, WDG_TIMEOUT_MS, WDG_SUPERVISED_MASK);
        for (id = 0; id < TN_TASK_COUNT; id++) {
            if ((stale & (1u << id)) != 0u) {
                recover((tn_task_id)id);
            }
        }

        if ((now - health_at) >= 1000u) {
            app_event_t health;
            memset(&health, 0, sizeof health);
            health.kind = EVT_HEALTH;
            health.count = (uint16_t)atomic_load(&s_recoveries);
            health.value = now;
            (void)post_event(&health);
            health_at = now;
        }

        board_iwdg_refresh();
        kick(TN_TASK_SUPERVISOR);
        vTaskDelay(pdMS_TO_TICKS(WDG_PERIOD_MS));
    }
}

void app_set_boot(uint16_t boot_count, uint8_t node_id, const char *reason)
{
    s_boot_count = boot_count;
    s_node_id = node_id;
    snprintf(s_boot_reason, sizeof s_boot_reason, "%s", (reason != NULL) ? reason : "power");
}

void app_init(void)
{
    int id;

    uart_link_init();
    can_sim_init(board_can_hw_send);
    for (id = 0; id < TN_TASK_COUNT; id++) {
        atomic_store(&s_last_kick[id], board_millis());
        atomic_store(&s_stall_until[id], 0u);
        s_task[id] = NULL;
    }

    s_env_q = xQueueCreate(Q_ENV, sizeof(env_sample_t));
    s_imu_q = xQueueCreate(Q_IMU, sizeof(imu_sample_t));
    s_evt_q = xQueueCreate(Q_EVT, sizeof(app_event_t));
    s_set = xQueueCreateSet(Q_ENV + Q_IMU + Q_EVT);
    configASSERT(s_env_q != NULL);
    configASSERT(s_imu_q != NULL);
    configASSERT(s_evt_q != NULL);
    configASSERT(s_set != NULL);
    configASSERT(xQueueAddToSet(s_env_q, s_set) == pdPASS);
    configASSERT(xQueueAddToSet(s_imu_q, s_set) == pdPASS);
    configASSERT(xQueueAddToSet(s_evt_q, s_set) == pdPASS);
}

void app_create_tasks(void)
{
    uint32_t now = board_millis();
    int id;

    for (id = 0; id < TN_TASK_COUNT; id++) {
        atomic_store(&s_last_kick[id], now);
    }

    configASSERT(xTaskCreate(env_task, "env", STACK_WORDS, NULL, PRIO_SENSOR, &s_task[TN_TASK_ENV]) == pdPASS);
    configASSERT(xTaskCreate(imu_task, "imu", STACK_WORDS, NULL, PRIO_SENSOR, &s_task[TN_TASK_IMU]) == pdPASS);
    configASSERT(xTaskCreate(telem_task, "telem", STACK_WORDS, NULL, PRIO_TELEM, &s_task[TN_TASK_TELEM]) == pdPASS);
    configASSERT(xTaskCreate(supervisor_task, "wdg", STACK_WORDS, NULL, PRIO_SUPERVISOR, &s_task[TN_TASK_SUPERVISOR]) == pdPASS);
    configASSERT(xTaskCreate(cmd_task, "cmd", STACK_WORDS, NULL, PRIO_CMD, &s_cmd_task) == pdPASS);
}

void vApplicationMallocFailedHook(void)
{
    board_log("malloc failed");
    abort();
}
