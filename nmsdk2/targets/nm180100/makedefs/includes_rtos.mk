RTOS_DEFINES += -DAM_FREERTOS

RTOS_INC += -I$(RTOS)/kernel/include
RTOS_INC += -I$(RTOS)/cli/include
RTOS_INC += -I$(RTOS)/../../targets/nm180100/rtos/FreeRTOS
RTOS_INC += -I$(RTOS)/../../targets/nm180100/rtos/FreeRTOS/portable

