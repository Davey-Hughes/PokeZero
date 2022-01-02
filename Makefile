TARGET := pokezero
BUILD := ./build
OBJDIR := $(BUILD)/objects
BINDIR := $(BUILD)/bin
DEPDIR := $(OBJDIR)/.deps

BASE_FLAGS := -Wall -Werror -Wextra -pedantic-errors
SANITIZE := -fsanitize=address -fsanitize=undefined -fsanitize=leak \
	-fno-sanitize-recover=all -fsanitize=float-divide-by-zero \
	-fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment

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

.PHONY: all debug release clean run showdown

all: debug $(GIT_SUBMODULES) showdown

# running PokeEnv
RUN_ENV :=
RUN_ARGS :=
ifneq ($(OS), Windows_NT)
	UNAME_S := $(shell uname -s)
	# macos specific variables
	ifeq ($(UNAME_S), Darwin)
		RUN_ENV += MallocNanoZone=0
	endif
endif
run: PokeZero showdown
	$(RUN_ENV) ./$(TARGET)

# updating git submodules
GIT=git
GIT_SUBMODULES = $(shell sed -nE 's/path = +(.+)/\1\/.git/ p' .gitmodules | paste -s -)
$(GIT_SUBMODULES): %/.git: .gitmodules
	$(GIT) submodule update --init $*
	@touch $@

# building the showdown source code
SHOWDOWN_DIR := ./pokemon-showdown
showdown: $(GIT_SUBMODULES)
	$(SHOWDOWN_DIR)/build

PokeZero: $(CXXTARGET)
	ln -sf $(BINDIR)/$(CXXTARGET) $(TARGET)

debug: CXXFLAGS += -DDEBUG -g -Og
debug: LDLIBS += $(SANITIZE)
debug: PokeZero

release: CXXFLAGS += -O3 -march=native
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
