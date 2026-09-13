# Installing NXDevSDK

This guide explains how to install the pre-packaged **NXDevSDK** on development machines.

---

## Installation via `install.sh`

Extract `NXDevSDK.zip` and run the bundled `install.sh` script:

```bash
unzip NXDevSDK.zip
sudo ./install.sh
```

By default, the SDK is installed to `/opt/nxdev/`.

### Custom Installation Prefix

For non-root environments or CI test pipelines, specify `--prefix`:

```bash
./install.sh --prefix /custom/path/nxdev
```

### Installation Options

| Flag | Description |
| :--- | :--- |
| `--prefix <path>` | Install into `<path>` instead of `/opt/nxdev`. |
| `--no-env` | Skip automatic shell profile and environment configuration. |
| `-f`, `--force` | Overwrite existing SDK installation without prompting. |
| `-y`, `--yes` | Non-interactive execution (uses defaults). |
| `--check` | Validates `SDK.zip` payload integrity without performing installation. |
| `-h`, `--help` | Display script help. |

---

## Environment Setup

`install.sh` automatically configures persistent environment variables so developer tools, CMake, and the `nxdev` CLI can discover the SDK and its binaries:

1. **System-wide installations (`/opt/nxdev` with root / sudo)**:
   Creates `/etc/profile.d/nxdev.sh` with permissions `644`.

2. **User-level installations (custom prefix or non-root)**:
   Detects the active shell and updates the appropriate profile (`~/.bashrc`, `~/.zshrc`, or `~/.profile`).

### Managed Configuration Block

Environment lines are encapsulated in a demarcated managed block to guarantee idempotence (running the installer repeatedly replaces rather than duplicates entries):

```sh
# >>> NXDevSDK >>>
export NXDEV_SDK_ROOT="/opt/nxdev"
case ":$PATH:" in
    *":$NXDEV_SDK_ROOT/bin:"*) ;;
    *) export PATH="$NXDEV_SDK_ROOT/bin:$PATH" ;;
esac
# <<< NXDevSDK <<<
```

---

## SDK Discovery Precedence

When building Nintendo Switch applications with CMake or invoking `nxdev` CLI commands:

```text
explicit CLI / config override (-DNXDEV_SDK_ROOT=... or sdk.root)
>
NXDEV_SDK_ROOT environment variable
>
standard platform installation path (/opt/nxdev or C:\NXDevSDK)
>
development / source-tree fallback
```

---

## Verifying the Installation

Run `nxdev doctor` or `nxdev sdk info` to verify that the installed SDK is recognized:

```bash
nxdev doctor
```

Example diagnostics:

```text
NXDevSDK
  [PASS] NXDEV_SDK_ROOT=/opt/nxdev
  [PASS] SDK root exists (/opt/nxdev)
  [PASS] /opt/nxdev/bin is on PATH
  [PASS] CMake package found
  [PASS] SDK version 0.1.0-beta.1
```
