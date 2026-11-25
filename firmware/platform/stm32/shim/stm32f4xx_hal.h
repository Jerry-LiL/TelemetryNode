#ifndef STM32F4XX_HAL_SHIM_H
#define STM32F4XX_HAL_SHIM_H

/* Compile-check stand-in for the STM32F4 HAL headers. A Cube build drops this
 * include path and uses the real stm32f4xx_hal.h. */

#include <stdint.h>
#include <stddef.h>

typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;

typedef struct { volatile uint32_t SR; volatile uint32_t DR; volatile uint32_t BRR; volatile uint32_t CR1; volatile uint32_t CR2; volatile uint32_t CR3; volatile uint32_t GTPR; } USART_TypeDef;
typedef struct { volatile uint32_t u8; } ITM_PORT_Type;
typedef struct { ITM_PORT_Type PORT[32]; } ITM_Type;

extern USART_TypeDef g_usart2_regs;
extern ITM_Type g_itm;
#define USART2 (&g_usart2_regs)
#define ITM (&g_itm)

#define USART_SR_RXNE   (1u << 5)
#define USART_SR_TXE    (1u << 7)
#define USART_SR_ORE    (1u << 3)
#define USART_CR1_TXEIE (1u << 7)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_CR1_UE    (1u << 13)
#define USART_CR1_TE    (1u << 3)
#define USART_CR1_RE    (1u << 2)

#define GPIO_PIN_0  0x0001u
#define GPIO_PIN_1  0x0002u
#define GPIO_PIN_2  0x0004u
#define GPIO_PIN_3  0x0008u
#define GPIO_PIN_4  0x0010u
#define GPIO_PIN_5  0x0020u
#define GPIO_PIN_6  0x0040u
#define GPIO_PIN_7  0x0080u
#define GPIO_PIN_8  0x0100u
#define GPIO_PIN_9  0x0200u
#define GPIO_PIN_RESET 0u
#define GPIO_PIN_SET   1u
#define GPIO_MODE_AF_PP 2u
#define GPIO_MODE_AF_OD 18u
#define GPIO_MODE_OUTPUT_PP 1u
#define GPIO_NOPULL 0u
#define GPIO_PULLUP 1u
#define GPIO_SPEED_FREQ_VERY_HIGH 3u

#define GPIO_AF4_I2C1  4u
#define GPIO_AF5_SPI1  5u
#define GPIO_AF7_USART2 7u
#define GPIO_AF9_CAN1  9u

#define RCC_OSCILLATORTYPE_HSE 1u
#define RCC_HSE_ON 1u
#define RCC_PLL_ON 2u
#define RCC_PLLSOURCE_HSE 1u
#define RCC_PLLP_DIV2 2u
#define RCC_CLOCKTYPE_SYSCLK 1u
#define RCC_CLOCKTYPE_HCLK 2u
#define RCC_CLOCKTYPE_PCLK1 4u
#define RCC_CLOCKTYPE_PCLK2 8u
#define RCC_SYSCLKSOURCE_PLLCLK 2u
#define RCC_SYSCLK_DIV1 0u
#define RCC_HCLK_DIV4 5u
#define RCC_HCLK_DIV2 4u
#define FLASH_LATENCY_5 5u

#define I2C_ADDRESSINGMODE_7BIT 1u
#define I2C_DUTYCYCLE_2 0u
#define I2C_DUALADDRESS_DISABLE 0u
#define I2C_GENERALCALL_DISABLE 0u
#define I2C_NOSTRETCH_DISABLE 0u
#define I2C_MEMADD_SIZE_8BIT 1u

#define SPI_MODE_MASTER 1u
#define SPI_DIRECTION_2LINES 0u
#define SPI_DATASIZE_8BIT 8u
#define SPI_POLARITY_LOW 0u
#define SPI_PHASE_1EDGE 0u
#define SPI_NSS_SOFT 1u
#define SPI_BAUDRATEPRESCALER_16 16u
#define SPI_FIRSTBIT_MSB 0u
#define SPI_TIMODE_DISABLE 0u
#define SPI_CRCCALCULATION_DISABLE 0u

