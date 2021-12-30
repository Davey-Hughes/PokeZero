TARGET := pokezero
BUILD := ./build
OBJDIR := $(BUILD)/objects
BINDIR := $(BUILD)/bin
DEPDIR := $(OBJDIR)/.deps

BASE_FLAGS := -Wall -Werror -Wextra -pedantic-errors

# compiler
CXX := clang++
# linker
LD := $(CXX)
# linker flags
LDFLAGS :=
# linker flags: libraries to link (e.g. -lfoo)
LDLIBS :=
# flags required for dependency generation; passed to compilers
DEPFLAGS = -MQ $@ -MD -MP -MF $(DEPDIR)/$*.Td

CXXTARGET := $(TARGET)
CXXFLAGS := -std=c++20 $(BASE_FLAGS)
SYSINCLUDE :=
CXXINCLUDE := -Iinclude
CXXSRC := $(wildcard src/*.cc)
CXXOBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(CXXSRC)))
CXXDEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(CXXSRC)))

# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CXXINCLUDE) $(SYSINCLUDE) -c -o $@
# link object files to binary
LINK.o = $(LD) $(LDFLAGS) $(LDLIBS) -o $(BINDIR)/$@
# precompile step
PRECOMPILE =
# postcompile step
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

.PHONY: all debug release clean

all: debug

PokeZero: $(CXXTARGET)
	ln -sf $(BINDIR)/$(CXXTARGET) $(TARGET)

debug: CXXFLAGS += -DDEBUG -g -Og
debug: LDLIBS += -fsanitize=leak
debug: PokeZero

release: CXXFLAGS += -O3
release: PokeZero

clean:
	rm -rvf $(BUILD)
	rm -vf $(TARGET)

$(CXXTARGET): $(CXXOBJS)
	@mkdir -p $(BINDIR)
	$(LINK.o) $^

$(OBJDIR)/%.o: %.cc
$(OBJDIR)/%.o: %.cc $(DEPDIR)/%.d
	$(shell mkdir -p $(dir $(CXXOBJS)) >/dev/null)
	$(shell mkdir -p $(dir $(CXXDEPS)) >/dev/null)
	$(PRECOMPILE)
	$(COMPILE.cc) $<
	$(POSTCOMPILE)

.PRECIOUS: $(DEPDIR)/%.d
$(DEPDIR)/%.d: ;

-include $(CXXDEPS)
