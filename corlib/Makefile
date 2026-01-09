# IronNet Corlib Build
# Compiles the C# BCL implementation to corlib.dll

# Find the SDK path dynamically
SDK_VERSION := $(shell dotnet --version)
SDK_PATH := /usr/local/share/dotnet/sdk/$(SDK_VERSION)
CSC := dotnet $(SDK_PATH)/Roslyn/bincore/csc.dll

# Directories
BUILD_DIR := ../bin
CORLIB_DIR := .

# Source files - recursively find all .cs files
SOURCES := $(shell find $(CORLIB_DIR) -name "*.cs" -type f)

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
	@echo "SDK: $(SDK_PATH)"
	@echo "Sources: $(words $(SOURCES)) files"
	$(CSC) -nostdlib -noconfig \
		-target:library \
		-out:$(OUTPUT) \
		-langversion:latest \
		-nullable:disable \
		-unsafe+ \
		-warn:0 \
		$(SOURCES)

clean:
	rm -f $(OUTPUT)

# Debug target to show variables
debug:
	@echo "SDK_VERSION: $(SDK_VERSION)"
	@echo "SDK_PATH: $(SDK_PATH)"
	@echo "CSC: $(CSC)"
	@echo "SOURCES: $(SOURCES)"
