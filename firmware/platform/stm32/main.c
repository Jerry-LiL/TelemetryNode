#include "app.h"
#include "board.h"
#include "stm32_board.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx_hal.h"

void vApplicationTickHook(void)
{
    HAL_IncTick();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

int main(void)
{
    stm32_board_init();
    app_set_boot(1u, 1u, "power");
    app_init();
    if (board_uart_open(NULL, 115200u) != 0) {
        for (;;) {
        }
    }
    board_iwdg_start(1500u);
    app_create_tasks();
    vTaskStartScheduler();
    for (;;) {
    }
}
