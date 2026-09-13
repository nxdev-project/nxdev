# ELF Symbolization with addr2line

`nxdev symbolize` translates raw 64-bit hexadecimal memory addresses into human-readable C++ function names, source filenames, and line numbers using `aarch64-none-elf-addr2line` (from `devkitA64`).

---

## 1. How Symbolization Works

```
0x0000007100014230
        │
        ▼
aarch64-none-elf-addr2line -e app.elf -f -C 0x0000007100014230
        │
        ▼
main() at /home/user/project/src/main.cpp:42
```

* `-f`: Display demangled function names
* `-C`: Demangle C++ symbol names (`_Z...`)
* `-e <elf>`: Specify target AArch64 ELF binary

---

## 2. CLI Command Reference

### Basic Symbolization
```bash
# Symbolize one or more addresses using the active project's latest ELF binary
nxdev symbolize 0x0000007100014230 0x0000007100015500

# Explicitly specify target ELF binary
nxdev symbolize --elf .nxdev/build/debug/bin/my_game.elf 0x0000007100014230

# Apply memory base offset (e.g. ASLR base address 0x7100000000)
nxdev symbolize --base 0x7100000000 0x0000007100014230

# Structured JSON output for IDE extensions
nxdev symbolize --json 0x0000007100014230
```

### JSON Output Format
```json
{
  "elf": "/path/to/app.elf",
  "symbols": [
    {
      "address": "0x0000007100014230",
      "function": "main()",
      "file": "/home/user/project/src/main.cpp",
      "line": 42
    }
  ]
}
```

---

## 3. In VS Code

In `NXDevToolkit`:
1. Open the Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`).
2. Run **`NXDev: Symbolize Crash Address`** (`nxdev.symbolize`).
3. Paste one or more crash addresses.
4. The decoded stack trace will be printed in the **NXDev Output Channel**.
