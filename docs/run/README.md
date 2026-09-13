# NXDev Run, Deploy & Runtime Diagnostics Subsystem

The **NXDev Run & Deploy** subsystem provides a streamlined development loop for deploying, running, and diagnosing homebrew applications on physical Nintendo Switch hardware.

```
       edit code
          │
          ▼
      nxdev build
          │
          ▼
    nxdev pack nro
          │
          ▼
    nxdev deploy / nxdev run ───► nxlink netloader (port 28280)
          │                                  │
          ▼                                  ▼
   live log stream (NDJSON / stdout) ◄─── Nintendo Switch (libnx)
          │
          ▼
  crash capture & addr2line symbolization
```

---

## Key Features

1. **Fast Interactive Loop (`nxdev run`)**:
   Automatically checks/builds the Switch ELF, packages the `.nro` bundle, deploys to target device via `nxlink`, starts a logging server session, captures stdout/stderr, detects crashes, and symbolizes backtrace addresses.
2. **Dedicated Deployment (`nxdev deploy`)**:
   Deploys an `.nro` binary to the target Switch without maintaining an open logging server session.
3. **Multi-Device Target Management (`nxdev devices`)**:
   Manage multiple target Switch consoles by symbolic ID (e.g. `switch-desk`, `oled-living-room`) with support for defaults, project-local overrides, and TCP connectivity preflight checks.
4. **Live Log Streaming & Structured Events**:
   Streams stdout/stderr in real-time or outputs machine-readable NDJSON (`--json-stream`) for IDE integration. Automatically persists session logs to `.nxdev/logs/run-<timestamp>.log` with bounded retention.
5. **Crash Diagnostics & Symbolization (`nxdev symbolize`)**:
   Parses crash addresses from Horizon crash dumps, panic messages, or user queries and demangles function names, source file paths, and line numbers using `aarch64-none-elf-addr2line`.
6. **Rich VS Code IDE Integration**:
   Full support in `NXDevToolkit` with Device explorer, run/deploy buttons, status bar indicators, command palette integration, and task providers.

---

## Documentation Index

- [Target Device Management Guide](file:///home/jvrcruz/Projects/NXDev/docs/run/devices.md)
- [nxlink Integration & Deployment Guide](file:///home/jvrcruz/Projects/NXDev/docs/run/nxlink.md)
- [Runtime Logging & Session History](file:///home/jvrcruz/Projects/NXDev/docs/run/logging.md)
- [Crash Diagnostics & Exception Analysis](file:///home/jvrcruz/Projects/NXDev/docs/run/crashes.md)
- [ELF Symbolization with addr2line](file:///home/jvrcruz/Projects/NXDev/docs/run/symbolization.md)
- [WSL Networking & Firewall Setup](file:///home/jvrcruz/Projects/NXDev/docs/run/wsl.md)

---

## Quick Start Example

```bash
# 1. Register your Switch IP
nxdev devices add switch-desk 192.168.1.50 --default

# 2. Test connectivity
nxdev devices test switch-desk

# 3. Launch Homebrew Menu netloader on Switch (Press Y in hbmenu)

# 4. Build, package, deploy, and stream logs in one command:
nxdev run

# 5. Symbolize a crash address manually if needed:
nxdev symbolize 0x0000007100014230
```
