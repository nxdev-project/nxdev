# WSL Networking & Firewall Setup

When running NXDev under Windows Subsystem for Linux (WSL2), `nxlink` requires network access between the Switch and the WSL2 environment.

---

## 1. WSL2 Networking Modes

### Mirrored Mode (Recommended for Windows 11 23H2+)
In `.wslconfig` on Windows (`%USERPROFILE%\.wslconfig`):
```ini
[wsl2]
networkingMode=mirrored
```
With mirrored networking mode, WSL2 shares the host IP directly, and network packets to/from the Nintendo Switch require no special port forwarding.

### NAT Mode (Standard WSL2)
In standard NAT mode, inbound network connections from the Switch to the WSL2 virtual machine require a port proxy or firewall allowance for port `28280`.

To allow inbound logging packets from the Switch:
```powershell
# Run in Windows PowerShell as Administrator
netsh interface portproxy add v4tov4 listenport=28280 listenaddress=0.0.0.0 connectport=28280 connectaddress=<WSL_IP>
```

---

## 2. Windows Defender Firewall Rule

Ensure Windows Firewall allows inbound UDP/TCP traffic on port 28280 for `nxlink`:
```powershell
# Run in Administrator PowerShell
New-NetFirewallRule -DisplayName "NXDev nxlink Netloader" -Direction Inbound -LocalPort 28280 -Protocol TCP -Action Allow
```
