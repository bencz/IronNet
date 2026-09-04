# IronNet Examples Build
# Single Makefile for all test examples

# Find dotnet SDK path
SDK_VERSION := $(shell dotnet --version 2>/dev/null)
SDK_PATH := $(shell dotnet --list-sdks | grep "$(SDK_VERSION)" | sed 's/.*\[\(.*\)\]/\1/')
CSC := dotnet $(SDK_PATH)/$(SDK_VERSION)/Roslyn/bincore/csc.dll

CORLIB := ../bin/corlib.dll
BINDIR := ../bin
IRONNET := ../bin/ironnet

# Source files
SOURCES := $(wildcard *.cs)
TARGETS := $(SOURCES:%.cs=$(BINDIR)/%.exe)

# Common compiler flags
CSCFLAGS := -nostdlib -noconfig -target:exe -reference:$(CORLIB) \
            -langversion:latest -nullable:disable -warn:4 -warnaserror+

.PHONY: all clean run-all help

all: $(TARGETS)
	@echo "=== All examples built ==="
	@echo "Available targets:"
	@for exe in $(TARGETS); do echo "  $$exe"; done

$(BINDIR)/%.exe: %.cs $(CORLIB)
	@echo "Building $<..."
	@$(CSC) $(CSCFLAGS) -out:$@ $<

clean:
	rm -f $(TARGETS)

# Run targets
run-hello: $(BINDIR)/HelloWorld.exe
	$(IRONNET) --corlib $(CORLIB) $<

run-array: $(BINDIR)/ArrayTest.exe
	$(IRONNET) --corlib $(CORLIB) $<

run-math: $(BINDIR)/MathTest.exe
	$(IRONNET) --corlib $(CORLIB) $<

run-loop: $(BINDIR)/LoopTest.exe
	$(IRONNET) --corlib $(CORLIB) $<

run-method: $(BINDIR)/MethodTest.exe
	$(IRONNET) --corlib $(CORLIB) $<

run-gc: $(BINDIR)/GCTest.exe
	$(IRONNET) --corlib $(CORLIB) $<

run-cond: $(BINDIR)/ConditionalTest.exe
	$(IRONNET) --corlib $(CORLIB) $<

# Run with debug
debug-hello: $(BINDIR)/HelloWorld.exe
	$(IRONNET) --debug --corlib $(CORLIB) $<

debug-trace: $(BINDIR)/HelloWorld.exe
	$(IRONNET) --debug-level trace --corlib $(CORLIB) $<

debug-gc: $(BINDIR)/GCTest.exe
	$(IRONNET) --debug-level debug --debug-components gc --corlib $(CORLIB) $<

debug-exec: $(BINDIR)/LoopTest.exe
	$(IRONNET) --debug-level trace --debug-components exec --corlib $(CORLIB) $<

# Run all tests
run-all: $(TARGETS)
	@echo "=== Running all tests ==="
	@for exe in $(TARGETS); do \
		echo ""; \
		echo ">>> Running $$exe"; \
		$(IRONNET) --corlib $(CORLIB) $$exe; \
	done
	@echo ""
	@echo "=== All tests completed ==="

help:
	@echo "IronNet Examples Makefile"
	@echo ""
	@echo "Build targets:"
	@echo "  all          - Build all examples"
	@echo "  clean        - Remove built executables"
	@echo ""
	@echo "Run targets:"
	@echo "  run-hello    - Run HelloWorld"
	@echo "  run-array    - Run ArrayTest"
	@echo "  run-math     - Run MathTest"
	@echo "  run-loop     - Run LoopTest"
	@echo "  run-method   - Run MethodTest"
	@echo "  run-gc       - Run GCTest"
	@echo "  run-cond     - Run ConditionalTest"
	@echo "  run-all      - Run all tests"
	@echo ""
	@echo "Debug targets:"
	@echo "  debug-hello  - Run HelloWorld with --debug"
	@echo "  debug-trace  - Run HelloWorld with --debug-level trace"
	@echo "  debug-gc     - Run GCTest with GC debug output"
	@echo "  debug-exec   - Run LoopTest with execution trace"
