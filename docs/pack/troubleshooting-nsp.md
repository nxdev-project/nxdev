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

## 4. `InvalidRomFS` / RomFS Recursion Error

### Symptom
```text
Error [InvalidRomFS]: RomFS source directory cannot be identical to project root because this would cause recursive inclusion of build and staging artifacts.
```

### Cause
RomFS was configured to point to `.`, `/`, `$HOME`, or contains `.nxdev`.

### Resolution
Place your RomFS asset files in a dedicated folder such as `romfs/` or `assets/romfs/` and set:
```yaml
assets:
  romfs: romfs/
```

---

## 5. `NpdmGenerationFailed`

### Symptom
```text
Error [NpdmGenerationFailed]: Failed to generate NPDM binary via npdmtool.
```

### Cause
Invalid thread priorities, CPU core IDs out of range (0-3), or invalid service names (must be 1-8 characters).

### Resolution
Check the `npdm:` section in `nxapp.yaml`. Ensure thread priorities are between 0 and 63, and CPU cores are between 0 and 3.

---

## 6. `PackagingFailed` / OOM / Process Limits

### Symptom
```text
NSP packaging failed during hacBrewPack execution.

Backend:
  gayhearts/hacBrewPack
  revision: 1a5f378c1b5747c603f4a50a4a97d86cc7c05fd4

Process:
  exit: signal SIGKILL
  peak memory: 3.8 GiB

Backend log:
  .nxdev/package/nsp/debug/logs/hacbrewpack.log
```

### Cause
The backend process was killed by the system OOM killer or exceeded NXDev's memory safety ceiling.

### Resolution
1. Check the backend log at `.nxdev/package/nsp/<profile>/logs/hacbrewpack.log`.
2. Inspect the RomFS asset sizes and ensure there are no unbounded file trees or circular symlinks.
3. If running in WSL, check Linux-allocated memory in `.wslconfig`.
