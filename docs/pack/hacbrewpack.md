# hacBrewPack Backend Integration

NXDevPack delegates low-level NCA construction and PFS0 packaging to a bundled, pinned **gayhearts/hacBrewPack** backend adapter.

---

## 1. Upstream Metadata & License

- **Upstream Project**: [gayhearts/hacBrewPack](https://github.com/gayhearts/hacBrewPack)
- **Pinned Revision**: `1a5f378c1b5747c603f4a50a4a97d86cc7c05fd4`
- **Version**: `3.17`
- **License**: GPL-2.0 (see [third_party/hacbrewpack/LICENSE](file:///home/jvrcruz/Projects/NXDev/third_party/hacbrewpack/LICENSE))
- **Build Target**: `nxdev_hacbrewpack` / `build/bin/hacbrewpack`

---

## 2. Architecture & Role

`hacBrewPack` is maintained as a native host-side tool integrated into NXDev. NXDev shields developers from low-level NCA / PFS0 layouts by orchestrating:

1. Translating `nxapp.yaml` manifest configurations into standard staging directories (`backend/input/exefs/`, `backend/input/control/`).
2. Generating NPDM descriptors for `npdmtool`.
3. Generating NACP binaries for `nacptool`.
4. Staging and verifying 256x256 JPEG icons (with EXIF normalization).
5. Validating RomFS boundaries to prevent recursion and cyclic symlink traversal.
6. Invoking `hacbrewpack` with explicit paths and strict process/memory safeguards:
   ```text
   hacbrewpack --titleid <title_id> --keyset <keyfile> --exefsdir <exefs_dir> --controldir <control_dir> --tempdir <temp_dir> --ncadir <nca_dir> --nspdir <nsp_dir> --backupdir <backup_dir> --nologo [--romfsdir <romfs_dir> | --noromfs]
   ```
7. Streaming stdout and stderr directly to `.nxdev/package/nsp/<profile>/logs/hacbrewpack.log` without unbounded memory buffering.
8. Monitoring peak memory (RSS) and process group state, stopping runaway execution before host instability occurs.
9. Verifying the resulting PFS0 container structure and atomically publishing the final `.nsp` to the `dist/` directory.

---

## 3. WSL Environment Execution

When running under Windows Subsystem for Linux (WSL):
- NXDev builds and executes the native Linux `hacBrewPack` executable.
- NXDev never attempts to execute Windows `.exe` binaries inside WSL.
- Memory usage is actively monitored relative to Linux-visible available memory.
