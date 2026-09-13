# Build Configuration & Profiles

The `build` section defines semantic compiler options and build profiles.

## Semantic Compiler Settings

Rather than embedding non-portable compiler flags, NXDev uses semantic configuration:

```yaml
build:
  language: cpp              # 'cpp' | 'c'
  standard: "c++20"          # 'c++20', 'c++17', 'c++23', 'c11', 'c17'
  defaultProfile: debug

  profiles:
    debug:
      optimization: debug    # 'debug', 'release', 'size', 'fast'
      symbols: true
    release:
      optimization: release
      symbols: false
```

NXDev CMake integration translates these semantic definitions into appropriate toolchain flags for both host and Switch targets.
