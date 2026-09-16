# RomFS Staging Pipeline

NXDev features an automated, host-side **RomFS Staging Tool and Pipeline** (`nxdev::pack::RomFsStager`) that executes prior to both NRO (`elf2nro`) and NSP (`hacbrewpack`) package generation.

---

## Core Architecture Principles

1. **Source RomFS Immutability**:
   NXDev **never** directly packages or alters your project source directory (e.g. `assets/romfs/` or `romfs/`).
2. **Deterministic Build Staging**:
   All RomFS assets are safely staged into `.nxdev/build/<profile>/romfs/` before being fed to backend packaging tools.
3. **Multi-Layer Priority Overlays**:
   When optional SDK modules (such as `nxdev.borealis`) are included, their framework runtime resources are automatically staged at base priority (100). The user's application RomFS is overlaid on top at priority (1000). **User files always win on collision.**
4. **Unified Package Inputs**:
   Both `nxdev pack nro` and `nxdev pack nsp` consume the identical staged RomFS tree.

---

## Staging Pipeline Flow

```text
┌──────────────────────────────────────┐
│  SDK Framework Resources (Borealis)  │  (Priority 100)
└──────────────────┬───────────────────┘
                   │
                   ▼
       ┌───────────────────────┐
       │   NXDev RomFS Stage   │
       └───────────▲───────────┘
                   │ Overlay
┌──────────────────┴───────────────────┐
│     Project Source RomFS Assets      │  (Priority 1000 - User Wins)
└──────────────────────────────────────┘
                   │
                   ▼
       ┌───────────────────────┐
       │  Final Staged RomFS   │
       └───────────┬───────────┘
                   │
         ┌─────────┴─────────┐
         ▼                   ▼
    ┌─────────┐         ┌─────────┐
    │ elf2nro │         │   NSP   │
    │  (NRO)  │         │(hacbp)  │
    └─────────┘         └─────────┘
```

---

## Concrete Example: Borealis UI Overlay

Given a project using `nxdev.borealis`:

### 1. Framework Base Resources (from NXDevSDK)
```text
share/nxdev/borealis/resources/
├── font/
│   └── material-icons.ttf
├── material/
│   ├── theme_dark.json
│   └── theme_light.json
└── i18n/
    └── en-US.json
```

### 2. User Application RomFS (`romfs/`)
```text
romfs/
├── resources/
│   └── material/
│       └── theme_light.json   <-- Custom user theme override
└── data/
    └── level1.json            <-- Custom game asset
```

### 3. Final Staged RomFS (`.nxdev/build/debug/romfs/`)
```text
.nxdev/build/debug/romfs/
├── resources/
│   ├── font/
│   │   └── material-icons.ttf  (from Borealis SDK)
│   ├── material/
│   │   ├── theme_dark.json     (from Borealis SDK)
│   │   └── theme_light.json    (OVERRIDDEN by User Project)
│   └── i18n/
│       └── en-US.json          (from Borealis SDK)
└── data/
    └── level1.json             (from User Project)
```

---

## Staging Manifest (`romfs-manifest.json`)

During staging, NXDev produces a machine-readable audit manifest at `.nxdev/build/<profile>/romfs-manifest.json` containing:
- SHA-256 hashes of every staged file
- Source layer attributing each file
- Recorded collision and override histories
- A cryptographic composite fingerprint of the entire RomFS payload

Example `romfs-manifest.json`:
```json
{
  "status": "success",
  "hasRomFS": true,
  "stagedDirectory": "/path/to/project/.nxdev/build/debug/romfs",
  "fingerprint": "3c847d1e8...",
  "totalFiles": 139,
  "overriddenFiles": 1,
  "layers": ["borealis", "project"],
  "files": [
    {
      "path": "resources/material/theme_light.json",
      "sourceLayer": "project",
      "sourceFile": "/path/to/project/romfs/resources/material/theme_light.json",
      "sha256": "9a12bc...",
      "sizeBytes": 1420,
      "overriddenLayer": "borealis"
    }
  ]
}
```

---

## Security & Validation

The `RomFsStager` enforces strict safety measures:
- **Recursion Protection**: Rejects configuration where source RomFS points to the staging output directory, `.nxdev`, or filesystem root.
- **Symlink Jail**: Symlinks escaping their respective layer root directories are immediately blocked.
- **Path Traversal Jail**: Paths attempting `../` traversal or escaping the destination directory are rejected.
- **Safe Clean**: Only the staging directory within `.nxdev/` is cleaned before generation; user sources are never touched.
