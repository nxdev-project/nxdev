# `nxdev prepare`

Installs and configures the official devkitPro toolchain and Switch build prerequisites on Linux development hosts.

---

## Supported Environments

- **Linux**: Ubuntu 22.04+, Debian 12+, Fedora 39+, Arch Linux / Manjaro.
- **Windows via WSL / WSL2**: Supported when running inside the Linux guest environment.
- **Native Windows / macOS**: Unsupported by `nxdev prepare`. On native Windows, WSL2 is recommended. On macOS, follow the [official devkitPro macOS instructions](https://devkitpro.org/wiki/Getting_Started).

---

## Usage

```bash
# Interactive preparation
nxdev prepare

# Non-interactive / CI execution
nxdev prepare --yes

# Dry run (display planned commands without modifying host)
nxdev prepare --dry-run

# Check readiness status
nxdev prepare --check

# Force package repair/reinstallation
nxdev prepare --repair
```

---

## Environment Configuration & Discovery

`nxdev prepare` persistently configures the canonical devkitPro environment variables and required `PATH` entries:

```sh
export DEVKITPRO="/opt/devkitpro"
export DEVKITA64="$DEVKITPRO/devkitA64"
```

### Managed Shell Block

Configuration is written into `/etc/profile.d/devkitpro.sh` (for system-wide installations) or the detected user shell profile (`~/.bashrc`, `~/.zshrc`, or `~/.profile`) using a demarcated managed block:

```sh
# >>> NXDev devkitPro >>>
export DEVKITPRO="/opt/devkitpro"
export DEVKITA64="$DEVKITPRO/devkitA64"
case ":$PATH:" in
    *":$DEVKITA64/bin:"*) ;;
    *) export PATH="$DEVKITA64/bin:$PATH" ;;
esac
case ":$PATH:" in
    *":$DEVKITPRO/tools/bin:"*) ;;
    *) export PATH="$DEVKITPRO/tools/bin:$PATH" ;;
esac
# <<< NXDev devkitPro <<<
```

Repeated runs of `nxdev prepare` safely replace or preserve this block without duplicating environment entries or PATH variables.

### Conflict Detection & Custom Installations

- **Custom Installation Preservation**: If a valid devkitPro installation exists at a non-default location (e.g., `/home/user/devkitpro`), `nxdev prepare` respects and preserves that location.
- **Conflict Detection**: If `DEVKITPRO` and `DEVKITA64` point to inconsistent trees (e.g., `DEVKITPRO=/opt/devkitpro` but `DEVKITA64=/other/devkitA64`), `nxdev prepare` detects the mismatch and prompts to repair or fails with actionable instructions. Running with `--repair` automatically resolves `DEVKITA64` to `$DEVKITPRO/devkitA64`.

---

## Verification & Doctor Output

At the end of preparation, `nxdev prepare` immediately updates the current process environment and verifies toolchain readiness without requiring a new terminal session:

```text
devkitPro Environment
  [PASS] DEVKITPRO=/opt/devkitpro
  [PASS] DEVKITA64=/opt/devkitpro/devkitA64
  [PASS] aarch64-none-elf-gcc (16.1.0)
  [PASS] libnx (4.6.0)
  [PASS] switch-tools
```

`nxdev doctor` distinguishes:
- `variable missing` (e.g. `DEVKITPRO` not set)
- `path missing` (configured path does not exist)
- `path inconsistent` (`DEVKITA64` does not belong to `DEVKITPRO`)
- `tool missing from PATH` (`devkitA64 bin` or `tools bin` not in `PATH`)
- `valid environment`

---

## Safety & Idempotency

- `nxdev prepare` displays the full installation plan and requests confirmation before invoking any privileged system commands.
- If devkitPro and core tools are already healthy and verified, running `nxdev prepare` is idempotent and exits cleanly without redundant changes.
- Automated post-install diagnostics verify compiler binaries and tools against `nxdev doctor`.
