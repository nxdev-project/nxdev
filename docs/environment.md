# NXDev Environment Discovery & Resolution

NXDev includes an intelligent, non-destructive environment detection engine that discovers compilers, system libraries, and Switch homebrew tools without modifying user configuration or altering shell profiles.

---

## Toolchain Precedence Model

When discovering devkitPro and toolchain binaries, NXDev evaluates candidates in the following deterministic order:

```text
1. CLI Arguments          (--devkitpro / --devkita64)
2. Project-Local Config   (.nxdev/local.yaml)
3. User-Global Config     (~/.config/nxdev/config.yaml or %APPDATA%/nxdev/config.yaml)
4. Environment Variables  (DEVKITPRO, DEVKITA64)
5. Standard Layout        (/opt/devkitpro on Linux/WSL or standard Windows paths)
6. System PATH Lookup     (for CMake, Git, Make, Ninja, and individual tools)
```

---

## Discovered Tool Categories

NXDev categorizes discovered tools by role:

### 1. Core Compiler Toolchain
- `aarch64-none-elf-gcc`: GNU C compiler for AArch64 target
- `aarch64-none-elf-g++`: GNU C++ compiler for AArch64 target
- `aarch64-none-elf-ar`: Target archive utility
- `aarch64-none-elf-objcopy`: Object copying and binary translation utility

### 2. Switch Libraries & Headers
- `libnx`: Standard Nintendo Switch homebrew operating system library (`switch.h`, `libnx.a`)

### 3. Packaging Tools
- `elf2nro`: Converts ELF executables into Nintendo Relocatable Object (`.nro`) homebrew binaries
- `nacptool`: Generates Nintendo Application Control Property (`.nacp`) metadata

### 4. Deployment Tools
- `nxlink`: Wireless network deployment client for sending NRO packages to Switch homebrew loaders

### 5. Build Systems & VCS
- `cmake`: CMake build system generator
- `ninja` / `make`: Native build executors
- `git`: Version control system

---

## Tool Execution Safety

All tool probes in NXDev adhere to strict safety practices:
- **No shell injection**: Executables are invoked with explicit argument arrays, never raw concatenated shell strings.
- **Strict timeouts**: Probes enforce safe execution timeouts (1–2 seconds) to prevent hanging.
- **Space and Unicode handling**: Full support for directory paths containing spaces and non-ASCII characters.
- **Non-destructive inspection**: Probes only execute safe informational commands (e.g. `--version`, `-h`).
