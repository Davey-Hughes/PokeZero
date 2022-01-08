export TARGET := pokezero
export BUILD := build
export BINDIR := $(BUILD)/bin

# debug is the default build
export OBJDIR := $(BUILD)/objects/debug
export CXXTARGET := $(BINDIR)/$(TARGET)_debug

.PHONY: all debug release format run clean showdown $(GIT_SUBMODULES)

# passes C++ building to another makefile to assist in separate release and
# debug builds
all debug release format $(TARGET):
	$(MAKE) $@ --no-print-directory -j -f makefiles/main.mk

run:
	$(MAKE) $@ --no-print-directory -j -f makefiles/main.mk

# update git submodules
GIT=git
GIT_SUBMODULES = $(shell sed -nE 's/path = +(.+)/\1\/.git/ p' .gitmodules | paste -s -)
$(GIT_SUBMODULES): %/.git: .gitmodules
	$(GIT) submodule update --init $*
	@touch $@

# build the showdown source code
SHOWDOWN_DIR := ./pokemon-showdown
showdown: $(GIT_SUBMODULES)
	$(SHOWDOWN_DIR)/build

# change the executale target and object build location on release build
release: export OBJDIR := $(BUILD)/objects/release
release: export CXXTARGET := $(BINDIR)/$(TARGET)_release

clean:
	rm -rvf $(BUILD)
	rm -vf $(TARGET)
