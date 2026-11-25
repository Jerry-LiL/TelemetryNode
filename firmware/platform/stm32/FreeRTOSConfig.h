#ifndef FREERTOS_CONFIG_STM32_H
#define FREERTOS_CONFIG_STM32_H

/* STM32F407 @ 168 MHz, SysTick owned by FreeRTOS. Not used by the host build. */

#define configUSE_PREEMPTION                       1
#define configUSE_IDLE_HOOK                        0
#define configUSE_TICK_HOOK                        1
#define configCPU_CLOCK_HZ                         (168000000UL)
#define configTICK_RATE_HZ                         (1000)
#define configMAX_PRIORITIES                       (6)
#define configMINIMAL_STACK_SIZE                   (128)
#define configTOTAL_HEAP_SIZE                      (24 * 1024)
#define configMAX_TASK_NAME_LEN                    (16)
#define configUSE_16_BIT_TICKS                     0
#define configIDLE_SHOULD_YIELD                    1
#define configUSE_MUTEXES                          1
#define configUSE_QUEUE_SETS                       1
#define configUSE_TASK_NOTIFICATIONS               1
#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configSUPPORT_STATIC_ALLOCATION            0
#define configUSE_TIMERS                           0
#define configCHECK_FOR_STACK_OVERFLOW             2
#define configUSE_MALLOC_FAILED_HOOK               1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#define configKERNEL_INTERRUPT_PRIORITY            (0xF0)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY       (0x50)

#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for (;;) { } }

#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_vTaskDelayUntil              1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_xTaskGetCurrentTaskHandle    1
#define INCLUDE_xTaskGetSchedulerState       1

#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif
