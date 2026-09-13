# Result & Error Handling (`nxdev::Result`)

NXDev uses a value-or-error idiom instead of C++ exceptions. The system is designed around `nxdev::ResultCode`, `nxdev::Result<T>`, and `nxdev::Result<void>`.

## Design Principles

1. **Horizon OS 32-bit Compatibility**:
   Horizon OS returns 32-bit integer error codes partitioned into:
   - **Module** (bits 0–8, 9 bits): Identifies the subsystem (FS, NCM, OS, etc.).
   - **Description** (bits 9–21, 13 bits): Identifies the specific error condition.
   `ResultCode` preserves this representation exactly without loss.

2. **`[[nodiscard]]` Enforcement**:
   All `Result` types are marked `[[nodiscard]]`, ensuring that returned errors cannot be accidentally ignored.

3. **No Dynamic Allocations**:
   `Result<T>` is implemented using `std::variant<T, ResultCode>`, avoiding heap allocations for error propagation.

---

## `nxdev::Result<T>`

Use `Result<T>` when an operation produces a value on success or an error code on failure.

```cpp
#include <nxdev/result.hpp>

nxdev::Result<int> parse_port(std::string_view str) {
    if (str.empty()) {
        return nxdev::results::InvalidArgument;
    }
    int port = std::stoi(std::string(str));
    return port;
}

void example() {
    auto res = parse_port("8080");
    if (res.is_success()) {
        int port = res.value();
        // or: int port = *res;
    } else {
        std::cout << "Error: " << res.error_name() << " (0x" << std::hex << res.raw() << ")\n";
    }

    // Default fallback:
    int port = parse_port("").value_or(80);
}
```

---

## `nxdev::Result<void>`

Use `Result<void>` for operations that produce no value but can fail.

```cpp
nxdev::Result<void> init_service() {
    // perform initialization...
    if (failed) {
        return nxdev::results::NotInitialized;
    }
    return nxdev::Result<void>::Success();
}
```

---

## Early Propagation with `NXDEV_TRY`

The `NXDEV_TRY(expr)` macro enables clean, Rust-like error propagation:

```cpp
nxdev::Result<std::string> load_config_file() {
    auto fs_guard = NXDEV_TRY(nxdev::make_service_guard(fsInitialize, fsExit));
    auto file_data = NXDEV_TRY(read_file("sdmc:/config.ini"));
    return file_data;
}
```

---

## Converting to/from libnx

```cpp
// Converting raw libnx Result:
::Result raw = fsOpenSdCardFileSystem(&fs);
nxdev::Result<void> res = nxdev::from_libnx(raw);

if (res.failed()) {
    printf("FS Error: Module %u, Desc %u\n", res.module(), res.description());
}
```
