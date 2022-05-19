SDK_ROOT   ?= ../..
INSTALLDIR := ./lib

include makedefs/common.mk

all: debug release

install: debug release $(INSTALLDIR) hal_install rtos_install

$(INSTALLDIR):
	$(MKDIR) -p "$@"

debug: $(BUILDDIR_DBG) hal_dbg rtos_dbg

$(BUILDDIR_DBG):
	$(MKDIR) -p "$@"

release: $(BUILDDIR_REL) hal_rel rtos_rel

$(BUILDDIR_REL):
	$(MKDIR) -p "$@"

include makedefs/build_hal.mk
include makedefs/build_rtos.mk

clean:
	$(RM) -rf ./build

uninstall:
	$(RM) -rf ./lib