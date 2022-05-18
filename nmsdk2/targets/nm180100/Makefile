SDK_ROOT	?= ../..

BUILDDIR_DBG := ./build/debug
SUFFIX_DBG   := -dbg
BUILDDIR_REL := ./build/release
SUFFIX_REL   := -rel

HAL		:= $(SDK_ROOT)/hal/ambiq
RTOS	:= $(SDK_ROOT)/rtos/FreeRTOS

include makedefs/common.mk
include makedefs/sources_hal.mk
include makedefs/sources_rtos.mk

HAL_OBJS_DBG += $(HAL_SRC:%.c=$(BUILDDIR_DBG)/%.o)
HAL_DEPS_DBG += $(HAL_SRC:%.c=$(BUILDDIR_DBG)/%.d)
HAL_LIB_DBG  := libhal$(SUFFIX_DBG).a

HAL_OBJS_REL += $(HAL_SRC:%.c=$(BUILDDIR_REL)/%.o)
HAL_DEPS_REL += $(HAL_SRC:%.c=$(BUILDDIR_REL)/%.d)
HAL_LIB_REL  := libhal$(SUFFIX_REL).a


RTOS_OBJS_DBG += $(RTOS_SRC:%.c=$(BUILDDIR_DBG)/%.o)
RTOS_DEPS_DBG += $(RTOS_SRC:%.c=$(BUILDDIR_DBG)/%.d)
RTOS_LIB_DBG  := librtos$(SUFFIX_DBG).a

RTOS_OBJS_REL += $(RTOS_SRC:%.c=$(BUILDDIR_REL)/%.o)
RTOS_DEPS_REL += $(RTOS_SRC:%.c=$(BUILDDIR_REL)/%.d)
RTOS_LIB_REL  := librtos$(SUFFIX_REL).a


all: debug release


debug: $(BUILDDIR_DBG) hal_dbg rtos_dbg

$(BUILDDIR_DBG):
	$(MKDIR) -p "$@"

hal_dbg: $(BUILDDIR_DBG)/$(HAL_LIB_DBG)

$(BUILDDIR_DBG)/$(HAL_LIB_DBG): $(HAL_OBJS_DBG)
	$(AR) rsvc $@ $^

$(HAL_OBJS_DBG): $(HAL_SRC)
	$(CC) -c $(CFLAGS_DBG) $(HAL_INC) $< -o $@

rtos_dbg: $(BUILDDIR_DBG)/$(RTOS_LIB_DBG)

$(BUILDDIR_DBG)/$(RTOS_LIB_DBG): $(RTOS_OBJS_DBG)
	$(AR) rsvc $@ $^

$(RTOS_OBJS_DBG): $(RTOS_SRC)
	$(CC) -c $(CFLAGS_DBG) $(RTOS_INC) $< -o $@



release: $(BUILDDIR_REL) hal_rel rtos_rel

$(BUILDDIR_REL):
	$(MKDIR) -p "$@"

hal_rel: $(BUILDDIR_REL)/$(HAL_LIB_REL)

$(BUILDDIR_REL)/$(HAL_LIB_REL): $(HAL_OBJS_REL)
	$(AR) rsvc $@ $^

$(HAL_OBJS_REL): $(HAL_SRC)
	$(CC) -c $(CFLAGS_REL) $(HAL_INC) $< -o $@

rtos_rel: $(BUILDDIR_REL)/$(RTOS_LIB_REL)

$(BUILDDIR_REL)/$(RTOS_LIB_REL): $(RTOS_OBJS_REL)
	$(AR) rsvc $@ $^

$(RTOS_OBJS_REL): $(RTOS_SRC)
	$(CC) -c $(CFLAGS_REL) $(RTOS_INC) $< -o $@


clean:
	$(RM) -rf ./build