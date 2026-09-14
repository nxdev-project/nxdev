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

# Retain staging and backend directories on success or failure for inspection
nxdev pack nsp --keep-staging

# Enable full verbose output including backend execution details
nxdev pack nsp --debug-backend --verbose

# Validate configuration without keys or building
nxdev pack nsp --dry-run
```

---

## 4. Staging Workspace Architecture

NXDev creates an isolated, structured staging workspace under `.nxdev/package/nsp/<profile>/`:

```text
.nxdev/package/nsp/debug/
├── generated/
│   └── npdm.json              <- Generated NPDM descriptor
├── staging/
│   ├── exefs/
│   │   ├── main               <- NSO binary compiled via elf2nso
│   │   └── main.npdm          <- NPDM binary compiled via npdmtool
│   ├── control/
│   │   ├── control.nacp       <- NACP metadata compiled via nacptool
│   │   └── icon_AmericanEnglish.dat <- Staged 256x256 JPEG icon
│   └── romfs/                 <- Staged RomFS filesystem assets
├── backend/
│   ├── temp/                  <- hacBrewPack temporary scratch workspace
│   ├── nca/                   <- Intermediate NCA containers
│   └── nsp/                   <- Generated NSP output directory
└── logs/
    ├── hacbrewpack.log        <- Persistent per-run backend execution log
    └── hacbrewpack-latest.log <- Latest execution log link
```

### Execution & Publishing Workflow
1. **Preflight Validation**: Validates all staged ExeFS, NPDM, NACP, and Icon inputs before invoking the backend.
2. **Backend Execution**: Executes `hacbrewpack` in the isolated workspace with key paths sanitized in logs.
3. **PFS0 Binary Validation**: Verifies the `PFS0` magic header and structural integrity of the generated `.nsp` file.
4. **Atomic Publication**: Safely copies the verified `.nsp` into `dist/<profile>/<application-name>.nsp` (or custom `-o` path).
5. **Persistent Logging**: Writes comprehensive execution metadata, command-line arguments (with keys redacted), input file hashes (SHA-256), duration, and stdout/stderr to `.nxdev/package/nsp/<profile>/logs/hacbrewpack.log`.

