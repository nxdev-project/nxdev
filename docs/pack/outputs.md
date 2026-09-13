# Output Artifacts & Formats

NXDev organizes generated build outputs and distribution packages cleanly within the project directory structure.

---

## 1. Directory Structure

```text
my-project/
├── .nxdev/
│   ├── build/
│   │   ├── debug/
│   │   │   ├── bin/my-project.elf
│   │   │   └── compile_commands.json
│   │   └── release/
│   │       ├── bin/my-project.elf
│   │       └── compile_commands.json
│   └── package/
│       └── nro/
│           ├── debug/app.nacp
│           └── release/app.nacp
├── dist/
│   ├── debug/
│   │   └── my-project.nro
│   └── release/
│       └── my-project.nro
└── nxapp.yaml
```

---

## 2. Default Target Naming Convention

By default, output artifact filenames are sanitized from the application name:

* All characters are lowercased.
* Non-alphanumeric characters (including spaces and punctuation) are replaced with hyphens (`-`).
* Consecutive hyphens are collapsed into a single hyphen.
* Leading and trailing hyphens are trimmed.

Example:
* `My Switch App! 🎮` -> `my-switch-app.nro`
* Placed at `dist/debug/my-switch-app.nro`.

Custom output locations can be specified via `nxdev pack -o <path>` or `packaging.nro.output_name` in `nxapp.yaml`.

---

## 3. JSON Output Format

When executing `nxdev pack --json` or `nxdev pack nro --json`, the command emits machine-readable JSON:

```json
{
  "status": "success",
  "format": "nro",
  "profile": "debug",
  "artifact": "/path/to/project/dist/debug/my-project.nro",
  "fileSizeBytes": 879489,
  "elf": "/path/to/project/.nxdev/build/debug/bin/my-project.elf",
  "metadata": {
    "nacp": "/path/to/project/.nxdev/package/nro/debug/app.nacp",
    "icon": "/path/to/project/assets/icon.jpg",
    "iconSource": "project",
    "romfs": "/path/to/project/romfs/"
  },
  "logs": [
    "[Preflight] Validating input ELF binary and resolving switch-tools",
    "[Generate NACP] Generating NACP control metadata via nacptool",
    "[Resolve assets] Resolving and validating icon and RomFS assets",
    "[Create NRO] Converting ELF to NRO binary via elf2nro",
    "[Validate] Validating NRO header and structure",
    "[Finalize] Placing final NRO artifact at dist/debug/my-project.nro"
  ]
}
```

In the event of an error:

```json
{
  "status": "error",
  "format": "nro",
  "profile": "debug",
  "error": {
    "code": "InvalidIcon",
    "message": "Icon file is in PNG format. Nintendo Switch requires a JPEG icon image."
  }
}
```
