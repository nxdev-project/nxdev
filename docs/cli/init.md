# `nxdev init`

Initializes NXDev metadata and build integration inside an existing directory or source tree.

---

## Overview

Unlike `nxdev new` (which scaffolds a brand-new project from a template into a new directory), `nxdev init` is designed to adopt and onboard existing C/C++ or legacy devkitPro/libnx projects into the NXDev ecosystem.

```text
Existing Directory / Codebase
            │
            ▼
       nxdev init
            │
            ├── Inspect directory & detect language (C vs C++20)
            ├── Generate schema-compliant nxapp.yaml
            ├── Safely integrate CMakeLists.txt
            └── Append .nxdev/ & dist/ to .gitignore
```

---

## Usage

```bash
# Initialize current directory
nxdev init

# Initialize specific path
nxdev init /path/to/my-existing-app

# With custom metadata
nxdev init --name "Custom App" --author "Author Name" --title-id "0100000000002000" --version "1.0.0"
```

---

## Command Options

| Option | Description |
| :--- | :--- |
| `[path]` | Target directory to initialize (defaults to current working directory). |
| `--name <name>` | Application display name. Derived from directory name if omitted. |
| `--id <id>` | Internal project identifier / slug. |
| `--author <author>` | Author or team name. |
| `--version <ver>` | Application version string (default `0.1.0`). |
| `--title-id <id>` | 16-character hexadecimal Title ID (default `0100000000001000`). |
| `-f`, `--force` | Force reinitialization if `nxapp.yaml` already exists. |
| `-y`, `--non-interactive` | Non-interactive execution using default values. |
| `--json` | Emit structured JSON output for tool integration. |
| `-h`, `--help` | Display command help. |

---

## Safety & Migration Rules

1. **Existing `nxapp.yaml`**: If an `nxapp.yaml` already exists, `nxdev init` preserves it and runs validation. It will not overwrite existing metadata unless `--force` is passed.
2. **Existing `CMakeLists.txt`**: Existing CMake configuration is never destroyed. If `CMakeLists.txt` already exists, NXDev preserves it and outputs integration advice.
3. **Existing Makefiles / devkitPro Projects**: If a legacy Makefile containing devkitPro rules is detected, NXDev generates `nxapp.yaml` alongside it without altering the original Makefile.
4. **Git Ignore Safety**: Appends `.nxdev/`, `dist/`, and `build/` to `.gitignore` without modifying existing user entries.

---

## Next Steps After `nxdev init`

```bash
nxdev doctor
nxdev configure
nxdev build
nxdev pack nro
```
