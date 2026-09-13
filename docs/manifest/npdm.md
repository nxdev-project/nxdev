# NPDM (Nintendo Program Descriptor Metadata)

The `npdm` section specifies executable permissions, address space layout, and system capabilities for standalone packages.

## Presets

NXDev provides convenient standard presets for common homebrew workflows:

- `standard`: Basic user-mode homebrew permissions.
- `network`: Standard permissions plus BSD socket and network services.
- `filesystem`: Storage access and save data mounting capabilities.
- `multimedia`: Audio/video hardware acceleration and display buffers.
- `advanced`: Custom capability masks and service permissions.

## Fields

```yaml
npdm:
  preset: standard
  mainThread:
    priority: 44           # Main thread priority (0..63)
    core: 0                # Preferred CPU core (0..3)
    stackSize: 262144      # Stack size in bytes (e.g. 256 KB)
  addressSpace: 64bit      # '64bit', '32bit', '32bitNoMap', '64bitOld'
  services:
    - "sm:"
    - "time:u"
  filesystem:
    - "SaveData"
  kernelCapabilities:
    - "KernelFlags"
```
