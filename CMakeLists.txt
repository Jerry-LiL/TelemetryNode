cmake_minimum_required(VERSION 3.16)
project(TelemetryNode C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(freertos STATIC
    third_party/FreeRTOS-Kernel/tasks.c
    third_party/FreeRTOS-Kernel/queue.c
    third_party/FreeRTOS-Kernel/list.c
    third_party/FreeRTOS-Kernel/portable/MemMang/heap_4.c
    third_party/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/port.c
    third_party/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
)
target_include_directories(freertos PUBLIC
    third_party/FreeRTOS-Kernel/include
    third_party/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix
    firmware/platform/host
)
target_link_libraries(freertos PUBLIC pthread)

set(TN_WARNINGS -Wall -Wextra -Werror)

add_library(tn_core STATIC
    firmware/src/crc16.c
    firmware/src/frame.c
    firmware/src/payload.c
    firmware/src/can_codec.cpp
    firmware/src/can_sim.c
    firmware/src/wdg.c
    firmware/src/uart_link.c
    firmware/src/bme280.c
    firmware/src/icm42688.c
)
target_include_directories(tn_core PUBLIC firmware/include)
target_compile_options(tn_core PRIVATE ${TN_WARNINGS})

add_library(tn_app STATIC firmware/src/app.c)
target_link_libraries(tn_app PUBLIC tn_core freertos)
target_compile_options(tn_app PRIVATE ${TN_WARNINGS})

add_executable(telemetry_node
    firmware/platform/host/main.c
    firmware/platform/host/board.c
    firmware/platform/host/host_uart.c
    firmware/platform/host/bme280_model.c
    firmware/platform/host/icm42688_model.c
)
target_include_directories(telemetry_node PRIVATE firmware/platform/host)
target_link_libraries(telemetry_node PRIVATE tn_app)
target_compile_options(telemetry_node PRIVATE ${TN_WARNINGS})

function(tn_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE tests firmware/include)
    target_compile_options(${name} PRIVATE ${TN_WARNINGS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

enable_testing()
tn_test(test_crc tests/test_crc.c firmware/src/crc16.c)
tn_test(test_frame tests/test_frame.c firmware/src/frame.c firmware/src/crc16.c firmware/src/payload.c)
tn_test(test_wdg tests/test_wdg.c firmware/src/wdg.c)
tn_test(test_uart_link tests/test_uart_link.c firmware/src/uart_link.c)
target_include_directories(test_uart_link PRIVATE firmware/include)

tn_test(test_can tests/test_can.cpp firmware/src/can_codec.cpp firmware/src/can_sim.c)
tn_test(test_sensors
    tests/test_sensors.c
    firmware/src/bme280.c
    firmware/src/icm42688.c
    firmware/platform/host/bme280_model.c
    firmware/platform/host/icm42688_model.c
)
target_include_directories(test_sensors PRIVATE firmware/platform/host)
target_link_libraries(test_sensors PRIVATE m)

add_library(stm32_bsp OBJECT
    firmware/platform/stm32/board.c
    firmware/platform/stm32/main.c
)
target_include_directories(stm32_bsp PRIVATE
    firmware/platform/stm32/shim
    firmware/include
    third_party/FreeRTOS-Kernel/include
    third_party/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix
    firmware/platform/host
)
target_compile_options(stm32_bsp PRIVATE ${TN_WARNINGS})

add_library(esp32_bsp OBJECT
    firmware/platform/esp32/board.c
    firmware/platform/esp32/main.c
)
target_include_directories(esp32_bsp PRIVATE
    firmware/platform/esp32/shim
    firmware/include
)
target_compile_options(esp32_bsp PRIVATE ${TN_WARNINGS})
