# Getting Started with NXDev

Welcome to **NXDev**, the modern development SDK and toolchain ecosystem for Nintendo Switch homebrew applications.

---

## Prerequisites

Before starting, ensure your development host has the necessary toolchain and dependencies:

1. **devkitPro / devkitA64**:
   - Install devkitA64 and switch-tools following the [official devkitPro instructions](https://devkitpro.org/wiki/Getting_Started).
   - Ensure the environment variable `DEVKITPRO` is set (typically `/opt/devkitpro`).
2. **Build Tools**:
   - CMake 3.20 or newer
   - C++20 compatible compiler (GCC 11+, Clang 13+, or MSVC 2022)
   - Ninja or Make
3. **Optional (for VS Code integration)**:
   - VS Code 1.80+ with the **NXDevToolkit** extension (`.vsix`)
   - Node.js 18+ (if building the extension from source)

---

## 1. Verify Your Environment

Run the NXDev diagnostics command to verify your local toolchain and host configuration:

```bash
nxdev doctor
```

`nxdev doctor` inspects your `DEVKITPRO` environment, checks for `aarch64-none-elf-gcc`, `elf2nro`, `nacptool`, `nxlink`, and warns about any missing tools with actionable fix hints.

---

## 2. Create a New Project

Use `nxdev new` to create a new project from a curated template:

```bash
# List available templates
nxdev new --list

# Scaffold a project
nxdev new my-app --template console-cpp --author "Developer Name"
cd my-app
```

Available templates include:
- `minimal-c`: Pure C with raw libnx.
- `minimal-cpp`: Pure C++20 with raw libnx.
- `console-cpp`: Modern C++20 using `NXDev::Core` and `NXDev::Input`.
- `sdl2`: 2D graphics template with SDL2 dependency configured.
- `deko3d`: Modern Switch 3D graphics with deko3d (experimental).

---

## 3. Project Structure

Every NXDev project contains an `nxapp.yaml` manifest:

```yaml
schemaVersion: 1

application:
  id: "my-app"
  name: "My Switch App"
  author: "Developer Name"
  version: "0.1.0"
  titleId: "0100000000001000"

assets:
  icon: "assets/icon.png"
  romfs: "romfs"

dependencies:
  - "nxdev.core"
  - "nxdev.input"

build:
  standard: "c++20"
```

Validate your manifest at any time:

```bash
nxdev manifest validate
```

---

## 4. Install Dependencies

If your project specifies external devkitPro packages in `dependencies` (e.g. `nxdev.sdl2`):

```bash
nxdev package status
nxdev package install --missing
```

---

## 5. Configure & Build

Configure CMake and compile the target Switch ELF:

```bash
# Configure build tree
nxdev configure

# Build application ELF
nxdev build
```

The resulting ELF binary will be placed in `.nxdev/build/my-app.elf`.

---

## 6. Package as NRO

Package the compiled ELF into a Homebrew Menu compatible `.nro` binary with embedded NACP control metadata, icon, and RomFS:

```bash
nxdev pack nro
```

The packaged artifact is output to:
```text
dist/
└── release/
    └── my-app.nro
```

---

## 7. Deploy & Run on Switch

### Method A: nxlink (Wireless LAN)

Ensure your Switch is running the Homebrew Menu with `nxlink` listening (press `Y` in hbmenu on NetLoader):

```bash
# Discover or set default device IP
nxdev devices list
nxdev config set device.ip 192.168.1.100

# Deploy and stream logs
nxdev run --ip 192.168.1.100
```

`nxdev run` will automatically:
1. Rebuild and repackage if source files changed.
2. Send the `.nro` over `nxlink`.
3. Capture and stream standard output, error, and runtime crash logs.

### Method B: SD Card Manual Copy

Copy `dist/release/my-app.nro` to your Switch SD card at:
```text
sdmc:/switch/my-app/my-app.nro
```

---

## 8. Next Steps

- Explore the [NXDevSDK Reference](../sdk/README.md) for Core, Input, Filesystem, Account, Network, Time, and Power APIs.
- Consult the [NXDevAppManifest Specification](../manifest/README.md) for all manifest configuration options.
- Read [WSL Workflow Guide](../wsl.md) if developing from Windows via WSL2.
