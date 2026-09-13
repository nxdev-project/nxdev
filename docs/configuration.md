# NXDev Host Configuration

NXDev separates developer-specific machine settings from the shared application manifest (`nxapp.yaml`). Host configuration handles paths to devkitPro, preferred Switch IP addresses for wireless deployment, and local build preferences.

---

## Configuration Scopes

### 1. Global User Configuration
Stored in user-level application directories:
- **Linux / WSL**: `$XDG_CONFIG_HOME/nxdev/config.yaml` or `~/.config/nxdev/config.yaml`
- **macOS**: `~/Library/Application Support/nxdev/config.yaml`
- **Windows**: `%APPDATA%\nxdev\config.yaml`

### 2. Project-Local Configuration
Stored within the project's `.nxdev` folder:
- Path: `<project_root>/.nxdev/local.yaml`
- Recommended for `.gitignore` so personal hardware settings (such as local Switch IP addresses) are not committed to source control.

---

## Configuration File Format

```yaml
# Toolchain path overrides
toolchain:
  devkitPro: "/opt/devkitpro"
  devkitA64: "/opt/devkitpro/devkitA64"

# Deployment target settings
deployment:
  switchIp: "192.168.1.150"
  nxlinkPort: 28280
```

---

## Security & Secrets Policy

NXDev strictly enforces:
- **No plaintext secrets**: Secrets, tokens, or encryption keys must never be placed into configuration files.
- **No Nintendo proprietary keys**: `prod.keys`, `title.keys`, and unauthorized Nintendo signing materials are never stored or managed by NXDev configuration.
