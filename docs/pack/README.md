# NXDevPack Architecture & Packaging Subsystem

`NXDevPack` is the packaging and artifact generation subsystem of the NXDev ecosystem. It transforms compiled Nintendo Switch homebrew binaries (AArch64 ELF) into deployable and installable package formats.

```
                  nxapp.yaml
                      │
                      ▼
               NXDevAppManifest
                      │
                      ▼
                  nxdev build
                      │
                      ▼
                   Switch ELF
                      │
                      ▼
           ┌──────────────────────┐
           │   NXDevPack Engine   │
           └──────────┬───────────┘
                      │
         ┌────────────┴────────────┐
         │                         │
         ▼                         ▼
  NRO Packaging Backend     NSP Packaging Backend
  (Homebrew Menu .nro)      (Installable .nsp)
         │                         │
  ┌──────┴───────────┐      ┌──────┴────────────────────┐
  │ • NACP metadata  │      │ • NPDM generation         │
  │ • Icon validate  │      │ • ExeFS / elf2nso         │
  │ • RomFS embed    │      │ • Control / nacptool      │
  │ • elf2nro link   │      │ • User Switch keys check  │
  └──────┬───────────┘      │ • Pinned hacBrewPack      │
         │                  └──────┬────────────────────┘
         ▼                         │
  dist/<profile>/<app>.nro         ▼
                            dist/<profile>/<app>.nsp
```

---

## 1. Supported Packaging Backends

| Backend | Output Extension | Description | Status |
|---|---|---|---|
| **NRO** | `.nro` | Standard Nintendo Switch Homebrew Menu executable bundle | **Complete** (Prompt 8) |
| **NSP** | `.nsp` | Nintendo Submission Package installable to Switch Home Menu | **Complete** (Prompt 9 via `hacBrewPack`) |

---

## 2. Packaging Flow & Execution Stages

### NRO Flow (4 Stages)
1. **Preflight**: Validates AArch64 ELF binary format and resolves tools (`nacptool`, `elf2nro`).
2. **NACP Generation**: Generates NACP control binary metadata.
3. **Asset Resolution**: Validates JPEG icon and RomFS directory.
4. **Binary Packaging & Validation**: Invokes `elf2nro`, verifies `NRO0` header, and copies artifact to `dist/<profile>/<app>.nro`.

### NSP Flow (7 Stages)
1. **Preflight**: Validates 16-hex Title ID and user-provided Switch key file (`prod.keys`).
2. **ExeFS Preparation**: Converts ELF binary to NSO (`elf2nso`) and stages ExeFS directory.
3. **NPDM Generation**: Generates NPDM descriptor JSON from manifest configuration and compiles to binary `main.npdm` (`npdmtool`).
4. **NACP Generation**: Compiles NACP metadata with title ID (`nacptool`).
5. **Asset Resolution**: Validates JPEG icon and RomFS assets and stages them in Control and RomFS staging directories.
6. **NSP Construction**: Invokes pinned `hacBrewPack` backend adapter with user key file to assemble standard NCA sections and PFS0 container.
7. **Validation & Final Placement**: Verifies PFS0 container magic (`PFS0`) and places output artifact atomically at `dist/<profile>/<app>.nsp`.

---

## 3. Command Line Interface

```bash
# Package active project as Homebrew Menu NRO
nxdev pack nro

# Package active project as Home Menu installable NSP
nxdev pack nsp --keys ~/.switch/prod.keys

# Package with specific profile
nxdev pack nsp --profile release --keys ~/.switch/prod.keys

# Package existing ELF without rebuilding
nxdev pack nsp --no-build --keys ~/.switch/prod.keys

# Specify explicit destination path
nxdev pack nsp -o out/game.nsp --keys ~/.switch/prod.keys

# Dry-run validation (validates manifest & staging without needing keys)
nxdev pack nsp --dry-run

# Structured JSON output for IDEs / CI pipelines
nxdev pack nsp --json --keys ~/.switch/prod.keys
```

---

## 4. Documentation Index

- [NRO Packaging Guide](file:///home/jvrcruz/Projects/NXDev/docs/pack/nro.md)
- [NSP Packaging Guide](file:///home/jvrcruz/Projects/NXDev/docs/pack/nsp.md)
- [hacBrewPack Backend Integration](file:///home/jvrcruz/Projects/NXDev/docs/pack/hacbrewpack.md)
- [Switch Keys Resolution & Legal Policy](file:///home/jvrcruz/Projects/NXDev/docs/pack/keys.md)
- [NPDM Metadata & Service Access](file:///home/jvrcruz/Projects/NXDev/docs/pack/npdm.md)
- [Output Artifacts & Layouts](file:///home/jvrcruz/Projects/NXDev/docs/pack/outputs.md)
- [NRO Troubleshooting](file:///home/jvrcruz/Projects/NXDev/docs/pack/troubleshooting.md)
- [NSP Troubleshooting](file:///home/jvrcruz/Projects/NXDev/docs/pack/troubleshooting-nsp.md)
