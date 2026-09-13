# hacBrewPack Backend Integration

NXDevPack delegates low-level NCA construction and PFS0 packaging to a bundled, pinned **hacBrewPack** backend adapter.

---

## 1. Upstream Metadata & License

- **Upstream Project**: [The-4n/hacBrewPack](https://github.com/The-4n/hacBrewPack)
- **Pinned Version**: `3.05`
- **Pinned Commit**: `5c34cb2f57564d6db53d3ea7e204c3a7263b6555`
- **License**: ISC License (see [third_party/hacbrewpack/LICENSE](file:///home/jvrcruz/Projects/NXDev/third_party/hacbrewpack/LICENSE))
- **Build Target**: `nxdev_hacbrewpack` / `build/bin/hacbrewpack`

---

## 2. Architecture & Role

`hacBrewPack` is treated as an internal implementation detail and toolchain provider. NXDev shields developers from low-level NCA / PFS0 layout by orchestrating:

1. Translating `nxapp.yaml` manifest configurations into standard staging directories (`exefs/`, `control/`, `romfs/`).
2. Generating NPDM descriptors for `npdmtool`.
3. Generating NACP binaries for `nacptool`.
4. Invoking `hacbrewpack` with CLI arguments:
   ```text
   hacbrewpack --titleid=<title_id> --keyfile=<keyfile> --exefsdir=<exefs> --controldir=<control> [--romfsdir=<romfs>] --outdir=<outdir>
   ```
5. Verifying the resulting PFS0 container and copying the final `.nsp` to the `dist/` directory.

---

## 3. Maintenance & Upgrades

To update the pinned version of `hacBrewPack`:

1. Update `third_party/hacbrewpack/UPSTREAM` and `third_party/hacbrewpack/VERSION`.
2. Ensure `CMakeLists.txt` builds cleanly for all supported host architectures (Linux x86_64/AArch64, macOS, Windows/WSL).
3. Run the automated test suite: `ctest --test-dir build --output-on-failure`.
