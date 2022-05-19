#******************************************************************************
#
# Step 1
# Define the locations of the various SDKs and libraries.
#
#******************************************************************************
NMSDK  ?= ./nmsdk2
TARGET := $(NMSDK)/targets/nm180100
LDSCRIPT := ldscript.ld

#******************************************************************************
#
# Step 2
# Specify the location of the board support package to be used.
#
#******************************************************************************

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
# Include additional source, header, libraries or paths below.
#
# Examples:
#   INCLUDES += -Iadditional_include_path
#   VPATH    += additional_source_path
#   LIBS     += -ladditional_library
#******************************************************************************
INCLUDES += -I.

VPATH += .

SRC += startup_gcc.c
SRC += main.c
SRC += application.c
