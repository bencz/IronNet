# HelloWorld Example Build

SDK_VERSION := $(shell dotnet --version)
SDK_PATH := /usr/local/share/dotnet/sdk/$(SDK_VERSION)
CSC := dotnet $(SDK_PATH)/Roslyn/bincore/csc.dll

CORLIB := ../../bin/corlib.dll
OUTPUT := ../../bin/HelloWorld.exe

all: $(OUTPUT)
	@echo "=== HelloWorld built ==="

$(OUTPUT): HelloWorld.cs $(CORLIB)
	$(CSC) -nostdlib -noconfig \
		-target:exe \
		-out:$(OUTPUT) \
		-reference:$(CORLIB) \
		-langversion:latest \
		-nullable:disable \
		-warn:0 \
		HelloWorld.cs

clean:
	rm -f $(OUTPUT)

run: $(OUTPUT)
	../../bin/ironnet --corlib $(CORLIB) $(OUTPUT)
