#include "app.h"
#include "board.h"
#include "esp32_board.h"

void app_main(void)
{
    esp32_board_init();
    app_set_boot(1u, 1u, "power");
    app_init();
    if (board_uart_open("uart1", 115200u) != 0) {
        board_log("uart init failed");
        return;
    }
    board_iwdg_start(1500u);
    app_create_tasks();
}
