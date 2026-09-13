# nxlink Integration & Deployment Guide

NXDev integrates directly with `nxlink` (provided by `switch-tools` in devkitPro) to deploy and execute homebrew NROs over the local network via Homebrew Menu netloader.

---

## 1. Prerequisites on Nintendo Switch

1. Connect your Nintendo Switch to the same local area network (LAN / Wi-Fi) as your development machine.
2. Launch the **Homebrew Menu** (`hbmenu`).
3. Press **`Y`** on your controller to activate Netloader mode. The screen will display:
   ```
   Waiting for netloader connection...
   IP Address: 192.168.1.50
   ```

---

## 2. Deploying vs. Running

| Mode | Command | Action | Use Case |
|---|---|---|---|
| **Deploy** | `nxdev deploy` | Sends NRO to Switch via `nxlink -a <host> <nro>` and exits immediately | Deploying binary for standalone testing or automated scripts |
| **Run** | `nxdev run` | Builds ELF, packages NRO, sends via `nxlink -a <host> -s <nro>`, keeps server open to stream logs, captures crashes | Primary iterative inner development loop |

---

## 3. CLI Command Reference

### `nxdev deploy`
```bash
# Deploy active project (default profile and default device)
nxdev deploy

# Deploy specific profile to specific device
nxdev deploy --profile release --device switch-desk

# Deploy directly by IP
nxdev deploy --host 192.168.1.50

# Deploy a specific pre-built NRO file
nxdev deploy --artifact dist/release/my_game.nro --host 192.168.1.50

# Dry-run validation
nxdev deploy --dry-run
```

### `nxdev run`
```bash
# Build, pack NRO, deploy, and stream logs
nxdev run

# Run with custom command-line arguments passed to application
nxdev run --args "--level 3 --debug"

# Skip build or pack step if already compiled
nxdev run --no-build
nxdev run --no-pack

# Dry run preflight verification
nxdev run --dry-run

# Output streaming NDJSON events for IDE tooling
nxdev run --json-stream
```

---

## 4. How nxlink Arguments Work

When NXDev executes `deploy` or `run`, it translates options into standard `nxlink` invocations:

* **Deploy**:
  ```bash
  nxlink -a 192.168.1.50 -r 3 dist/debug/app.nro
  ```
* **Run**:
  ```bash
  nxlink -a 192.168.1.50 -s -r 3 --args "--level 3" dist/debug/app.nro
  ```
  *(The `-s` / `--server` flag starts the local socket server that listens for incoming log output from libnx socket standard I/O redirection.)*
