# Filesystem & RomFS (`NXDev::Filesystem`)

The `NXDev::Filesystem` module provides helpers for application RomFS resource mounting, safe text/binary file I/O, path normalization, and filesystem queries.

## CMake Target & Manifest

In your `nxapp.yaml`:
```yaml
build:
  dependencies:
    - nxdev.core
    - nxdev.filesystem
  romfs:
    enabled: true
    directory: "romfs"
```

In your `CMakeLists.txt`:
```cmake
find_package(NXDev REQUIRED)
target_link_libraries(myapp PRIVATE NXDev::Filesystem)
```

## Header

```cpp
#include <nxdev/filesystem.hpp>
```

---

## RomFS Mounting Lifecycle (RAII)

`RomFS::mount()` mounts the embedded application RomFS partition and automatically unmounts it when the RAII guard goes out of scope.

```cpp
#include <nxdev/filesystem.hpp>

void load_assets() {
    auto romfs = nxdev::filesystem::RomFS::mount();
    if (romfs.failed()) {
        // Handle RomFS mount failure
        return;
    }

    // Read asset safely from RomFS:
    auto text_res = nxdev::filesystem::read_text("romfs:/data/dialogue.txt");
    if (text_res.is_success()) {
        std::string text = text_res.value();
    }

    // RomFS is cleanly unmounted here when `romfs` leaves scope.
}
```

---

## File Operations & Size Safety

All file reading and writing functions return `nxdev::Result<T>` and guard against overflow/excessive memory allocations via an optional `max_size` limit (default: 64 MB):

```cpp
// Read UTF-8 text file:
auto text_res = nxdev::filesystem::read_text("sdmc:/switch/myapp/config.json");

// Read binary byte buffer:
auto bytes_res = nxdev::filesystem::read_bytes("sdmc:/switch/myapp/image.raw");

// Write text file (creates parent directories if needed):
auto write_res = nxdev::filesystem::write_text("sdmc:/switch/myapp/log.txt", "Application started.");

// Queries:
bool file_exists = nxdev::filesystem::exists("sdmc:/switch/myapp/config.json");
bool is_dir = nxdev::filesystem::is_directory("sdmc:/switch/myapp");
```

---

## Path Canonicalization Helpers

```cpp
// Builds "romfs:/textures/player.png":
std::string path1 = nxdev::filesystem::romfs_path("textures/player.png");

// Builds "sdmc:/switch/myapp/save.dat":
std::string path2 = nxdev::filesystem::sd_path("switch/myapp/save.dat");

// Builds "sdmc:/switch/0100000000002001/save.dat":
std::string path3 = nxdev::filesystem::app_data_path("0100000000002001", "save.dat");
```
