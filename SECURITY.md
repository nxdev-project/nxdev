# Security Policy

## 1. Supported Versions

| Version | Supported | Notes |
|---|---|---|
| `0.1.0-beta.1` | :white_check_mark: | Current active pre-release |
| `< 0.1.0` | :x: | Development prototypes |

---

## 2. Reporting a Vulnerability

If you discover a security vulnerability in NXDev or its packaging backends:

1. **Do not create a public GitHub issue.**
2. Send an email with full details, reproduction steps, and platform information to:
   `contact@jvrcruz.games` (or file a confidential GitHub Security Advisory).
3. We will acknowledge receipt within 48 hours and coordinate a fix.

---

## 3. Threat Model & Trust Boundaries

| Component | Trust Level | Security Boundary |
|---|---|---|
| **`nxapp.yaml` Manifests** | **Untrusted** | Project manifests from external repos are untrusted. The manifest parser validates all fields, escapes characters in generated CMake files, and rejects invalid paths. |
| **Runtime Logs (`nxdev run`)** | **Untrusted** | Output received from remote Switch over network socket is untrusted input. The parser parses lines defensively to prevent command injection. |
| **Package Registry (`registry.json`)** | **Trusted** | Ships as part of the official NXDev installation. Defines validated logical mappings to devkitPro portlibs. |
| **Switch Keys (`prod.keys`)** | **User Secret** | Cryptographic Switch keys belong solely to the developer. NXDev never commits, distributes, or prints key contents to log files. |
| **devkitPro / Host Tools** | **Trusted** | Binaries installed in the host environment (`pacman`, `elf2nro`, `hacBrewPack`) are assumed to be trusted development utilities. |

---

## 4. Nintendo Intellectual Property Policy

NXDev strictly adheres to clean-room open-source principles:
* **Zero Proprietary Material**: Under no circumstances does NXDev contain Nintendo SDK headers, proprietary binaries, title keys, master keys, or encrypted ROMs.
* **Clean-Room Packaging**: Packaging tools (`hacBrewPack`, `nacptool`, `elf2nro`) are open-source utilities maintained by the homebrew community.
* Users must supply their own legally dumped cryptographic keys (`prod.keys`) for NSP generation.
