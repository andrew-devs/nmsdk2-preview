RTOS	:= $(SDK_ROOT)/rtos/FreeRTOS
RTOS_LIB_DBG  := librtos$(SUFFIX_DBG).a
RTOS_LIB_REL  := librtos$(SUFFIX_REL).a
FREERTOSCONFIG ?= $(RTOS)/../../targets/nm180100/rtos/FreeRTOS/FreeRTOSConfig.h