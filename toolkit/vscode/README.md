# NXDev Toolkit for VS Code

NXDev Toolkit provides editor integration and developer tooling for Nintendo Switch homebrew projects in Visual Studio Code.

## Architecture

NXDev Toolkit is deliberately designed as a **thin frontend** over the native `nxdev` CLI.
It exposes convenient IDE commands, task providers, and debugger configurations without duplicating build or packaging logic within JavaScript/TypeScript.

## First-Class WSL Support

NXDev Toolkit is built to support Windows + WSL workflows as a first-class citizen, bridging editor UI with the Linux-based devkitPro toolchains seamlessly.
