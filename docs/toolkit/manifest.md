# Manifest Editing & Diagnostics in NXDevToolkit

NXDev uses declarative application manifests (`nxapp.yaml`) to describe metadata, target architecture, RomFS assets, icons, and SDK dependencies.

---

## Rich Manifest Features

### 1. JSON Schema Validation & IntelliSense
Every `nxapp.yaml` is mapped to `schema/nxapp.schema.json`.
- Hover tooltips explain all fields (e.g. `schemaVersion`, `application.titleId`, `romfs.mountPoint`).
- Auto-completion suggests known keys, types, and allowed values.

### 2. Live Semantic Diagnostics
Whenever `nxapp.yaml` is modified and saved (or debounced during editing), NXDevToolkit executes `nxdev manifest validate --json` in the background.
- Semantic errors (e.g. invalid Title ID hex format, missing RomFS directory, unknown dependency names) are flagged directly in the Problems pane and editor gutters.
- Diagnostics are line-accurate and clear.

### 3. QuickFix Code Actions (`Lightbulb`)
When common issues are detected, NXDevToolkit offers 1-click QuickFixes:
- **Add Missing schemaVersion**: Automatically inserts `schemaVersion: 1`.
- **Add Missing target**: Automatically inserts `target: switch`.
- **Add Missing application**: Automatically adds the template `application` block with placeholder fields.
- **Install Missing Dependencies**: When uninstalled packages are referenced in `dependencies`, a quick action triggers `nxdev package install --missing`.
