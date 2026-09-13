# Assets & Path Resolution

The `assets` section specifies icon and RomFS filesystem paths.

## Project-Relative Path Rules

> [!IMPORTANT]
> All paths in `nxapp.yaml` are **strictly resolved relative to the directory containing `nxapp.yaml`**, never relative to the shell's current working directory.

## Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `icon` | `string` | Path to a 256x256 JPEG image (`.jpg` or `.jpeg`). |
| `romfs` | `string` | Path to the directory containing RomFS assets. |

## Icon Validation & Default Fallback

1. **Extension Check**: Must end in `.jpg` or `.jpeg`.
2. **File Existence**: Must exist at the resolved project path.
3. **Magic Byte Signature**: The file header is validated for JPEG magic bytes (`0xFF 0xD8 0xFF`).
4. **Fallback**: If `assets.icon` is omitted, the normalized model represents this as `IconSourceType::LibnxDefault`. NXDev will automatically use the default homebrew icon provided by libnx without hardcoding toolchain paths in the manifest.

## RomFS Directory Validation

The RomFS path must point to an existing **directory** (not a regular file). If omitted, RomFS embedding is disabled (`assets.romfs.enabled = false`).
