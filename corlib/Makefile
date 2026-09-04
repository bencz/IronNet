# IronNet Corlib Build
# Compiles the C# BCL implementation to corlib.dll

# Find dotnet SDK path
SDK_VERSION := $(shell dotnet --version 2>/dev/null)
SDK_PATH := $(shell dotnet --list-sdks | grep "$(SDK_VERSION)" | sed 's/.*\[\(.*\)\]/\1/')
CSC := dotnet $(SDK_PATH)/$(SDK_VERSION)/Roslyn/bincore/csc.dll
CSCFLAGS := -nostdlib -noconfig -target:library -langversion:latest -nullable:disable -unsafe+ -warn:4 -warnaserror+

# Directories
BUILD_DIR := ../bin
CORLIB_DIR := .

# Source files - recursively find all .cs files (exclude obj/ directory)
SOURCES := $(shell find $(CORLIB_DIR) -path "$(CORLIB_DIR)/obj" -prune -o -name "*.cs" -type f -print)

# Output
OUTPUT := $(BUILD_DIR)/corlib.dll

.PHONY: all clean

all: $(BUILD_DIR) $(OUTPUT)
	@echo "=== Corlib built successfully ==="
	@echo "Output: $(OUTPUT)"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OUTPUT): $(SOURCES)
	@echo "=== Building corlib ==="
	@echo "CSC: $(CSC)"
	@echo "Sources: $(words $(SOURCES)) files"
	$(CSC) -nostdlib -noconfig \
		-target:library \
		-out:$(OUTPUT) \
		-langversion:latest \
		-nullable:disable \
		-unsafe+ \
		-warn:4 \
		-warnaserror+ \
		$(SOURCES)

clean:
	rm -f $(OUTPUT)

# Debug target to show variables
debug:
	@echo "SDK_VERSION: $(SDK_VERSION)"
	@echo "SDK_PATH: $(SDK_PATH)"
	@echo "CSC: $(CSC)"
	@echo "SOURCES: $(SOURCES)"
