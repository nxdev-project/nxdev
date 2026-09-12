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

2. **hacBrewPack (Planned integration in `third_party/hacbrewpack`)**
   - **Upstream**: [The-4n/hacBrewPack](https://github.com/The-4n/hacBrewPack)
   - **License**: ISC License / GPL
   - **Notice**: When integrated in `third_party/hacbrewpack/`, all upstream licenses, notices, and attribution will be strictly preserved in that subdirectory.

## Strict Policy on Proprietary Nintendo Material

NXDev is an unofficial, clean-room, open-source development ecosystem.

**Under no circumstances does NXDev include, distribute, or require:**
- Nintendo SDK or Nintendo proprietary headers/libraries.
- Nintendo cryptographic keys (e.g. `prod.keys`, `title.keys`, `console.keys`).
- Proprietary Nintendo binaries, microcode, or firmware files.
- Copyrighted game assets or official Nintendo system software.

All packaging tools or workflows requiring cryptographic keys operate strictly using user-provided external key files in adherence to standard homebrew ecosystem practices.
