# NXDevAppManifest Complete Reference (v1)

This document specifies all fields available in `nxapp.yaml` under `schemaVersion: 1`.

```yaml
# Schema Version (Required, integer: 1)
schemaVersion: 1

# Application Metadata (Required)
application:
  name: "My Homebrew App"             # Required, string (1..255 chars)
  author: "Developer Name"            # Required, string (1..255 chars)
  version: "1.0.0"                    # Optional, string (default: "1.0.0")
  titleId: "0100000000001000"         # Optional, 16-digit hex string (Required for NSP)
  description: "Application details"  # Optional, string
  localized:                          # Optional, map of language codes
    en-US:
      name: "My Homebrew App"
      author: "Developer Name"
    ja:
      name: "マイホームブリュー"
      author: "開発者"
    pt-BR:
      name: "Meu App Homebrew"
      author: "Desenvolvedor"

# Project Assets (Optional)
assets:
  icon: "assets/icon.jpg"             # Path to 256x256 JPEG. Falls back to libnx default icon if omitted.
  romfs: "romfs/"                     # Path to directory containing embedded assets.

# Package & Portlib Dependencies (Optional)
dependencies:
  - nxdev.core                        # Shorthand string syntax
  - name: switch-sdl2                 # Detailed object syntax
    version: "^2.28.0"
    optional: false

# Build Orchestration (Optional)
build:
  language: cpp                       # 'cpp' | 'c' (default: cpp)
  standard: "c++20"                   # 'c++20' | 'c++17' | 'c++23' | 'c11' | 'c17'
  defaultProfile: debug               # 'debug' | 'release'
  profiles:
    debug:
      optimization: debug             # 'debug' | 'release' | 'size' | 'fast'
      symbols: true
    release:
      optimization: release
      symbols: false

# Horizon NACP Metadata (Optional)
nacp:
  displayVersion: "1.0.0"             # Defaults to application.version
  startupUserAccount: none            # 'none' | 'required' | 'optional'
  userAccountSwitchLock: false        # boolean
  screenshot: allow                   # 'allow' | 'deny'
  videoCapture: disabled              # 'disabled' | 'manual' | 'automatic'
  logoType: licensedByNintendo        # 'licensedByNintendo' | 'nintendo' | 'distributedByNintendo'
  parentalControl: free               # 'free' | 'restricted'

# Horizon NPDM Capabilities (Optional)
npdm:
  preset: standard                    # 'standard' | 'network' | 'filesystem' | 'multimedia' | 'advanced'
  mainThread:
    priority: 44                      # int (default: 44)
    core: 0                           # int (default: 0)
    stackSize: 262144                 # bytes (default: 256 KB)
  addressSpace: 64bit                 # '64bit' | '32bit' | '32bitNoMap' | '64bitOld'
  services:                           # Required IPC services
    - "sm:"
    - "time:u"

# Packaging Output (Optional)
packaging:
  defaultFormat: nro                  # 'nro' | 'nsp'
  nro:
    enabled: true
  nsp:
    enabled: false

# Development Workflow (Optional)
development:
  nxlink:
    host: "192.168.1.50"              # Target console IP address
    port: 28771                       # Default nxlink port
```
