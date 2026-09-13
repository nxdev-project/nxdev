# Packaging Troubleshooting Guide

Common issues encountered when packaging Nintendo Switch binaries and how to resolve them.

---

## 1. `InvalidIcon`: Icon File Format Mismatch

### Problem
```text
Error: Icon file '/path/to/icon.png' is in PNG format. Nintendo Switch requires a JPEG icon image.
```

### Cause
The Nintendo Switch Homebrew Menu and `elf2nro` require JPEG images with the `0xFF 0xD8 0xFF` magic signature.

### Solution
Convert your icon to a 256x256 JPEG:
```bash
# Using ImageMagick
magick assets/icon.png -resize 256x256 assets/icon.jpg
```
Update `nxapp.yaml`:
```yaml
assets:
  icon: assets/icon.jpg
```
Or omit `assets.icon` to use the built-in devkitPro default icon.

---

## 2. `InvalidInputBinary`: Not a 64-bit ARM ELF

### Problem
```text
Error: ELF binary architecture is 62 (x86_64), expected 183 (EM_AARCH64).
```

### Cause
The input ELF was compiled for host x86_64 instead of the Nintendo Switch target (`aarch64-none-elf`).

### Solution
Build targeting Nintendo Switch using the `nxdev` CLI:
```bash
nxdev build --profile debug
nxdev pack nro
```

---

## 3. `ToolNotFound`: Missing `nacptool` or `elf2nro`

### Problem
```text
Error: nacptool executable not found in PATH or devkitPro switch-tools.
```

### Cause
The `switch-tools` package is not installed or `DEVKITPRO` environment variable is not configured.

### Solution
Run `nxdev doctor` to inspect your environment:
```bash
nxdev doctor
```
Install switch-tools:
```bash
sudo dkp-pacman -S switch-tools
```

---

## 4. `InvalidRomFS`: RomFS Directory Missing

### Problem
```text
Error: Configured RomFS path does not exist or is not a directory.
```

### Cause
`nxapp.yaml` specifies `assets.romfs`, but the directory has not been created.

### Solution
Create the directory or disable RomFS in `nxapp.yaml`:
```bash
mkdir -p romfs
```
