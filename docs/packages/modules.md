# Supported Module Reference

The NXDev package registry defines 23 supported modules: 8 built-in NXDev C++ abstraction modules and 15 verified devkitPro portlibs.

---

## Built-in Modules (In-tree C++ Abstractions)

| Module ID | Display Name | Category | Direct Dependencies | CMake Target | Key Headers |
|---|---|---|---|---|---|
| `nxdev.core` | NXDev Core SDK | `core` | None | `NXDev::Core` | `<nxdev/nxdev.hpp>`, `<nxdev/app.hpp>`, `<nxdev/result.hpp>` |
| `nxdev.input` | NXDev Input | `input` | `nxdev.core` | `NXDev::Input` | `<nxdev/input.hpp>` |
| `nxdev.filesystem` | NXDev Filesystem | `filesystem` | `nxdev.core` | `NXDev::Filesystem` | `<nxdev/filesystem.hpp>` |
| `nxdev.account` | NXDev Account | `system` | `nxdev.core` | `NXDev::Account` | `<nxdev/account.hpp>` |
| `nxdev.network` | NXDev Network | `networking` | `nxdev.core` | `NXDev::Network` | `<nxdev/network.hpp>` |
| `nxdev.time` | NXDev Time | `system` | `nxdev.core` | `NXDev::Time` | `<nxdev/time.hpp>` |
| `nxdev.power` | NXDev Power & Sleep | `system` | `nxdev.core` | `NXDev::Power` | `<nxdev/power.hpp>` |
| `nxdev.display` | NXDev Display & Console | `graphics` | `nxdev.core` | `NXDev::Display` | `<nxdev/display.hpp>` |

---

## devkitPro Portlib Modules

| Module ID | Display Name | Category | devkitPro Package | CMake Target | Key Header |
|---|---|---|---|---|---|
| `nxdev.sdl2` | SDL2 | `graphics` | `switch-sdl2` | `NXDev::SDL2` | `<SDL2/SDL.h>` |
| `nxdev.sdl2-image` | SDL2 Image | `graphics` | `switch-sdl2_image` | `NXDev::SDL2Image` | `<SDL2/SDL_image.h>` |
| `nxdev.sdl2-mixer` | SDL2 Mixer | `audio` | `switch-sdl2_mixer` | `NXDev::SDL2Mixer` | `<SDL2/SDL_mixer.h>` |
| `nxdev.sdl2-ttf` | SDL2 TTF | `graphics` | `switch-sdl2_ttf` | `NXDev::SDL2TTF` | `<SDL2/SDL_ttf.h>` |
| `nxdev.deko3d` | deko3d | `graphics` | `deko3d` | `NXDev::Deko3D` | `<deko3d.hpp>` |
| `nxdev.curl` | libcurl | `networking` | `switch-curl` | `NXDev::Curl` | `<curl/curl.h>` |
| `nxdev.mbedtls` | mbedtls | `networking` | `switch-mbedtls` | `NXDev::MbedTLS` | `<mbedtls/ssl.h>` |
| `nxdev.freetype` | FreeType | `graphics` | `switch-freetype` | `NXDev::FreeType` | `<ft2build.h>` |
| `nxdev.opus` | Opus | `audio` | `switch-libopus` | `NXDev::Opus` | `<opus/opus.h>` |
| `nxdev.zlib` | zlib | `compression` | `switch-zlib` | `NXDev::Zlib` | `<zlib.h>` |
| `nxdev.png` | libpng | `graphics` | `switch-libpng` | `NXDev::Png` | `<png.h>` |
| `nxdev.jpeg` | libjpeg-turbo | `graphics` | `switch-libjpeg-turbo` | `NXDev::JpegTurbo` | `<jpeglib.h>` |
| `nxdev.ogg` | libogg | `audio` | `switch-libogg` | `NXDev::Ogg` | `<ogg/ogg.h>` |
| `nxdev.vorbis` | libvorbis | `audio` | `switch-libvorbis` | `NXDev::Vorbis` | `<vorbis/codec.h>` |
| `nxdev.physfs` | PhysicsFS | `filesystem` | `switch-physfs` | `NXDev::PhysFS` | `<physfs.h>` |
