# hacBrewPack Integration in NXDev

## Overview
This directory contains the pinned upstream source and build configuration for **hacBrewPack**, the Nintendo Switch homebrew NSP packaging backend tool.

## Pinned Upstream Metadata
* **Upstream**: `https://github.com/gayhearts/hacBrewPack`
* **Pinned commit**: `1a5f378c1b5747c603f4a50a4a97d86cc7c05fd4`
* **Version**: `3.17`
* **License**: GPL-2.0 (see `LICENSE`)

## Build Integration
`hacBrewPack` is built natively as a host-side tool using CMake / Make:
* CMake Target: `nxdev_hacbrewpack` / `NXDevThirdParty::hacBrewPack`
* Output Binary: `build/bin/hacbrewpack` (and placed in `libexec/nxdev/hacbrewpack`)

## Clean-Room Rules & Keys Policy
* **NO CRYPTOGRAPHIC KEYS ARE COMMITTED HERE.**
* `hacBrewPack` is invoked at runtime with user-supplied key files (e.g. `~/.switch/prod.keys` or specified via `nxdev config` / `--keys`).
* Keys are never copied into intermediate staging directories or logged.
