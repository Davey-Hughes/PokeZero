export BUILD := ./build
export OBJDIR := $(BUILD)/objects/debug
export TARGET := pokezero

.PHONY: all debug release clean

release: export OBJDIR := $(BUILD)/objects/release

all run debug release:
	$(MAKE) $@ --no-print-directory -j -f makefiles/main.mk

clean:
	rm -rvf $(BUILD)
	rm -vf $(TARGET)
