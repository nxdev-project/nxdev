# hacBrewPack Integration in NXDev

## Overview
This directory contains the pinned upstream source and build configuration for **hacBrewPack**, the standard Nintendo Switch homebrew NSP creator originally created by The-4n.

## Pinned Upstream Metadata
* **Repository**: `https://github.com/The-4n/hacBrewPack`
* **Version**: `3.05`
* **Commit**: `5c34cb2f57564d6db53d3ea7e204c3a7263b6555`
* **License**: ISC License (see `LICENSE`)

## Build Integration
`hacBrewPack` is built as a host-side native tool:
* CMake Target: `nxdev_hacbrewpack` / `NXDevThirdParty::hacBrewPack`
* Output Binary: `build/bin/hacbrewpack`

## Clean-Room Rules & Keys Policy
* **NO CRYPTOGRAPHIC KEYS ARE COMMITTED HERE.**
* `hacBrewPack` is invoked at runtime with user-supplied key files (e.g. `~/.switch/prod.keys` or specified via `nxdev config` / `--keys`).
