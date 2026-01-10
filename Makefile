# IronNet CLR Interpreter
# Makefile for building the interpreter

CC ?= gcc
AR ?= ar

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
LIBDIR = lib
BINDIR = bin

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -Werror -I$(INCDIR)

# Debug/Release
ifdef DEBUG
    CFLAGS += -g -O0 -DIRON_DEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lpthread -lm
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -lpthread -lm
endif

# Source files
CORE_SRCS = \
    $(SRCDIR)/core/platform.c \
    $(SRCDIR)/core/memory.c \
    $(SRCDIR)/core/types.c \
    $(SRCDIR)/core/debug.c

PE_SRCS = \
    $(SRCDIR)/pe/pe.c

METADATA_SRCS = \
    $(SRCDIR)/metadata/metadata.c

RUNTIME_SRCS = \
    $(SRCDIR)/runtime/opcodes.c \
    $(SRCDIR)/runtime/opcode_table.c \
    $(SRCDIR)/runtime/exec.c \
    $(SRCDIR)/runtime/runtime.c

THREAD_SRCS = \
    $(SRCDIR)/thread/thread.c

GC_SRCS = \
    $(SRCDIR)/gc/gc.c

CORLIB_SRCS = \
    $(SRCDIR)/corlib/string.c \
    $(SRCDIR)/corlib/object.c \
    $(SRCDIR)/corlib/console.c \
    $(SRCDIR)/corlib/math.c \
    $(SRCDIR)/corlib/int32.c \
    $(SRCDIR)/corlib/environment.c \
    $(SRCDIR)/corlib/array.c \
    $(SRCDIR)/corlib/corlib_main.c

DISASM_SRCS = \
    $(SRCDIR)/disasm/disasm.c

MAIN_SRCS = \
    $(SRCDIR)/iron.c

ALL_SRCS = $(CORE_SRCS) $(PE_SRCS) $(METADATA_SRCS) $(RUNTIME_SRCS) $(THREAD_SRCS) $(GC_SRCS) $(CORLIB_SRCS) $(DISASM_SRCS) $(MAIN_SRCS)

# Object files
OBJS = $(ALL_SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Library name
LIBNAME = libiron
STATIC_LIB = $(LIBDIR)/$(LIBNAME).a

# Main executable
MAIN_EXE = $(BINDIR)/ironnet

# Test executable
TEST_EXE = $(BINDIR)/test_iron

# CLI executable source
CLI_SRC = $(SRCDIR)/main.c

# Default target
all: dirs $(STATIC_LIB) $(MAIN_EXE)

# Create directories
dirs:
	@mkdir -p $(BUILDDIR)/core
	@mkdir -p $(BUILDDIR)/pe
	@mkdir -p $(BUILDDIR)/metadata
	@mkdir -p $(BUILDDIR)/runtime
	@mkdir -p $(BUILDDIR)/thread
	@mkdir -p $(BUILDDIR)/gc
	@mkdir -p $(BUILDDIR)/corlib
	@mkdir -p $(BUILDDIR)/disasm
	@mkdir -p $(LIBDIR)
	@mkdir -p $(BINDIR)

# Static library
$(STATIC_LIB): $(OBJS)
	$(AR) rcs $@ $^
	@echo "Built static library: $@"

# Object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Main executable
$(MAIN_EXE): $(STATIC_LIB) $(CLI_SRC)
	$(CC) $(CFLAGS) $(CLI_SRC) -L$(LIBDIR) -liron $(LDFLAGS) -o $(MAIN_EXE)
	@echo "Built executable: $(MAIN_EXE)"

# Test executable
test: dirs $(STATIC_LIB) tests/test_main.c
	$(CC) $(CFLAGS) tests/test_main.c -L$(LIBDIR) -liron $(LDFLAGS) -o $(TEST_EXE)
	@echo "Built test executable: $(TEST_EXE)"
	$(TEST_EXE)

# Build corlib (C# BCL)
corlib: $(MAIN_EXE)
	@echo "=== Building corlib ==="
	$(MAKE) -C corlib

# Build examples
examples: corlib
	@echo "=== Building examples ==="
	$(MAKE) -C examples

# Full build including corlib and examples
full: all corlib examples
	@echo "=== Full build complete ==="

# Clean
clean:
	rm -rf $(BUILDDIR) $(LIBDIR) $(BINDIR)
	$(MAKE) -C corlib clean || true
	$(MAKE) -C examples clean || true

# Install (to /usr/local by default)
PREFIX ?= /usr/local
install: $(STATIC_LIB)
	install -d $(PREFIX)/lib
	install -d $(PREFIX)/include/iron
	install -m 644 $(STATIC_LIB) $(PREFIX)/lib/
	install -m 644 $(INCDIR)/iron/*.h $(PREFIX)/include/iron/

# Uninstall
uninstall:
	rm -f $(PREFIX)/lib/$(LIBNAME).a
	rm -rf $(PREFIX)/include/iron

# Dependencies
.PHONY: all dirs clean install uninstall test corlib examples full

# Header dependencies
$(BUILDDIR)/core/platform.o: $(INCDIR)/iron/platform.h
$(BUILDDIR)/core/memory.o: $(INCDIR)/iron/memory.h $(INCDIR)/iron/platform.h
$(BUILDDIR)/core/types.o: $(INCDIR)/iron/types.h $(INCDIR)/iron/platform.h
$(BUILDDIR)/pe/pe.o: $(INCDIR)/iron/pe.h $(INCDIR)/iron/platform.h $(INCDIR)/iron/types.h
$(BUILDDIR)/metadata/metadata.o: $(INCDIR)/iron/metadata.h $(INCDIR)/iron/pe.h
$(BUILDDIR)/runtime/opcodes.o: $(INCDIR)/iron/opcodes.h $(INCDIR)/iron/platform.h
$(BUILDDIR)/runtime/exec.o: $(INCDIR)/iron/exec.h $(INCDIR)/iron/runtime.h
$(BUILDDIR)/runtime/runtime.o: $(INCDIR)/iron/runtime.h $(INCDIR)/iron/metadata.h
$(BUILDDIR)/thread/thread.o: $(INCDIR)/iron/thread.h $(INCDIR)/iron/platform.h
$(BUILDDIR)/gc/gc.o: $(INCDIR)/iron/gc.h $(INCDIR)/iron/exec.h
$(BUILDDIR)/iron.o: $(INCDIR)/iron/iron.h
