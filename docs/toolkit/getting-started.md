# Getting Started with NXDevToolkit

**NXDevToolkit** brings first-class Nintendo Switch homebrew development directly into Visual Studio Code.

---

## Prerequisites

1. **NXDev CLI**: Ensure `nxdev` is built and available in your system `PATH` (or configured via `nxdev.cliPath` in VS Code settings).
2. **devkitPro / devkitA64**: A valid devkitPro installation with `DEVKITPRO` and `DEVKITARM` / `DEVKITA64` environment variables set.
3. **VS Code**: Version 1.80.0 or higher.

---

## Installation

### From VSIX Package

1. In VS Code, open the Extensions view (`Ctrl+Shift+X` / `Cmd+Shift+X`).
2. Click the `...` menu (Views and More Actions) at the top right of the Extensions view.
3. Select **Install from VSIX...**.
4. Choose the `nxdev-toolkit-*.vsix` file.

---

## Opening an NXDev Project

When you open any folder containing an `nxapp.yaml` (or `nxapp.yml`) manifest file:
1. NXDevToolkit automatically detects the project and activates.
2. The **NXDev** icon appears in the Activity Bar.
3. The Status Bar displays `NXDev [Debug]` or `NXDev [Release]`.
4. The manifest file receives automatic schema validation and diagnostics.

---

## Multi-Root Workspaces

NXDevToolkit supports multi-root workspaces. If multiple folders in the workspace contain `nxapp.yaml`, the **NXDev Project** tree view lists all discovered projects and lets you switch context effortlessly.

---

## Initial Health Check

To verify that your development environment is fully operational:
1. Open the Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`).
2. Run `NXDev: Run Doctor Diagnostics`.
3. Review the diagnostic report in the **NXDev** Output Channel and the **Environment & Diagnostics** view.
