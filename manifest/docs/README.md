# NXDevAppManifest

**NXDevAppManifest** defines the project and application manifest specification for the NXDev ecosystem.

## Standard File Name

The standard manifest file name is:

```yaml
nxapp.yaml
```

## Structure Overview

An `nxapp.yaml` manifest defines:

- **Application Metadata**: Name, author, version, title ID, icon.
- **Target Configurations**: Output filenames, RomFS paths, custom NACP / NPDM settings.
- **Dependencies**: Declarative portlib / devkitPro package dependencies.
- **Build Profiles**: Compiler flags, defines, and build mode overrides (e.g. `debug`, `release`).
- **Packaging Options**: NRO asset embedding, NSP generation parameters.

## Architecture

- The parser and validator are implemented as a native C++ library (`NXDevAppManifest::Parser`).
- It is decoupled from any editor or GUI environment and can be invoked directly by the `nxdev` CLI, build systems, or external scripts.
- Schema definitions (`schema/nxapp.schema.json`) provide IDE intellisense, validation, and tooling auto-completion.
