#include "stm32_board.h"

#include "board.h"
#include "uart_link.h"

#include "stm32f4xx_hal.h"

#include <string.h>

static I2C_HandleTypeDef hi2c1;
static SPI_HandleTypeDef hspi1;
static UART_HandleTypeDef huart2;
static IWDG_HandleTypeDef hiwdg;
static CAN_HandleTypeDef hcan1;

static void system_clock(void)
{
    RCC_OscInitTypeDef osc;
    RCC_ClkInitTypeDef clk;

    memset(&osc, 0, sizeof osc);
    memset(&clk, 0, sizeof clk);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 7;
    (void)HAL_RCC_OscConfig(&osc);

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    (void)HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);
}

static void pin_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af, uint32_t pull)
{
    GPIO_InitTypeDef gpio;
    memset(&gpio, 0, sizeof gpio);
    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = pull;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = af;
    HAL_GPIO_Init(port, &gpio);
}

static void i2c_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    pin_af(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_AF4_I2C1, GPIO_PULLUP);

    memset(&hi2c1, 0, sizeof hi2c1);
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    (void)HAL_I2C_Init(&hi2c1);
}

static void spi_init(void)
{
    GPIO_InitTypeDef cs;

    memset(&cs, 0, sizeof cs);
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    pin_af(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_AF5_SPI1, GPIO_NOPULL);

    cs.Pin = GPIO_PIN_4;
    cs.Mode = GPIO_MODE_OUTPUT_PP;
    cs.Pull = GPIO_NOPULL;
    cs.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    cs.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &cs);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    memset(&hspi1, 0, sizeof hspi1);
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    (void)HAL_SPI_Init(&hspi1);
}

static void usart_init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    pin_af(GPIOA, GPIO_PIN_2 | GPIO_PIN_3, GPIO_AF7_USART2, GPIO_PULLUP);

    memset(&huart2, 0, sizeof huart2);
    huart2.Instance = USART2_PERIPH;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&huart2);
}

static void can_init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();
    pin_af(GPIOD, GPIO_PIN_0 | GPIO_PIN_1, GPIO_AF9_CAN1, GPIO_NOPULL);

    /* Internal loopback: frames are clocked by bxCAN without a transceiver. */
    memset(&hcan1, 0, sizeof hcan1);
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 6;
    hcan1.Init.Mode = CAN_MODE_LOOPBACK;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = DISABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    (void)HAL_CAN_Init(&hcan1);
    (void)HAL_CAN_Start(&hcan1);
}

void stm32_board_init(void)
{
    (void)HAL_Init();
    system_clock();
    i2c_init();
    spi_init();
    usart_init();
    can_init();
}

uint32_t board_millis(void)
{
    return HAL_GetTick();
}

int board_i2c_write(uint8_t addr_7bit, const uint8_t *data, size_t len)
{
    if ((data == NULL) || (len < 2u)) {
        return -1;
    }
    if (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(addr_7bit << 1), data[0], I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)(data + 1), (uint16_t)(len - 1u), 20) != HAL_OK) {
        return -1;
    }
    return 0;
}

int board_i2c_write_read(uint8_t addr_7bit, const uint8_t *wdata, size_t wlen,
                         uint8_t *rdata, size_t rlen)
{
    if ((wdata == NULL) || (wlen < 1u) || (rdata == NULL)) {
        return -1;
    }
    if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(addr_7bit << 1), wdata[0], I2C_MEMADD_SIZE_8BIT,
                         rdata, (uint16_t)rlen, 20) != HAL_OK) {
        return -1;
    }
    return 0;
}

int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    HAL_StatusTypeDef status;
    if ((tx == NULL) || (rx == NULL) || (len == 0u) || (len > 0xFFFFu)) {
        return -1;
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, (uint16_t)len, 20);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    return status == HAL_OK ? 0 : -1;
}

int board_uart_open(const char *endpoint, uint32_t baud)
{
    (void)endpoint;
    huart2.Init.BaudRate = baud;
    (void)HAL_UART_Init(&huart2);
    USART2->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    return 0;
}

void board_uart_arm_tx(void)
{
    USART2->CR1 |= USART_CR1_TXEIE;
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;

    if ((sr & USART_SR_RXNE) != 0u) {
        uart_link_isr_rx((uint8_t)USART2->DR);
    }
    if (((sr & USART_SR_TXE) != 0u) && ((USART2->CR1 & USART_CR1_TXEIE) != 0u)) {
        uint8_t byte;
        if (uart_link_isr_pop_tx(&byte) == 0) {
            USART2->DR = byte;
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
    if ((sr & USART_SR_ORE) != 0u) {
        (void)USART2->DR;
    }
}

void board_iwdg_start(uint32_t timeout_ms)
{
    uint32_t ticks;
    /* LSI is about 32 kHz. Prescaler 64 gives a 500 Hz tick. */
    ticks = (timeout_ms * 500u) / 1000u;
    if (ticks == 0u) {
        ticks = 1u;
    }
    if (ticks > 4095u) {
        ticks = 4095u;
    }
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = ticks;
    (void)HAL_IWDG_Init(&hiwdg);
}

void board_iwdg_refresh(void)
{
    (void)HAL_IWDG_Refresh(&hiwdg);
}

void board_on_iwdg_reset(void (*fn)(void))
{
    /* The F407 IWDG has no warning interrupt. Missing a refresh resets the core. */
    (void)fn;
}

void board_log(const char *line)
{
    if (line == NULL) {
        return;
    }
    while (*line != '\0') {
        ITM->PORT[0].u8 = (uint32_t)(uint8_t)(*line);
        line++;
    }
    ITM->PORT[0].u8 = (uint32_t)'\n';
}

int board_can_hw_send(const can_frame_t *frame)
{
    CAN_TxHeaderTypeDef hdr;
    uint32_t mailbox = 0;

    if (frame == NULL) {
        return -1;
    }
    memset(&hdr, 0, sizeof hdr);
    hdr.StdId = frame->id;
    hdr.ExtId = 0;
    hdr.IDE = CAN_ID_STD;
    hdr.RTR = CAN_RTR_DATA;
    hdr.DLC = frame->dlc;
    hdr.TransmitGlobalTime = DISABLE;
    if (HAL_CAN_AddTxMessage(&hcan1, &hdr, (uint8_t *)frame->data, &mailbox) != HAL_OK) {
        return -1;
    }
    return 0;
}
