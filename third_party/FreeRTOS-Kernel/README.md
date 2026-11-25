# FreeRTOS Kernel V11.2.0 (subset)

Upstream: https://github.com/FreeRTOS/FreeRTOS-Kernel tag `V11.2.0`
License: MIT (see `LICENSE.md`)

This tree keeps the kernel sources the host build links, plus the GCC POSIX port:

- `tasks.c`, `queue.c`, `list.c`
- `include/`
- `portable/MemMang/heap_4.c`
- `portable/ThirdParty/GCC/Posix/`

STM32 builds should use the upstream GCC/ARM_CM4F port instead of the POSIX port. ESP-IDF ships its own FreeRTOS.
