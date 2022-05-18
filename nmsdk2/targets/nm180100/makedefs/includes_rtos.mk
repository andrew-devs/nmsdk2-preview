RTOS_DEFINES += -DAM_FREERTOS

RTOS_INC += -I$(HAL)/mcu/apollo3
RTOS_INC += -I$(HAL)/CMSIS/AmbiqMicro/Include
RTOS_INC += -I$(HAL)/CMSIS/ARM/Include
RTOS_INC += -I$(RTOS)/kernel/include
RTOS_INC += -I$(RTOS)/cli/include
RTOS_INC += -I./rtos/FreeRTOS
RTOS_INC += -I./rtos/FreeRTOS/portable

