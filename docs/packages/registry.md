# Package Registry Specification & Schema

The NXDev package registry defines all modules supported by the NXDev ecosystem. It is located at `package-registry/registry.json` and validated by JSON Schema at `package-registry/schema/package-registry.schema.json`.

---

## Registry File Format

```json
{
  "$schema": "./schema/package-registry.schema.json",
  "schema_version": "1.0",
  "packages": [
    {
      "id": "nxdev.sdl2",
      "name": "SDL2",
      "description": "Simple DirectMedia Layer 2 cross-platform multimedia library for Nintendo Switch",
      "kind": "devkitpro",
      "category": "graphics",
      "dependencies": [
        "nxdev.core"
      ],
      "devkitpro": {
        "packages": [
          "switch-sdl2"
        ]
      },
      "cmake": {
        "targets": [
          "NXDev::SDL2"
        ],
        "headers": [
          "SDL2/SDL.h"
        ],
        "libraries": [
          "libSDL2.a"
        ]
      },
      "license": "zlib",
      "upstream_url": "https://www.libsdl.org/"
    }
  ]
}
```

---

## Field Reference

### Root Object

| Field | Type | Required | Description |
|---|---|---|---|
| `$schema` | String | No | Relative or absolute path to JSON Schema |
| `schema_version` | String | Yes | Registry schema version (must be `"1.0"`) |
| `packages` | Array of Objects | Yes | List of package definitions |

### Package Definition Object

| Field | Type | Required | Description |
|---|---|---|---|
| `id` | String | Yes | Unique identifier prefixed with `nxdev.` (e.g., `nxdev.sdl2`, `nxdev.deko3d`) |
| `name` | String | Yes | Human-readable display name |
| `description` | String | Yes | Short summary of the module |
| `kind` | String | Yes | Module type: `builtin`, `devkitpro`, or `meta` |
| `category` | String | Yes | Functional category (`core`, `graphics`, `audio`, `networking`, `system`, `compression`, `filesystem`) |
| `dependencies` | Array of Strings | Yes | List of module IDs required by this package |
| `devkitpro` | Object | Conditional | DevkitPro system packages (required for `devkitpro` kind) |
| `devkitpro.packages` | Array of Strings | Yes | Package names in dkp-pacman repository (e.g., `switch-sdl2`, `deko3d`) |
| `cmake` | Object | Yes | CMake configuration metadata |
| `cmake.targets` | Array of Strings | Yes | Exported CMake imported targets (e.g., `NXDev::SDL2`) |
| `cmake.headers` | Array of Strings | No | Key header file paths for verification under portlibs include |
| `cmake.libraries` | Array of Strings | No | Key archive library paths for verification under portlibs lib |
| `license` | String | No | SPDX license identifier or descriptive name |
| `upstream_url` | String | No | Official upstream project homepage or repository |

---

## Registry Discovery Precedence

When loading the package registry, `nxdev` searches the following locations in order:

1. `NXDEV_PACKAGE_REGISTRY` environment variable (if set and file exists).
2. `package-registry/registry.json` relative to the current working directory or repository root.
3. System directories: `/usr/share/nxdev/package-registry/registry.json` and `$DEVKITPRO/share/nxdev/package-registry/registry.json`.
4. Fallback programmatic built-in registry (`PackageRegistry::create_default()`).
