# NXDevSDK Packaging & Distribution

NXDev provides a formal packaging pipeline for the **NXDevSDK itself**, producing a self-contained, relocatable distribution archive: **`NXDevSDK.zip`**.

---

## Archive Structure

```text
NXDevSDK.zip
├── install.sh
└── SDK.zip
```

### Inner Payload (`SDK.zip`) Layout

```text
SDK.zip
├── include/
│   └── nxdev/
│       ├── nxdev.hpp
│       ├── app.hpp
│       ├── result.hpp
│       ├── log.hpp
│       ├── system.hpp
│       ├── scope_exit.hpp
│       ├── service.hpp
│       ├── types.hpp
│       ├── version.hpp
│       ├── input.hpp
│       ├── filesystem.hpp
│       ├── account.hpp
│       ├── network.hpp
│       ├── time.hpp
│       ├── power.hpp
│       └── display.hpp
├── src/
│   ├── core/
│   └── modules/
├── share/
│   └── nxdev/
│       ├── cmake/
│       │   ├── NXDevConfig.cmake
│       │   ├── NXDevCompilerOptions.cmake
│       │   ├── NXDevToolchainSwitch.cmake
│       │   ├── NXDevVersion.cmake
│       │   └── toolchains/NXDevSwitch.cmake
│       ├── package-registry/
│       │   └── registry.json
│       ├── templates/
│       └── sdk.json
└── licenses/
    ├── LICENSE
    └── THIRD_PARTY_NOTICES.md
```

---

## Building `NXDevSDK.zip`

You can build the distributable SDK archive using either the CLI or CMake:

```bash
# Using NXDev CLI
nxdev sdk package --output dist/sdk

# Using packaging script
./scripts/package-sdk.sh
```

The resulting artifact is output to `dist/sdk/NXDevSDK.zip` along with `NXDevSDK.zip.sha256`.

---

## Separation from devkitPro

`NXDevSDK.zip` packages only NXDev's own C++ SDK modules, CMake configurations, templates, and schemas. It does **not** redistribute the devkitPro compiler toolchain. The toolchain is provisioned independently via `nxdev prepare`.
