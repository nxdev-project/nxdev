# NXDevPack Architecture & Packaging Specification

`NXDevPack` is the packaging toolkit for generating deployable Nintendo Switch homebrew binaries.

## Supported Output Formats

1. **NRO (Nintendo Relocated Object)**
   - Used for standard Homebrew Menu execution.
   - Built on devkitPro's `elf2nro` and `nacptool`.
   - Embeds NACP metadata, application icon, and RomFS asset filesystem directly into the single `.nro` binary.

2. **NSP (Nintendo Submission Package)**
   - Used for standalone homebrew installable to the Horizon OS home screen.
   - Built via the `hacBrewPack` backend integration (`third_party/hacbrewpack`).
   - Generates NCAs, Ticket/Cert data, NPDM, and final `.nsp` packages.
   - Strictly utilizes user-supplied key configuration (`keys.dat` or `prod.keys`).

## Backend Adapter Architecture

```
                      +-------------------+
                      |   PackManager     |
                      +-------------------+
                                |
                +---------------+---------------+
                |                               |
                v                               v
      +-------------------+           +-------------------+
      |  NroPackBackend   |           |  NspPackBackend   |
      +-------------------+           +-------------------+
                |                               |
                v                               v
        [ devkitPro Tools ]             [ hacBrewPack ]
         (elf2nro, nacp)               (bundled adapter)
```

Packaging operations are decoupled from UI tooling through the `IPackBackend` abstraction in `NXDevPack::Core`.
