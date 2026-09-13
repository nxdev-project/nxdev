# Building Nintendo Switch Applications with NXDev

NXDev provides a streamlined, manifest-aware build orchestration system on top of CMake and the devkitA64 GNU cross-compiler toolchain.

---

## Build Architecture

```text
nxapp.yaml (Application Manifest)
    │
    ▼
NXDev Project & Build Orchestrator
    │
    ▼
CMake (with NXDevSwitch.cmake toolchain & NXDevConfig.cmake)
    │
    ▼
devkitA64 GCC (aarch64-none-elf-gcc / g++)
    │
    ▼
libnx (switch.specs / switch.ld / libnx.a)
    │
    ▼
Position-Independent ELF Binary (.nxdev/build/<profile>/bin/<app>.elf)
```

---

## CLI Build Commands

### 1. `nxdev configure`

Configures the project's CMake build workspace without executing compilation.

```bash
# Configure default profile (debug)
nxdev configure

# Configure release profile
nxdev configure --profile release

# Regenerate build directory from scratch
nxdev configure --fresh

# Output result as machine-readable JSON
nxdev configure --json
```

### 2. `nxdev build`

Configures (if necessary) and compiles the project into a Nintendo Switch ELF binary.

```bash
# Build default profile (debug)
nxdev build

# Build with explicit profile
nxdev build --profile release

# Clean prior build artifacts before building
nxdev build --clean

# Verbose compiler invocation logging
nxdev build --verbose

# Specify parallel compiler worker count
nxdev build --jobs 8

# Output structured build result JSON (ideal for IDEs / CI)
nxdev build --json
```

### 3. `nxdev clean`

Safely removes the project's `.nxdev/build` workspace while protecting repository source files.

```bash
nxdev clean
```

---

## Build Output Directory Layout

```text
<project-root>/
├── nxapp.yaml
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── .nxdev/
    ├── compile_commands.json           <- Synced compilation database for IDEs
    └── build/
        ├── debug/
        │   ├── bin/
        │   │   └── <target>.elf        <- Switch homebrew ELF binary
        │   ├── build-info.json         <- Machine-readable build metadata
        │   └── compile_commands.json
        └── release/
            ├── bin/
            │   └── <target>.elf
            └── build-info.json
```

---

## Build Profiles

| Profile | CMake Type | Compiler Flags | Optimization | Debug Symbols |
|---|---|---|---|---|
| `debug` | `Debug` | `-g -O0` | Disabled | Enabled (`-g`) |
| `release` | `Release` | `-O3 -DNDEBUG` | Maximum (`-O3`) | Stripped/Disabled |

---

## Target Binary Verification

The generated `.elf` binary is a 64-bit ARM AArch64 Position-Independent Executable (`ET_DYN` with `-pie` and `-specs=switch.specs`) ready for subsequent packaging into Homebrew Menu `.nro` format.
