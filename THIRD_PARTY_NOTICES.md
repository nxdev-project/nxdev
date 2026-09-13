# Third-Party Notices and Licensing

NXDev is an open-source project dedicated to the Nintendo Switch homebrew community.

The core NXDev codebase (including NXDevSDK, NXDevToolkit, NXDevPack, NXDevAppManifest, and the NXDev CLI) is licensed under the **MIT License** as detailed in the `LICENSE` file.

## Third-Party Dependencies & Tools

NXDev interacts with and integrates external open-source tools and libraries from the broader homebrew ecosystem. Each third-party tool retains its original license and copyright notices.

### Future / Planned Bundled or Integrated Dependencies

1. **devkitPro Ecosystem & libnx**
   - **Upstream**: [devkitPro](https://devkitpro.org) / [switch-tools](https://github.com/devkitPro/switch-tools) / [libnx](https://github.com/switchbrew/libnx)
   - **License**: ISC / zlib / GPLv2+ (per component)
   - **Notice**: NXDev does not replace libnx or the devkitPro toolchains; it builds upon them as abstraction and orchestration layers.

2. **devkitPro Portlib Ecosystem Libraries**
   - NXDev allows optional integration with official devkitPro portlib packages (`switch-sdl2`, `switch-sdl2_image`, `switch-sdl2_mixer`, `switch-sdl2_ttf`, `switch-curl`, `switch-mbedtls`, `switch-freetype`, `switch-libopus`, `switch-zlib`, `switch-libpng`, `switch-libjpeg-turbo`, `switch-libogg`, `switch-libvorbis`, `switch-physfs`, `deko3d`).
   - These libraries are distributed by devkitPro and licensed under their respective open-source licenses (zlib, MIT, Apache 2.0, BSD-style, etc.).

3. **hacBrewPack (Bundled in `third_party/hacbrewpack`)**
   - **Upstream**: [The-4n/hacBrewPack](https://github.com/The-4n/hacBrewPack)
   - **Pinned Version**: 3.05 (Commit `5c34cb2f57564d6db53d3ea7e204c3a7263b6555`)
   - **License**: ISC License (Copyright (c) 2018 The-4n)
   - **Notice**: Preserved in `third_party/hacbrewpack/LICENSE` and `third_party/hacbrewpack/README-NXDEV.md`. Built as `nxdev_hacbrewpack`.

## Strict Policy on Proprietary Nintendo Material

NXDev is an unofficial, clean-room, open-source development ecosystem.

**Under no circumstances does NXDev include, distribute, or require:**
- Nintendo SDK or Nintendo proprietary headers/libraries.
- Nintendo cryptographic keys (e.g. `prod.keys`, `title.keys`, `console.keys`).
- Proprietary Nintendo binaries, microcode, or firmware files.
- Copyrighted game assets or official Nintendo system software.

All packaging tools or workflows requiring cryptographic keys operate strictly using user-provided external key files in adherence to standard homebrew ecosystem practices.
