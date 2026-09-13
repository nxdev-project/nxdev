# NXDevToolkit for Visual Studio Code

**NXDevToolkit** is the official Visual Studio Code extension for the **NXDev** Nintendo Switch homebrew development ecosystem.

---

## Features

- **Explorer Views**:
  - **NXDev Project**: View active application identity, title ID, version, author, RomFS and icon configuration.
  - **Environment & Diagnostics**: Real-time devkitPro / devkitA64 toolchain status and `nxdev doctor` integration.
  - **SDK Modules & Dependencies**: Visual package manager with categorised dependencies.
- **Manifest Intelligence (`nxapp.yaml`)**:
  - Full schema completion, hover docs, and instant validation.
  - Real-time semantic diagnostics powered by `nxdev manifest validate --json`.
  - QuickFix code actions.
- **Compilation & Packaging**:
  - Build Debug and Release Switch ELFs (`Ctrl+Shift+B`).
  - Package `.nro` homebrew executables.
  - Package `.nsp` installable applications.
- **Task Provider**:
  - First-class VS Code `tasks.json` integration.
- **WSL Native**:
  - Seamless development across Windows and Linux via WSL bridge mode.

---

## Configuration

| Setting | Default | Description |
| :--- | :--- | :--- |
| `nxdev.cliPath` | `"nxdev"` | Path to the `nxdev` CLI executable. |
| `nxdev.wsl.enabled` | `false` | Enable WSL bridge mode on Windows hosts. |
| `nxdev.wsl.distribution` | `""` | Target WSL distribution name. |
| `nxdev.environment.devkitProPath` | `""` | Custom devkitPro root path override. |
| `nxdev.build.defaultProfile` | `"debug"` | Default build profile (`debug` or `release`). |
| `nxdev.diagnostics.enableManifestValidation` | `true` | Enable real-time semantic diagnostics on `nxapp.yaml`. |
| `nxdev.logLevel` | `"info"` | Log level for the NXDev Output Channel. |

---

## License & Third-Party Notices

NXDev and NXDevToolkit are released under the terms described in the root repository license. See [THIRD_PARTY_NOTICES.md](file:///home/jvrcruz/Projects/NXDev/THIRD_PARTY_NOTICES.md) for details.
