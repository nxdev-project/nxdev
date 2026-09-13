# SDK Modules & Dependency Management in NXDevToolkit

NXDev features a pluggable SDK and package management architecture. Optional functionality is split into modular libraries backed by devkitPro packages.

---

## SDK Modules Tree View

The **SDK Modules & Dependencies** view organizes all available packages by functional category:

- **Core**: `nxdev.core`
- **Graphics**: `nxdev.sdl2`, `nxdev.sdl2-image`, `nxdev.sdl2-gfx`, `nxdev.deko3d`, `nxdev.gl`
- **Audio**: `nxdev.sdl2-mixer`, `nxdev.audio`
- **Networking**: `nxdev.curl`, `nxdev.mbedtls`, `nxdev.network`
- **Media & Fonts**: `nxdev.sdl2-ttf`, `nxdev.freetype`, `nxdev.libpng`, `nxdev.libjpeg`
- **Storage & Helpers**: `nxdev.sqlite3`, `nxdev.zlib`, `nxdev.json`

---

## Package States

Each package in the tree is flagged with its current installation and project status:
- **`[Installed & Required]`**: Package is installed locally and listed in `nxapp.yaml`.
- **`[Installed]`**: Package is present in your devkitPro environment.
- **`[Missing Dependency]`**: Package is declared in `nxapp.yaml` but missing from your environment.
- **`[Not Installed]`**: Available package in the registry.

---

## Managing Packages from VS Code

1. **Install Missing Dependencies**: Click the Cloud Download icon on the view toolbar, or run `NXDev: Install Missing SDK Dependencies`.
2. **Install a Specific Module**: Right-click any module and select **Install Package**.
3. **Add to Manifest**: Right-click any module and select **Add to Manifest (`nxapp.yaml`)**.
4. **Remove Package**: Right-click an installed module and select **Remove Package**.
