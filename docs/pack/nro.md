# NRO Packaging Backend

The NRO packaging backend converts a compiled Nintendo Switch AArch64 ELF binary into an executable `.nro` bundle compatible with the Homebrew Menu.

---

## 1. Prerequisites & Toolchain Dependencies

The NRO packaging backend depends on the following switch-tools provided by devkitPro:

* `nacptool`: Used to generate Nintendo Application Control Property (`.nacp`) metadata binaries.
* `elf2nro`: Used to convert ELF binaries to `.nro` and embed NACP metadata, JPEG icons, and RomFS assets.

`nxdev doctor` verifies the presence and executable status of these tools automatically.

---

## 2. Manifest Configuration (`nxapp.yaml`)

NRO packaging options are configured under the `application`, `assets`, and `packaging` sections of `nxapp.yaml`:

```yaml
schemaVersion: 1

application:
  titleId: "0100000000000001"
  name: "My Switch Game"
  author: "Developer"
  version: "1.0.0"
  description: "An example Nintendo Switch homebrew application"

assets:
  icon: assets/icon.jpg      # Must be JPEG format (recommended 256x256)
  romfs: romfs/              # Directory with assets to embed into RomFS

packaging:
  defaultFormat: nro
  nro:
    enabled: true
```

---

## 3. Asset Processing Details

### Application Icon
* **Format Requirement**: Nintendo Switch Homebrew Menu requires JPEG image format (`0xFF 0xD8 0xFF` magic).
* **Validation**: NXDevPack inspects the file header. If a PNG or BMP image is passed, NXDev produces an actionable diagnostic explaining that JPEG format is required.
* **Default Fallback**: If `assets.icon` is not specified, NXDevPack automatically uses the default devkitPro icon at `$DEVKITPRO/libnx/default_icon.jpg`.

### RomFS Directory
* When `assets.romfs` is specified and exists, NXDevPack passes `--romfsdir=<path>` to `elf2nro`.
* Inside the Switch application, embedded assets can be accessed via `romfs:/` by calling `nxdev::filesystem::RomFS::mount()`.

---

## 4. Packaging Pipeline Steps

```
[1/4] Build Switch ELF        -> .nxdev/build/<profile>/bin/<target>.elf
[2/4] Generate NACP Metadata  -> .nxdev/package/nro/<profile>/app.nacp
[3/4] Resolve Assets          -> Validate icon & RomFS
[4/4] Invoke elf2nro          -> dist/<profile>/<safe_name>.nro
```

---

## 5. Verification & Safety Guarantees

* **ELF Validation**: Verifies that the binary is a valid 64-bit Little Endian AArch64 (`EM_AARCH64 = 183`) ELF before executing tools.
* **Header Magic Check**: Inspects byte offset `0x10` of the generated `.nro` to verify the ASCII sequence `NRO0`.
* **Atomic Staging**: Intermediate assets are staged in `.nxdev/package/nro/<profile>/` and only placed into `dist/<profile>/` once verification succeeds.
