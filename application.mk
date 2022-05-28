#******************************************************************************
#
# Step 1
# Define the locations of the various SDKs and libraries.
#
#******************************************************************************
NMSDK    ?= $(shell pwd)/nmsdk2
TARGET   := $(NMSDK)/targets/nm180100
LDSCRIPT := ldscript.ld

#******************************************************************************
#
# Step 2
# Specify the location of the board support package to be used.
#
#******************************************************************************
BSP_DIR := ./bsp/nm180100evb
BSP_SRC := bsp_pins.src

#******************************************************************************
#
# Step 3
# Specify output version and name
#
#******************************************************************************
OUTPUT_VERSION := 0x00

OUTPUT      := nmapp
OUTPUT_OTA  := nmapp-ota

#******************************************************************************
#
# Step 4
# Specify SDK custom configurations here.  Use $(shell pwd) expansion as an
# absolute path is required.  The SDK must be rebuilt manually the first time a
# configuration file is overridden.
#
#   make clean-sdk
#
# For example: 
#   FREERTOS_CONFIG := $(shell pwd)/config/FreeRTOSConfig.h
#
# Current overridable configurations are:
#   FREERTOS_CONFIG
#	LORAWAN_EEPROM_CONFIG
#
#******************************************************************************
# FREERTOS_CONFIG := $(shell pwd)/config/FreeRTOSConfig.h

#******************************************************************************
#
# Step 5
# Include additional source, header, libraries or paths below.
#
# Examples:
#   INCLUDES += -Iadditional_include_path
#   VPATH    += additional_source_path
#   LIBS     += -ladditional_library
#******************************************************************************
INCLUDES += -I.
INCLUDES += -I./config

VPATH += .

SRC += startup_gcc.c
SRC += main.c
SRC += console_task.c
SRC += application_task.c
SRC += application_task_cli.c


DEFINES += -DSOFT_SE
DEFINES += -DCONTEXT_MANAGEMENT_ENABLED

INCLUDES += -I./comms/lorawan/common/LmHandler/packages
INCLUDES += -I./comms/lorawan/common/LmHandler
INCLUDES += -I./comms/lorawan/common
INCLUDES += -I./comms/lorawan/soft-se
INCLUDES += -I./comms/lorawan

VPATH += ./comms/lorawan/common/LmHandler/packages
VPATH += ./comms/lorawan/common/LmHandler
VPATH += ./comms/lorawan/common
VPATH += ./comms/lorawan/soft-se
VPATH += ./comms/lorawan

SRC += aes.c
SRC += cmac.c
SRC += soft-se.c
SRC += lmh_callbacks.c
SRC += lmhp_fragmentation.c
SRC += lorawan_se.c
SRC += lorawan_task.c
SRC += lorawan_task_cli.c