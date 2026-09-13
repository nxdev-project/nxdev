# NXDevAppManifest Overview

**NXDevAppManifest** is the versioned application and project manifest format for the NXDev Nintendo Switch homebrew ecosystem.

The standard manifest filename is:

```text
nxapp.yaml
```

---

## Purpose & Architecture

NXDevAppManifest is the single source of truth describing an application's identity, assets, dependencies, compiler settings, Horizon OS metadata (NACP/NPDM), and packaging rules.

```
+-----------------------------------------------------------+
|                        nxapp.yaml                         |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                     Safe YAML Parser                      |
|       (Bounded AST, line/col tracking, safe modes)        |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                  ManifestValidator                        |
|    (Strict schema, relative paths, typed diagnostics)     |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                  Normalized Manifest Model                |
+-----------------------------------------------------------+
          /                   |                   \
         v                    v                    v
+-----------------+  +-----------------+  +-----------------+
|    NXDev CLI    |  |    NXDevPack    |  |  NXDevToolkit   |
| (build/inspect) |  |   (NRO / NSP)   |  |   (VS Code)     |
+-----------------+  +-----------------+  +-----------------+
```

---

## Quick Start: Minimal Manifest

Every NXDev application starts with a minimal `nxapp.yaml`:

```yaml
schemaVersion: 1

application:
  name: "My Switch App"
  author: "Developer"
  version: "1.0.0"
```

All other sections (assets, dependencies, NACP, NPDM, packaging) have sensible, zero-configuration defaults.

---

## Manifest Documentation Index

- [Full Reference (`reference.md`)](reference.md)
- [Application Identity & Localization (`application.md`)](application.md)
- [Assets & Icon Fallback (`assets.md`)](assets.md)
- [Dependencies (`dependencies.md`)](dependencies.md)
- [NACP Configuration (`nacp.md`)](nacp.md)
- [NPDM Permissions & Capabilities (`npdm.md`)](npdm.md)
- [Build Profiles & Semantic Flags (`build.md`)](build.md)
- [Packaging Options (`packaging.md`)](packaging.md)
