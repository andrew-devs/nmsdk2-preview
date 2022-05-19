RTOS_DEFINES += -DAM_FREERTOS

RTOS_INC += -I$(RTOS)/kernel/include
RTOS_INC += -I$(RTOS)/cli/include
RTOS_INC += -I./rtos/FreeRTOS
RTOS_INC += -I./rtos/FreeRTOS/portable

