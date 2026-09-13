# NSP Packaging Troubleshooting Guide

Common issues encountered when packaging Nintendo Switch applications as `.nsp` and how to resolve them.

---

## 1. `KeyFileMissing`

### Symptom
```text
Error [KeyFileMissing]: NSP packaging requires a user-provided Switch key file (prod.keys).
```

### Cause
No valid Switch cryptographic key file was found.

### Resolution
Obtain `prod.keys` from your Nintendo Switch console and supply it using one of:
1. `--keys /path/to/prod.keys`
2. `export NXDEV_KEYS=/path/to/prod.keys`
3. Place `prod.keys` at `~/.switch/prod.keys`

---

## 2. `MissingTitleId` / `InvalidTitleId`

### Symptom
```text
Error [MissingTitleId]: NSP packaging requires application.titleId configured in nxapp.yaml.
Error [InvalidTitleId]: Invalid Title ID: Title ID must be exactly 16 hexadecimal characters.
```

### Cause
The `nxapp.yaml` manifest does not define `application.titleId`, or the value is not a valid 16-character hexadecimal string.

### Resolution
Add or fix `titleId` in `nxapp.yaml`:
```yaml
application:
  titleId: "0100000000000088"
```

---

## 3. `InvalidIcon`

### Symptom
```text
Error [InvalidIcon]: Invalid icon image: file must be a valid JPEG image (magic bytes 0xFF 0xD8 0xFF).
```

### Cause
The icon file specified in `assets.icon` is a PNG, BMP, or corrupted file.

### Resolution
Convert your icon to a 256x256 JPEG image (`.jpg`).

---

## 4. `NpdmGenerationFailed`

### Symptom
```text
Error [NpdmGenerationFailed]: Failed to generate NPDM binary via npdmtool.
```

### Cause
Invalid thread priorities, CPU core IDs out of range (0-3), or invalid service names (must be 1-8 characters).

### Resolution
Check the `npdm:` section in `nxapp.yaml`. Ensure thread priorities are between 0 and 63, and CPU cores are between 0 and 3.

---

## 5. `ToolNotFound`

### Symptom
```text
Doctor reports: pack.elf2nso missing / pack.npdmtool missing
```

### Resolution
Ensure devkitPro switch-tools is installed in `/opt/devkitpro/tools/bin` or `$DEVKITPRO/tools/bin`.
Run `nxdev doctor` to verify host toolchain health.
