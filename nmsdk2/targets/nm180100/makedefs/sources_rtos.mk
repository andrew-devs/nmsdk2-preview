RTOS_DEFINES += -DAM_FREERTOS

RTOS_INC += -I$(HAL)/mcu/apollo3
RTOS_INC += -I$(HAL)/CMSIS/AmbiqMicro/Include
RTOS_INC += -I$(HAL)/CMSIS/ARM/Include
RTOS_INC += -I$(RTOS)/kernel/include
RTOS_INC += -I$(RTOS)/cli/include
RTOS_INC += -I./rtos/FreeRTOS
RTOS_INC += -I./rtos/FreeRTOS/portable

VPATH += $(RTOS)/kernel
VPATH += $(RTOS)/cli
VPATH += ./rtos/FreeRTOS/portable

RTOS_SRC += croutine.c
RTOS_SRC += event_groups.c
RTOS_SRC += list.c
RTOS_SRC += queue.c
RTOS_SRC += stream_buffer.c
RTOS_SRC += tasks.c
RTOS_SRC += timers.c
RTOS_SRC += heap_4.c
RTOS_SRC += port.c

RTOS_SRC += FreeRTOS_CLI.c