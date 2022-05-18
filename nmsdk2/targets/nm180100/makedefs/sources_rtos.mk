DEFINES += -DAM_FREERTOS

INCLUDES  = -I$(AMBIQ)/mcu/apollo3
INCLUDES += -I$(AMBIQ)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ)/CMSIS/ARM/Include
INCLUDES += -I$(RTOS)/kernel/include
INCLUDES += -I$(RTOS)/cli/include
INCLUDES += -I./rtos/FreeRTOS
INCLUDES += -I./rtos/FreeRTOS/portable

VPATH += $(RTOS)/kernel
VPATH += $(RTOS)/cli
VPATH += ./rtos/FreeRTOS/portable

SRC += croutine.c
SRC += event_groups.c
SRC += list.c
SRC += queue.c
SRC += stream_buffer.c
SRC += tasks.c
SRC += timers.c
SRC += heap_4.c
SRC += port.c

SRC += FreeRTOS_CLI.c