# Packaging NRO and NSP in NXDevToolkit

NXDevToolkit integrates with **NXDevPack** to package compiled Switch ELFs into standard homebrew formats (.nro) and installable packages (.nsp).

---

## Packaging NRO (.nro)

The **NRO backend** bundles:
- The compiled Switch ELF
- NACP metadata (title, author, version)
- Icon (256x256 JPEG)
- Optional RomFS directory

### How to Package NRO
1. From the Command Palette: `NXDev: Package NRO (.nro)`
2. Or from the **NXDev Project** view: click the Package NRO icon.
3. The packaged `.nro` will be written to `dist/<profile>/<name>.nro`.
4. Click **Open Output Folder** in the notification prompt to open the destination directory in your system file explorer.

---

## Packaging NSP (.nsp)

The **NSP backend** bundles:
- Generated NPDM (Program Description Metadata)
- ExeFS partition (main ELF, main.npdm, rtld)
- Control partition (NACP metadata and icon)
- RomFS partition
- Pinned, bundled `hacBrewPack` packaging backend

### Key Requirements
Packaging installable NSP files requires Nintendo Switch encryption keys (such as `prod.keys` / `keys.dat`):
- NXDevToolkit will prompt you to select your local keys file, or
- You can set the `NXDEV_KEYS_FILE` or `KEYS_FILE` environment variable.

> [!CAUTION]
> **No Keys Bundled**: NXDev never bundles, embeds, or distributes copyrighted Nintendo encryption keys. You must provide your own legally obtained keys file.

### How to Package NSP
1. From the Command Palette: `NXDev: Package NSP (.nsp)`
2. If keys are not configured, select your `prod.keys` file in the file picker.
3. The packaged `.nsp` will be generated in `dist/<profile>/<name>.nsp`.
