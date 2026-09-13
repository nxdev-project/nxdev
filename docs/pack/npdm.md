# NPDM Metadata & Service Access

The **Nintendo Program Descriptor Metadata (NPDM)** defines the capabilities, service permissions, CPU assignments, and thread priorities for Switch executables.

---

## 1. Built-in NPDM Presets

NXDev provides curated NPDM presets configured in `nxapp.yaml`:

| Preset | Target Use Case | Included Services |
|---|---|---|
| `standard` | General Homebrew | `sm:`, `bsds:u`, `acc:u`, `set`, `hid`, `fsp-srv`, `vi:u`, `appletOE`, `audren:u`, `audin:u`, `audout:u`, `time:u`, `psm`, `pl:u` |
| `network` | Online / Multiplayer Apps | Includes `standard` plus `sfdnsres`, `nifm:u`, `ssl` |
| `filesystem` | File Managers & Backup Tools | Includes `standard` plus `fsp-pr`, `fsp-ldr` |
| `multimedia` | Video & Audio Players | Includes `standard` plus `hwopus`, `caps:u`, `caps:a` |
| `advanced` | Full Unrestricted Access | All common homebrew services enabled |

---

## 2. NPDM Customization in `nxapp.yaml`

Developers can select a preset and optionally declare additional service requirements or tune main thread properties:

```yaml
npdm:
  preset: network
  mainThread:
    priority: 44       # 0 (highest) to 63 (lowest)
    core: 0            # 0 to 3
    stackSize: 262144  # Minimum 4096 bytes (default: 0x40000 = 256KB)
  services:
    - "custom:s"
```

---

## 3. Descriptor Generation

During NSP packaging, NXDev generates an `npdm.json` descriptor conforming to devkitPro `npdmtool` specifications and invokes:

```bash
npdmtool .nxdev/package/nsp/<profile>/generated/npdm.json .nxdev/package/nsp/<profile>/staging/exefs/main.npdm
```
