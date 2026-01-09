# IronNet - Pure C89 CLI Interpreter

IronNet is a pure C89 implementation of a Common Language Infrastructure (CLI) interpreter. It reads compiled .NET assemblies (PE/COFF format with CLI metadata) and interprets their Intermediate Language (IL) code.

## Features

- **Pure C89**: No C99 or later features, maximum portability
- **Multi-platform**: Windows, Linux, macOS, BSD, and bare-metal support
- **Multi-architecture**: x86, x64, ARM, ARM64, MIPS, PowerPC, RISC-V
- **Multi-endianness**: Little-endian and big-endian support
- **Word sizes**: 16, 24, 31, 32, and 64-bit support
- **Full CLI metadata**: Complete ECMA-335 metadata parsing
- **Generics support**: Generic types and methods
- **Multi-threading**: Thread support with synchronization primitives
- **Custom corlib**: Built-in Base Class Library with InternalCall support
- **Garbage Collection**: Simple mark-and-sweep GC

## Project Structure

```
IronNet/
├── include/iron/          # Public headers
│   ├── platform.h         # Platform detection and types
│   ├── types.h            # Core type definitions
│   ├── memory.h           # Memory management (allocators, containers)
│   ├── vtable.h           # Runtime object system with vtables
│   ├── pe.h               # PE/COFF file format
│   ├── metadata.h         # CLI metadata reader
│   ├── opcodes.h          # CIL opcode definitions
│   ├── runtime.h          # Runtime type system
│   ├── exec.h             # IL interpreter
│   ├── gc.h               # Garbage collector
│   ├── thread.h           # Threading primitives
│   └── iron.h             # Main header (includes all)
├── src/
│   ├── core/              # Core infrastructure
│   │   ├── platform.c
│   │   ├── memory.c
│   │   └── types.c
│   ├── pe/                # PE/COFF reader
│   │   └── pe.c
│   ├── metadata/          # CLI metadata
│   │   └── metadata.c
│   ├── runtime/           # Runtime system
│   │   ├── opcodes.c
│   │   ├── exec.c
│   │   └── runtime.c
│   ├── thread/            # Threading
│   │   └── thread.c
│   ├── gc/                # Garbage collector
│   │   └── gc.c
│   └── iron.c             # Main entry point
├── corlib/                # Custom Base Class Library (future)
├── tests/                 # Unit tests
├── Makefile
└── README.md
```

## Building

### Prerequisites

- C89-compatible compiler (GCC, Clang, MSVC)
- Make (GNU Make recommended)

### Build Commands

```bash
# Build static library
make

# Build with debug symbols
make DEBUG=1

# Build and run tests
make test

# Clean build artifacts
make clean

# Install to system (default: /usr/local)
make install

# Install to custom prefix
make install PREFIX=/opt/ironnet
```

## Usage

### As a Library

```c
#include <iron/iron.h>

int main(int argc, char **argv) {
    int exit_code;
    iron_result_t result;
    
    /* Initialize runtime */
    result = iron_init();
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Failed to initialize: %s\n", result.message);
        return 1;
    }
    
    /* Run assembly */
    result = iron_run_assembly("MyApp.exe", argc - 1, (const char**)(argv + 1), &exit_code);
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Execution failed: %s\n", result.message);
        iron_shutdown();
        return 1;
    }
    
    iron_shutdown();
    return exit_code;
}
```

### Low-Level API

```c
#include <iron/iron.h>

int main(void) {
    iron_domain_t *domain;
    iron_assembly_t *assembly;
    iron_exec_context_t *ctx;
    iron_domain_config_t config = {0};
    
    iron_init();
    
    /* Create application domain */
    config.name = "MyDomain";
    iron_domain_create(&domain, &config, NULL);
    
    /* Load assembly */
    iron_domain_load_assembly(domain, "MyApp.exe", &assembly);
    
    /* Create execution context */
    iron_exec_create(&ctx, domain);
    
    /* Register internal calls */
    iron_register_corlib(ctx);
    
    /* Execute... */
    
    /* Cleanup */
    iron_exec_destroy(ctx);
    iron_domain_destroy(domain);
    iron_shutdown();
    
    return 0;
}
```

## Architecture

### Memory Management

IronNet uses a vtable-based polymorphic allocator system:

- **System Allocator**: Standard malloc/free wrapper
- **Arena Allocator**: Fast bump allocation with bulk free
- **Pool Allocator**: Fixed-size object allocation

### Runtime Type System

Types are represented at runtime with full metadata:

- `iron_runtime_type_t`: Type definitions with fields, methods, vtables
- `iron_runtime_method_t`: Method definitions with IL bodies
- `iron_runtime_field_t`: Field definitions with layout info

### Execution Model

- **Stack-based**: Evaluation stack per thread
- **Frame-based**: Call stack with locals and arguments
- **Exception handling**: Try/catch/finally support

### Threading

Platform-independent threading primitives:

- Threads, mutexes, condition variables
- Read-write locks, semaphores, events
- Atomic operations
- Thread-local storage

## Supported CLI Features

### Implemented
- [x] PE/COFF file parsing
- [x] CLI metadata reading
- [x] Basic IL interpretation
- [x] Arithmetic and logic opcodes
- [x] Branch instructions
- [x] Local variables and arguments
- [x] Basic GC

### In Progress
- [ ] Full opcode support
- [ ] Type resolution
- [ ] Method dispatch (virtual, interface)
- [ ] Exception handling
- [ ] Generics instantiation

### Planned
- [ ] Complete corlib implementation
- [ ] P/Invoke support
- [ ] Reflection
- [ ] JIT compilation (optional)

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]
