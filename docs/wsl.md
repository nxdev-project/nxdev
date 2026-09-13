# Windows Subsystem for Linux (WSL) Support

NXDev provides first-class support for Windows Subsystem for Linux (WSL1 and WSL2).

---

## Architecture & Workflow

WSL provides a seamless Linux environment on Windows hosts for Nintendo Switch homebrew development:

```text
Windows Host (VS Code / Terminal)
      │
      ├── VS Code Remote - WSL
      │         └── nxdev runs natively inside WSL
      │
      └── Future NXDevToolkit Host Bridge
                └── wsl.exe nxdev ...
```

---

## WSL Detection Signals

NXDev automatically detects when it is running inside WSL by analyzing:
- `/proc/version` and `/proc/sys/kernel/osrelease` for Microsoft WSL kernel strings
- Presence of `WSL_DISTRO_NAME` and `WSL_INTEROP`
- Availability of `wslpath` for Windows/WSL path conversions

---

## Filesystem Performance Recommendations

- **WSL-Native Filesystems (`/home/...`)**: Recommended for best I/O throughput and fastest build times.
- **Windows-Mounted Drives (`/mnt/c/...`)**: Supported transparently by NXDev. `nxdev doctor` provides an informational advisory when projects reside on `/mnt/c/` noting that 9P filesystem bridging can introduce build overhead compared with the native WSL ext4 filesystem.
