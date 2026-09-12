# NXDevAppManifest Specification (`nxapp.yaml`)

The `nxapp.yaml` manifest is the central configuration file for an NXDev homebrew application.

## Schema Versioning

Manifests must specify a format version:

```yaml
manifest_version: "1.0"
```

## Structure Reference

```yaml
manifest_version: "1.0"

app:
  name: "SampleHomebrew"
  author: "DeveloperName"
  version: "1.0.0"
  title_id: "010012345678ABCD"
  icon: "assets/icon.png"

target:
  romfs_dir: "assets/romfs"
  nro_output: "bin/SampleHomebrew.nro"
  nsp_output: "bin/SampleHomebrew.nsp"

dependencies:
  - name: "switch-sdl2"
    version: "^2.28.0"
  - name: "switch-glad"
    optional: false

profiles:
  debug:
    flags: ["-g", "-O0"]
    definitions: ["DEBUG=1"]
  release:
    flags: ["-O3"]
    definitions: ["NDEBUG=1"]
```

## Future Implementation Roadmap

Subsequent stages will introduce:
1. Strict schema validation with error line mapping.
2. NACP metadata block generation.
3. NPDM permission generator.
4. Portlib dependency resolver mapping into devkitPro pacman packages.
