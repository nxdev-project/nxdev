# Windows & WSL Integration in NXDevToolkit

NXDev and NXDevToolkit provide native support for development on Microsoft Windows using the Windows Subsystem for Linux (WSL).

---

## Two Supported Modes

### Mode 1: VS Code Remote - WSL (Recommended)
When you run VS Code with the **WSL extension** installed and open a folder inside WSL (e.g. `\\wsl$\Ubuntu\home\user\project`):
- NXDevToolkit runs directly inside the Linux environment.
- No special configuration is required; devkitPro toolchains and `nxdev` execute as native Linux binaries.

### Mode 2: Windows-Hosted VS Code with WSL Bridge
If you run VS Code on Windows directly and keep your files on Windows drives (`C:\Users\...`):
1. Enable WSL in settings: `nxdev.wsl.enabled: true`.
2. (Optional) Set target distribution: `nxdev.wsl.distribution: "Ubuntu"`.
3. NXDevToolkit automatically translates Windows drive paths to WSL mounts (`C:\Users\Dev` ↔ `/mnt/c/Users/Dev`) and dispatches commands via `wsl.exe`.

---

## Troubleshooting WSL

- **devkitPro not found in WSL**:
  Ensure `/etc/profile.d/devkit-env.sh` or your `~/.bashrc` exports `DEVKITPRO=/opt/devkitpro` and `DEVKITA64=/opt/devkitpro/devkitA64`.
- **Permission errors with Windows mounts**:
  Ensure your project files inside `/mnt/c/` have appropriate read/write permissions.
- **Run Doctor in WSL**:
  Execute `wsl nxdev doctor` or run the doctor command inside VS Code to verify toolchain detection.
