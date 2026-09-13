# devkitPro Package Management with NXDev CLI

NXDev includes a package management workflow integrated with devkitPro's `dkp-pacman` tool.

---

## Commands Reference

### `nxdev package list`

List all supported NXDev modules and their current host installation status:

```bash
nxdev package list
```

Add `--json` for machine-readable output:

```bash
nxdev package list --json
```

### `nxdev package info <id>`

Inspect details for a specific module, including its category, upstream URL, required devkitPro system packages, and CMake targets:

```bash
nxdev package info nxdev.sdl2
```

### `nxdev package search <query>`

Search available modules across ID, name, description, category, and system packages:

```bash
nxdev package search audio
nxdev package search freetype
```

### `nxdev package status`

Checks the installation status of all dependencies required by the active project (`nxapp.yaml`):

```bash
nxdev package status
```

Example Output:
```text
Project Dependencies Status (Project: MySwitchApp):

Module ID            Type        Status          Details
---------------------------------------------------------------------------
nxdev.core           builtin     Installed       built-in
nxdev.input          builtin     Installed       built-in
nxdev.sdl2           devkitpro   Installed       version: 2.28.5-4
nxdev.sdl2-image     devkitpro   Missing         run 'nxdev package install nxdev.sdl2-image'

Tip: Run 'nxdev package install --missing' to install missing project dependencies.
```

### `nxdev package install`

Install one or more modules:

```bash
# Install individual modules
nxdev package install nxdev.sdl2-image nxdev.sdl2-mixer

# Install all missing project dependencies
nxdev package install --missing

# Dry-run mode (simulate without making system changes)
nxdev package install --missing --dry-run

# Non-interactive mode (for CI/CD environments)
nxdev package install --missing -y
```

### `nxdev package remove`

Remove devkitPro portlib packages:

```bash
nxdev package remove nxdev.sdl2-image --dry-run
nxdev package remove nxdev.sdl2-image -y
```

---

## Doctor Integration

The `nxdev doctor` command automatically checks all declared dependencies of your active project and verifies artifact presence under `$DEVKITPRO/portlibs/switch`:

```bash
nxdev doctor
```
