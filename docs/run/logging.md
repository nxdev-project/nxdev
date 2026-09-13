# Runtime Logging & Session History

During an `nxdev run` session, all standard output (`stdout`) and standard error (`stderr`) transmitted from the Switch over the network socket are captured, formatted, streamed, and persisted.

---

## 1. How Libnx Socket Redirection Works

When an application is launched via `nxlink`, libnx automatically configures `stdout` and `stderr` to pipe data over a TCP socket back to the computer running `nxdev run`.

In your C++ application:
```cpp
#include <iostream>
#include <nxdev/core/core.hpp>

int main(int argc, char* argv[]) {
    // Normal C++ std::cout / printf output will appear in nxdev run terminal
    std::cout << "[INFO] Initializing engine subsystems..." << std::endl;
    std::cerr << "[WARN] Texture format fallback applied" << std::endl;
    return 0;
}
```

---

## 2. Session Log Retention & Inspection

Every `nxdev run` execution automatically records a session log file under:
`<project-root>/.nxdev/logs/run-<YYYYMMDD-HHMMSS>.log`

NXDev maintains a bounded retention policy (the **10 most recent logs** are preserved; older logs are pruned automatically).

### CLI Commands:
```bash
# List available saved session logs
nxdev logs list

# Output path to latest session log
nxdev logs latest

# Show contents of the latest session log
nxdev logs show

# Show contents of a specific session log
nxdev logs show run-20260913-112233.log
```

---

## 3. Streaming NDJSON Events (`--json-stream`)

For IDEs and integration tooling, `nxdev run --json-stream` outputs newline-delimited JSON messages in real time:

* **Stage Event**:
  ```json
  {"event":"step","step":1,"totalSteps":4,"name":"Building ELF binary"}
  ```
* **Stdout Event**:
  ```json
  {"event":"stdout","line":"[INFO] Scene initialized"}
  ```
* **Stderr Event**:
  ```json
  {"event":"stderr","line":"[WARN] Low memory warning"}
  ```
* **Crash Event**:
  ```json
  {"event":"crash","line":"[NXDEV-CRASH] 0x0000007100014230","addresses":["0x0000007100014230"]}
  ```
* **Exit Event**:
  ```json
  {"event":"exit","code":0}
  ```
