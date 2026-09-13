# Target Device Management

NXDev provides a device management subsystem to configure, store, and target Nintendo Switch development consoles.

---

## 1. Storage Locations & Precedence

Target devices are stored in YAML format:
* **Project-local**: `<project-root>/.nxdev/devices.yaml` (highest persistence precedence)
* **Global user**: `~/.config/nxdev/devices.yaml` (fallback for all projects)

### Device Resolution Precedence (Highest to Lowest):
1. Command-line host argument (`--host <ip-or-name>`)
2. Command-line device argument (`--device <id>`)
3. Environment variable `NXDEV_DEVICE` or `NXDEV_HOST`
4. Project-local default device in `.nxdev/devices.yaml`
5. Global default device in `~/.config/nxdev/devices.yaml`
6. Single configured device (if only 1 device exists across configs)

---

## 2. YAML Configuration Format

```yaml
version: 1
default_device: switch-desk
devices:
  - id: switch-desk
    name: Desk Switch OLED
    host: 192.168.1.50
    port: 28280
    is_default: true
    notes: Connected via 5GHz Wi-Fi
  - id: switch-dock
    name: Living Room Switch
    host: 192.168.1.55
    port: 28280
    is_default: false
```

---

## 3. CLI Commands

### Listing Devices
```bash
# Formatted terminal table
nxdev devices list

# Structured JSON output
nxdev devices list --json
```

### Adding a Device
```bash
# Add device with ID and IP
nxdev devices add switch-desk 192.168.1.50

# Add device with custom port, notes, and mark as default
nxdev devices add switch-desk 192.168.1.50 --port 28280 --default --notes "Primary dev unit"
```

### Setting Default Device
```bash
nxdev devices set-default switch-desk
```

### Testing Device Connectivity
```bash
# Test configured device by ID
nxdev devices test switch-desk

# Test arbitrary IP / hostname
nxdev devices test 192.168.1.50 --port 28280 --timeout 3000
```

### Removing a Device
```bash
nxdev devices remove switch-desk
```

### Showing Device Details
```bash
nxdev devices show switch-desk
nxdev devices show switch-desk --json
```
