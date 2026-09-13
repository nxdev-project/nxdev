# NXDev CLI JSON API Reference

The NXDev CLI provides structured, machine-readable JSON outputs for integration with IDEs (such as **NXDevToolkit**), automation scripts, and CI pipelines.

---

## 1. Machine API Version

- **CLI JSON API Version**: `1`
- **Schema Version**: `1`

All JSON responses follow standard envelope conventions.

---

## 2. Environment (`nxdev env --json`)

Returns detected host environment, toolchain paths, and active flags.

```json
{
  "devkitpro": {
    "root": "/opt/devkitpro",
    "devkitA64": "/opt/devkitpro/devkitA64",
    "libnx": "/opt/devkitpro/libnx",
    "installed": true
  },
  "tools": {
    "aarch64Gcc": "/opt/devkitpro/devkitA64/bin/aarch64-none-elf-gcc",
    "elf2nro": "/opt/devkitpro/tools/bin/elf2nro",
    "nacptool": "/opt/devkitpro/tools/bin/nacptool",
    "nxlink": "/opt/devkitpro/tools/bin/nxlink"
  },
  "host": {
    "os": "linux",
    "arch": "x86_64",
    "wsl": false
  }
}
```

---

## 3. Diagnostics (`nxdev doctor --json`)

Returns structured check results with severity levels (`ok`, `warning`, `error`) and actionable fix hints.

```json
{
  "summary": {
    "status": "ok",
    "passed": 8,
    "warnings": 0,
    "errors": 0
  },
  "checks": [
    {
      "id": "toolchain.devkitpro",
      "category": "Toolchain",
      "name": "devkitPro Environment",
      "status": "ok",
      "message": "Found DEVKITPRO at /opt/devkitpro",
      "hint": ""
    },
    {
      "id": "manifest.validation",
      "category": "Project",
      "name": "NXDevAppManifest Validation",
      "status": "ok",
      "message": "nxapp.yaml is valid",
      "hint": ""
    }
  ]
}
```

---

## 4. Templates Catalog (`nxdev new --list --json`)

Returns list of available project starter templates with metadata.

```json
{
  "version": 1,
  "templates": [
    {
      "id": "minimal-c",
      "name": "Minimal C",
      "description": "Clean C project using raw libnx",
      "author": "NXDev Team",
      "dependencies": []
    },
    {
      "id": "console-cpp",
      "name": "Console C++",
      "description": "Standard modern C++ project using NXDev::Core and NXDev::Input",
      "author": "NXDev Team",
      "dependencies": ["nxdev.core", "nxdev.input"]
    }
  ]
}
```

---

## 5. Build Result (`nxdev build --json`)

Returns build status, artifact locations, and compilation metrics.

```json
{
  "status": "success",
  "target": "my-app",
  "elfPath": "/path/to/project/.nxdev/build/my-app.elf",
  "buildType": "Release",
  "durationMs": 1420
}
```

---

## 6. Packaging Result (`nxdev pack nro --json`)

Returns packaging results for `.nro` artifacts.

```json
{
  "status": "success",
  "artifactType": "nro",
  "outputPath": "/path/to/project/dist/release/my-app.nro",
  "nacp": {
    "name": "My App",
    "author": "Developer",
    "version": "0.1.0"
  },
  "hasRomfs": true,
  "hasIcon": true,
  "sizeBytes": 1048576
}
```

---

## 7. Runtime Stream (`nxdev run --json-stream`)

During interactive deployment and debugging, `nxdev run` emits line-delimited JSON events over standard output.

### Event Types:

1. **`session`**: Initiated connection to Switch.
   ```json
   {"type": "session", "ip": "192.168.1.100", "port": 28280, "status": "connecting"}
   ```
2. **`stage`**: Transition between pipeline phases (`build`, `pack`, `transfer`, `execute`).
   ```json
   {"type": "stage", "name": "transfer", "progress": 1.0}
   ```
3. **`log`**: Standard output or error captured from Switch application.
   ```json
   {"type": "log", "stream": "stdout", "message": "NXDev Core Initialized"}
   ```
4. **`diagnostic`**: Warning or runtime diagnostic detected.
   ```json
   {"type": "diagnostic", "severity": "warning", "message": "Socket buffer overflow"}
   ```
5. **`crash`**: Parsed crash report with symbolicated stack frames.
   ```json
   {
     "type": "crash",
     "module": "main",
     "address": "0x00000071000142a0",
     "frames": [
       {"frame": 0, "function": "foo()", "file": "src/main.cpp", "line": 42}
     ]
   }
   ```
6. **`exit`**: Application process terminated on target.
   ```json
   {"type": "exit", "code": 0}
   ```