#define UART_MODE_TX_RX 3u
#define UART_WORDLENGTH_8B 8u
#define UART_STOPBITS_1 1u
#define UART_PARITY_NONE 0u
#define UART_HWCONTROL_NONE 0u
#define UART_OVERSAMPLING_16 16u

#define IWDG_PRESCALER_64 4u

#define CAN_MODE_LOOPBACK 2u
#define CAN_SJW_1TQ 1u
#define CAN_BS1_13TQ 13u
#define CAN_BS2_2TQ 2u
#define CAN_ID_STD 0u
#define CAN_RTR_DATA 0u
#define DISABLE 0u

typedef struct { uint32_t PLLState; uint32_t PLLSource; uint32_t PLLM; uint32_t PLLN; uint32_t PLLP; uint32_t PLLQ; } RCC_PLLInitTypeDef;
typedef struct { uint32_t OscillatorType; uint32_t HSEState; uint32_t PLLState; RCC_PLLInitTypeDef PLL; } RCC_OscInitTypeDef;
typedef struct { uint32_t ClockType; uint32_t SYSCLKSource; uint32_t AHBCLKDivider; uint32_t APB1CLKDivider; uint32_t APB2CLKDivider; } RCC_ClkInitTypeDef;
typedef struct { uint32_t Pin; uint32_t Mode; uint32_t Pull; uint32_t Speed; uint32_t Alternate; } GPIO_InitTypeDef;
typedef struct { uint32_t ClockSpeed; uint32_t DutyCycle; uint32_t OwnAddress1; uint32_t AddressingMode; uint32_t DualAddressMode; uint32_t OwnAddress2; uint32_t GeneralCallMode; uint32_t NoStretchMode; } I2C_InitTypeDef;
typedef struct { uint32_t Mode; uint32_t Direction; uint32_t DataSize; uint32_t CLKPolarity; uint32_t CLKPhase; uint32_t NSS; uint32_t BaudRatePrescaler; uint32_t FirstBit; uint32_t TIMode; uint32_t CRCCalculation; uint32_t CRCPolynomial; } SPI_InitTypeDef;
typedef struct { uint32_t BaudRate; uint32_t WordLength; uint32_t StopBits; uint32_t Parity; uint32_t Mode; uint32_t HwFlowCtl; uint32_t OverSampling; } UART_InitTypeDef;
typedef struct { uint32_t Prescaler; uint32_t Reload; } IWDG_InitTypeDef;
typedef struct { uint32_t Prescaler; uint32_t Mode; uint32_t SyncJumpWidth; uint32_t TimeSeg1; uint32_t TimeSeg2; uint32_t TimeTriggeredMode; uint32_t AutoBusOff; uint32_t AutoWakeUp; uint32_t AutoRetransmission; uint32_t ReceiveFifoLocked; uint32_t TransmitFifoPriority; } CAN_InitTypeDef;

typedef struct GPIO_TypeDef GPIO_TypeDef;
typedef struct { void *Instance; I2C_InitTypeDef Init; } I2C_HandleTypeDef;
typedef struct { void *Instance; SPI_InitTypeDef Init; } SPI_HandleTypeDef;
typedef struct { void *Instance; UART_InitTypeDef Init; } UART_HandleTypeDef;
typedef struct { void *Instance; IWDG_InitTypeDef Init; } IWDG_HandleTypeDef;
typedef struct { void *Instance; CAN_InitTypeDef Init; } CAN_HandleTypeDef;
typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; uint32_t TransmitGlobalTime; } CAN_TxHeaderTypeDef;

extern GPIO_TypeDef *GPIOA;
extern GPIO_TypeDef *GPIOB;
extern GPIO_TypeDef *GPIOD;
extern void *I2C1;
extern void *SPI1;
extern void *USART2_PERIPH;
extern void *IWDG;
extern void *CAN1;

#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOD_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_SPI1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_CAN1_CLK_ENABLE() ((void)0)

HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *cfg);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *cfg, uint32_t latency);
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, uint32_t state);
uint32_t HAL_GetTick(void);
void HAL_IncTick(void);

HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t addr, uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t addr, uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t len, uint32_t timeout);

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout);

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);

HAL_StatusTypeDef HAL_IWDG_Init(IWDG_HandleTypeDef *hiwdg);
HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg);

HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *hdr, uint8_t *data, uint32_t *mailbox);

#endif
