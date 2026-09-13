# NXDev Command-Line Interface (CLI)

The `nxdev` CLI is the central host-side frontend and orchestration tool for the NXDev Nintendo Switch homebrew ecosystem. It provides host environment detection, devkitPro toolchain discovery, project management, configuration handling, and comprehensive environment diagnostics.

---

## Command Overview

```text
nxdev [GLOBAL OPTIONS] <COMMAND> [SUBCOMMAND] [ARGUMENTS...]
```

### Global Options

| Option | Description |
|---|---|
| `-p, --project <path>` | Override active project directory or path to `nxapp.yaml` |
| `--devkitpro <path>` | Explicit override for devkitPro root path |
| `--devkita64 <path>` | Explicit override for devkitA64 toolchain path |
| `-v, --verbose` | Enable verbose diagnostic logging |
| `--no-color` | Disable ANSI color sequences in output |
| `-h, --help` | Print command usage and help information |
| `--version` | Display NXDev version |

---

## Core Commands

### 1. `nxdev doctor`

Executes multi-category health checks on your host system, project configuration, compiler toolchain, Switch packaging utilities, and deployment readiness.

```bash
# Run all diagnostic checks
nxdev doctor

# Run diagnostics with machine-readable JSON output (ideal for IDEs/CI)
nxdev doctor --json

# Run diagnostics targeted for specific workflow profiles
nxdev doctor --build
nxdev doctor --pack
nxdev doctor --deploy
```

#### Exit Codes
- `0`: No blocking errors found (environment is healthy or only has warnings/info notes).
- `1`: One or more blocking errors detected (e.g., missing compiler, invalid manifest, missing devkitPro).

---

### 2. `nxdev env`

Summarizes the detected host platform, architecture, WSL environment, active project, and all discovered toolchain binaries.

```bash
# Human-readable summary
nxdev env

# Machine-readable JSON summary
nxdev env --json
```

---

### 3. `nxdev project`

Inspects and queries the active NXDev project resolved by project discovery.

```bash
# Display project information (application metadata, dependencies, build settings)
nxdev project info

# Output project information as JSON
nxdev project info --json

# Print the canonical root path of the active project (useful for shell scripts)
nxdev project root
```

---

### 4. `nxdev config`

Inspects the global and project-local NXDev configuration.

```bash
# Show configuration file locations
nxdev config path

# List all resolved configuration entries
nxdev config list

# Get a specific configuration key
nxdev config get toolchain.devkitpro
```

---

### 5. `nxdev manifest`

Validates, inspects, and queries the `nxapp.yaml` application manifest.

```bash
# Validate manifest syntax and schema
nxdev manifest validate [path/to/nxapp.yaml]

# Pretty-print parsed manifest metadata
nxdev manifest inspect [path/to/nxapp.yaml]

# Output parsed manifest as JSON
nxdev manifest inspect [path/to/nxapp.yaml] --json

# Output canonical JSON schema for IDE tooling
nxdev manifest schema
```
