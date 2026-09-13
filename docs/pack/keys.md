# Switch Keys Resolution & Legal Policy

---

## 1. Strict Legal & Security Policy

> [!IMPORTANT]
> **NXDev does NOT contain, distribute, commit, or bundle Nintendo proprietary cryptographic keys.**
>
> Users must dump their own keys from their legally-owned Nintendo Switch consoles using homebrew utilities such as `Lockpick_RCM`.

---

## 2. Key File Discovery Precedence

NXDev resolves user keys via the following precedence order:

1. **Explicit CLI Flag**:
   ```bash
   nxdev pack nsp --keys /path/to/prod.keys
   ```
2. **Environment Variable**:
   ```bash
   export NXDEV_KEYS=/path/to/prod.keys
   nxdev pack nsp
   ```
3. **Project-Local Directory**:
   - `.nxdev/prod.keys`
   - `.nxdev/keys.dat`
   - `prod.keys` (in project root)
4. **Standard User Switch Directory**:
   - `~/.switch/prod.keys`
   - `~/.switch/keys.dat`
   - `~/.switch/keys.ini`

If no key file is found across any of these locations during a real packaging operation, `nxdev pack nsp` fails cleanly with a `KeyFileMissing` diagnostic explaining how to configure keys.

---

## 3. Dry-Run Mode

`nxdev pack nsp --dry-run` performs preflight validation on Title IDs, icons, RomFS directories, and NPDM presets **without requiring real key files**, enabling CI validation and configuration testing in headless environments.
