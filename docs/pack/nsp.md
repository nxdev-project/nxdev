# NSP Packaging Guide

`nxdev pack nsp` packages a compiled Nintendo Switch AArch64 ELF binary into an installable Nintendo Submission Package (`.nsp`).

---

## 1. Prerequisites

NSP packaging requires the following:

1. **A valid Title ID** configured in `nxapp.yaml` (`application.titleId: "0100000000000088"`).
2. **A user-provided Switch key file** (`prod.keys`).
3. **Switch build tools**:
   - `elf2nso` (devkitPro switch-tools)
   - `npdmtool` (devkitPro switch-tools)
   - `nacptool` (devkitPro switch-tools)
   - `hacbrewpack` (bundled host tool built from `third_party/hacbrewpack`)

---

## 2. Configuration (`nxapp.yaml`)

```yaml
schemaVersion: 1
target: switch

application:
  id: com.example.nspdemo
  name: "NSP Packaging Demo"
  author: "NXDev Team"
  version: "1.0.0"
  titleId: "0100000000000088"

npdm:
  preset: standard  # standard | network | filesystem | multimedia | advanced
  mainThread:
    priority: 44
    core: 0
    stackSize: 262144
  services:
    - "sm:"
    - "bsds:u"
    - "acc:u"

assets:
  icon: assets/icon.jpg
  romfs: romfs/
```

---

## 3. CLI Usage

```bash
# Package with explicit key file
nxdev pack nsp --keys /path/to/prod.keys

# Package using NXDEV_KEYS environment variable
export NXDEV_KEYS=/path/to/prod.keys
nxdev pack nsp

# Package with Release profile and custom output
nxdev pack nsp --profile release -o out/MyApp.nsp

# Validate configuration without keys or building
nxdev pack nsp --dry-run
```

---

## 4. Staging Workspace Architecture

NXDev creates an isolated staging workspace under `.nxdev/package/nsp/<profile>/`:

```text
.nxdev/package/nsp/debug/
├── generated/
│   └── npdm.json         <- Generated NPDM descriptor
├── staging/
│   ├── exefs/
│   │   ├── main          <- NSO binary compiled via elf2nso
│   │   └── main.npdm     <- NPDM binary compiled via npdmtool
│   ├── control/
│   │   ├── control.nacp  <- NACP metadata compiled via nacptool
│   │   └── icon_AmericanEnglish.dat <- Staged 256x256 JPEG icon
│   └── romfs/            <- Staged RomFS filesystem assets
└── output/
    └── 0100000000000088.nsp <- hacBrewPack assembled package
```

The validated output is atomically published to `dist/<profile>/<application-name>.nsp`.
