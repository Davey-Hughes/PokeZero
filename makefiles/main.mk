DEPDIR := $(OBJDIR)/objects/.deps

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
LDLIBS := -lpthread
# flags required for dependency generation; passed to compilers
DEPFLAGS = -MQ $@ -MD -MP -MF $(DEPDIR)/$*.Td

CXXFLAGS := -std=c++20 $(BASE_FLAGS)
SYSINCLUDE :=
CXXINCLUDE := -Iinclude $(shell pkg-config nlohmann_json --cflags)
CXXSRC := $(wildcard src/*.cc)
CXXINC := $(wildcard include/*.hh)
CXXOBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(CXXSRC)))
CXXDEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(CXXSRC)))

# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CXXINCLUDE) $(SYSINCLUDE) -c -o $@
# link object files to binary
LINK.o = $(LD) $(LDFLAGS) $(LDLIBS) -o $@
# precompile step
PRECOMPILE =
# postcompile step
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

.PHONY: all valgrind debug release showdown format run

all: format debug $(GIT_SUBMODULES) showdown

# run PokeZero
RUN_ENV :=
RUN_ARGS :=
ifneq ($(OS), Windows_NT)
	UNAME_S := $(shell uname -s)
	# macos specific variables
	ifeq ($(UNAME_S), Darwin)
		RUN_ENV += MallocNanoZone=0
	endif
endif

ifeq (,$(wildcard $(TARGET)))
run:
	$(error Build executable first with `make debug` or `make release`)
else
run:
	$(RUN_ENV) ./$(TARGET)
endif

# format all source and header files
format: $(CXXSRC)
	clang-format -i $(CXXSRC) $(CXXINC)

# debug flags
debug: CXXFLAGS += -glldb -O0 -DDEBUG
debug: LDLIBS += $(SANITIZE)
debug: $(TARGET)
	ln -sf $(CXXTARGET) $(TARGET)

# release flags
release: CXXFLAGS += -O3 -march=native
release: $(TARGET)
	ln -sf $(CXXTARGET) $(TARGET)

# valgrind flags
valgrind: CXXFLAGS += -O3 -march=native -mno-avx512f
valgrind: $(TARGET)
	ln -sf $(CXXTARGET) $(TARGET)

$(TARGET): $(CXXTARGET)

# run the linker to create the executable
$(CXXTARGET): $(CXXOBJS)
	@mkdir -p $(BINDIR)
	$(LINK.o) $^

# compile object files from every source file
$(OBJDIR)/%.o: %.cc
$(OBJDIR)/%.o: %.cc | $(DEPDIR)/%.d
	$(shell mkdir -p $(dir $(CXXOBJS)) >/dev/null)
	$(shell mkdir -p $(dir $(CXXDEPS)) >/dev/null)
	$(PRECOMPILE)
	$(COMPILE.cc) $<
	$(POSTCOMPILE)

.PRECIOUS: $(DEPDIR)/%.d
$(DEPDIR)/%.d: ;

-include $(CXXDEPS)
