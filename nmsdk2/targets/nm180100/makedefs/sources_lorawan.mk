VPATH += $(LORAWAN)/src/radio/sx126x
VPATH += $(LORAWAN)/src/boards/mcu
VPATH += $(LORAWAN)/src/mac
VPATH += $(LORAWAN)/src/mac/region
VPATH += $(LORAWAN)/src/system
VPATH += $(LORAWAN)/src/peripherals/soft-se
VPATH += ./comms/lorawan
VPATH += ./comms/lorawan/src/boards/nm180100

LORAWAN_SRC += radio.c
LORAWAN_SRC += sx126x.c
LORAWAN_SRC += utilities.c

LORAWAN_SRC += LoRaMacAdr.c
LORAWAN_SRC += LoRaMac.c
LORAWAN_SRC += LoRaMacClassB.c
LORAWAN_SRC += LoRaMacCommands.c
LORAWAN_SRC += LoRaMacConfirmQueue.c
LORAWAN_SRC += LoRaMacCrypto.c
LORAWAN_SRC += LoRaMacParser.c
LORAWAN_SRC += LoRaMacSerializer.c

LORAWAN_SRC += RegionAS923.c
LORAWAN_SRC += RegionAU915.c
LORAWAN_SRC += RegionBaseUS.c
LORAWAN_SRC += Region.c
#LORAWAN_SRC += RegionCN470.c
#LORAWAN_SRC += RegionCN779.c
LORAWAN_SRC += RegionCommon.c
#LORAWAN_SRC += RegionEU433.c
LORAWAN_SRC += RegionEU868.c
LORAWAN_SRC += RegionIN865.c
LORAWAN_SRC += RegionKR920.c
LORAWAN_SRC += RegionRU864.c
LORAWAN_SRC += RegionUS915.c

LORAWAN_SRC += delay.c
LORAWAN_SRC += nvmm.c
LORAWAN_SRC += timer.c
LORAWAN_SRC += systime.c

LORAWAN_SRC += board.c
LORAWAN_SRC += delay-board.c
LORAWAN_SRC += eeprom-board.c
LORAWAN_SRC += rtc-board.c
LORAWAN_SRC += sx1262-board.c

LORAWAN_SRC += lorawan_power.c
