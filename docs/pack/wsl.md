# WSL (Windows Subsystem for Linux) Packaging Guide

This guide details best practices and safeguards when using `nxdev pack nsp` inside WSL.

---

## 1. Native Execution Architecture

NXDev automatically detects when it is operating within WSL (via `/proc/version` and WSL environment indicators).

Key principles in WSL:
- **Native Linux Binaries**: NXDev strictly uses native Linux binaries for all host tooling (`hacbrewpack`, `elf2nso`, `npdmtool`, `nacptool`). It does **not** invoke `/mnt/c/.../hacbrewpack.exe` or `cmd.exe`.
- **Memory-Aware Preflight**: NXDev inspects Linux-visible memory (`/proc/meminfo`) rather than assuming unlimited Windows RAM.
- **Direct Log Streaming**: Stdout and stderr are streamed incrementally to disk logs rather than buffering gigabytes in memory, avoiding WSL OOM crashes.
- **Process Group Cleanup**: Child process trees are tracked with dedicated process groups (`setpgid`), ensuring all background processes terminate cleanly.

---

## 2. Diagnosing WSL Crashes / Termination

If a packaging job fails or terminates unexpectedly:

1. **Check Backend Log**:
   ```bash
   cat .nxdev/package/nsp/debug/logs/hacbrewpack.log
   ```
   Inspect the launch metadata, staged file checksums, and live output stream.

2. **Check Memory Safety Ceilings**:
   If the backend exceeded host memory limits, the log will record:
   ```text
   Process exceeded memory safety ceiling of XXXX MiB (current RSS: YYYY MiB)
   ```

3. **Check RomFS Configuration**:
   Ensure `assets.romfs` does not point to `.` (the project root) or parent folders containing `.nxdev`. RomFS must point to a dedicated asset folder (e.g. `romfs/`).

4. **Preserved Staging Workspace**:
   Use `--keep-staging` to inspect intermediate NCA and PFS0 files in `.nxdev/package/nsp/<profile>/backend/`.
