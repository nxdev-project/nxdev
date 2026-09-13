# Crash Diagnostics & Exception Analysis

When an unhandled exception or panic occurs in your Nintendo Switch homebrew application, Horizon and libnx output crash telemetry. NXDev parses crash frames, extracts instruction pointers, and resolves them to exact source code lines.

---

## 1. Supported Crash Formats

NXDev's crash parser automatically identifies:

1. **Explicit NXDev Crash Tags**:
   ```
   [NXDEV-CRASH] 0x0000007100014230 0x0000007100015500
   ```
2. **Horizon Program Counter / Link Register lines**:
   ```
   PC: 0x0000007100014230, LR: 0x0000007100015500
   ```
3. **Stack / Backtrace Frame Lines**:
   ```
   Frame #0: 0x0000007100014230
   Frame #1: 0x0000007100015500
   ```

---

## 2. Automatic In-Session Diagnostics

During an `nxdev run` session, if a crash line is detected in the incoming log stream:
1. NXDev immediately emits a high-visibility warning banner.
2. The crash addresses are captured.
3. If an ELF binary with debug symbols is available (`.nxdev/build/<profile>/bin/<app>.elf`), NXDev invokes `aarch64-none-elf-addr2line` and prints demangled stack frame information directly below the crash notification.

---

## 3. Best Practices for Debuggable Binaries

To ensure complete crash traces with line numbers:
* Use the **`debug` profile** during development (`nxdev run --profile debug` or default).
* In `nxapp.yaml`, the `debug` profile includes `-g` symbols and `-O0`/`-Og` optimizations by default.
* Do not strip debug symbols before running/deploying.
