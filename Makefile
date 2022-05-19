include application.mk

SDK_ROOT := $(NMSDK)
include $(TARGET)/makedefs/common.mk
include $(TARGET)/makedefs/defs_hal.mk
include $(TARGET)/makedefs/defs_rtos.mk
include $(TARGET)/makedefs/includes_hal.mk
include $(TARGET)/makedefs/includes_rtos.mk

CFLAGS_DBG += $(INCLUDES) $(HAL_INC) $(RTOS_INC)
CFLAGS_REL += $(INCLUDES) $(HAL_INC) $(RTOS_INC)

LIBS_DBG += $(HAL_LIB_DBG)
LIBS_DBG += $(RTOS_LIB_DBG)
SDK_LIBS_DBG = $(LIBS_DBG:%.a=$(TARGET)/lib/%.a)

LFLAGS_DBG += $(LFLAGS)
LFLAGS_DBG += -Wl,--start-group
LFLAGS_DBG += -L$(TARGET)/lib
LFLAGS_DBG += -lm
LFLAGS_DBG += -lc
LFLAGS_DBG += -lgcc
LFLAGS_DBG += $(patsubst lib%.a,-l%,$(subst :, , $(LIBS_DBG)))
LFLAGS_DBG += --specs=nano.specs
LFLAGS_DBG += --specs=nosys.specs
LFLAGS_DBG += -Wl,--end-group


LIBS_REL += $(HAL_LIB_REL)
LIBS_REL += $(RTOS_LIB_REL)
SDK_LIBS_REL = $(LIBS_REL:%.a=$(TARGET)/lib/%.a)

LFLAGS_REL += $(LFLAGS)
LFLAGS_REL += -Wl,--start-group
LFLAGS_REL += -L$(TARGET)/lib
LFLAGS_REL += -lm
LFLAGS_REL += -lc
LFLAGS_REL += -lgcc
LFLAGS_REL += $(patsubst lib%.a,-l%,$(subst :, , $(LIBS_REL)))
LFLAGS_REL += --specs=nano.specs
LFLAGS_REL += --specs=nosys.specs
LFLAGS_REL += -Wl,--end-group


all: nmsdk debug release

nmsdk:
	make -C $(TARGET) install

OBJS_DBG += $(SRC:%.c=$(BUILDDIR_DBG)/%.o)
DEPS_DBG += $(SRC:%.c=$(BUILDDIR_DBG)/%.o)
OUTPUT_DBG := $(BUILDDIR_DBG)/$(OUTPUT)$(SUFFIX_DBG).axf
OUTPUT_BIN_DBG := $(OUTPUT_DBG:%.axf=%.bin)
OUTPUT_LST_DBG := $(OUTPUT_DBG:%.axf=%.lst)
OUTPUT_SIZE_DBG := $(OUTPUT_DBG:%.axf=%.size)

debug: $(BUILDDIR_DBG) $(OUTPUT_BIN_DBG)

$(BUILDDIR_DBG):
	$(MKDIR) -p "$@"

$(OUTPUT_BIN_DBG): $(OUTPUT_DBG)
	$(OCP) $(OCPFLAGS) $< $@
	$(OD)  $(ODFLAGS) $< > $(OUTPUT_LST_DBG)
	$(SIZE) $(OBJS_DBG) $(OUTPUT_DBG)

$(OUTPUT_DBG): $(OBJS_DBG) $(SDK_LIBS_DBG)
	$(CC) -Wl,-T,$(LDSCRIPT) -o $@ $(OBJS_DBG) $(LFLAGS_DBG)

$(OBJS_DBG): $(BUILDDIR_DBG)/%.o : %.c
	$(CC) -c $(CFLAGS_DBG) $< -o $@


OBJS_REL += $(SRC:%.c=$(BUILDDIR_REL)/%.o)
DEPS_REL += $(SRC:%.c=$(BUILDDIR_REL)/%.o)
OUTPUT_REL := $(BUILDDIR_REL)/$(OUTPUT)$(SUFFIX_REL).axf
OUTPUT_BIN_REL := $(OUTPUT_REL:%.axf=%.bin)
OUTPUT_LST_REL := $(OUTPUT_REL:%.axf=%.lst)
OUTPUT_SIZE_REL := $(OUTPUT_REL:%.axf=%.size)

release: $(BUILDDIR_REL) $(OUTPUT_BIN_REL)

$(BUILDDIR_REL):
	$(MKDIR) -p "$@"

$(OUTPUT_BIN_REL): $(OUTPUT_REL)
	$(OCP) $(OCPFLAGS) $< $@
	$(OD)  $(ODFLAGS) $< > $(OUTPUT_LST_REL)
	$(SIZE) $(OBJS_REL) $(OUTPUT_REL)

$(OUTPUT_REL): $(OBJS_REL) $(SDK_LIBS_REL)
	$(CC) -Wl,-T,$(LDSCRIPT) -o $@ $(OBJS_REL) $(LFLAGS_REL)

$(OBJS_REL): $(BUILDDIR_REL)/%.o : %.c
	$(CC) -c $(CFLAGS_REL) $< -o $@

clean-debug:
	$(RM) -rf $(BUILDDIR_DBG)

clean-release:
	$(RM) -rf $(BUILDDIR_REL)

cleanall:
	$(RM) -rf ./build

.phony: nmsdk