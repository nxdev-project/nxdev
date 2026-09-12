# Contributing to NXDev

Thank you for your interest in contributing to **NXDev**! We welcome contributions from developers across the Nintendo Switch homebrew community.

---

## Code of Conduct & Core Rules

1. **Strictly Clean-Room**: Never commit, link to, or distribute proprietary Nintendo material, SDK headers, microcode, keys, or copyrighted assets.
2. **Third-Party Licensing**: Any external tools or libraries introduced must retain their original upstream licenses and copyright headers under `third_party/` and must be referenced in `THIRD_PARTY_NOTICES.md`.
3. **Complementary Ecosystem**: NXDev builds upon devkitPro and libnx without replacing or obfuscating their capabilities.

---

## Repository Structure & Subsystems

- `sdk/`: C++20 SDK abstractions (`NXDev::Core`, `NXDev::Input`, etc.).
- `manifest/`: Manifest parser and schema (`NXDevAppManifest::Parser`).
- `pack/`: Packaging backends for NRO and NSP (`NXDevPack::Core`).
- `cli/`: Unified command line interface (`nxdev`).
- `toolkit/vscode/`: Visual Studio Code extension (TypeScript).
- `examples/`: Sample homebrew projects.
- `tests/`: Automated unit tests.

---

## Development Workflow

### 1. Build and Test Native Components

```bash
# Configure host build
cmake -S . -B build

# Build targets
cmake --build build

# Execute automated tests
ctest --test-dir build --output-on-failure
```

### 2. Formatting & Code Style

- **C/C++**: Follow the `.clang-format` configuration provided in the root directory. Run `./scripts/format.sh` to auto-format code.
- **TypeScript**: Follow ESLint and Prettier standards configured in `toolkit/vscode/`.
- **Naming Conventions**:
  - Namespace: `namespace nxdev` (and `nxdev::manifest`, `nxdev::pack`, etc.)
  - CMake targets: `NXDev::<Component>`, `NXDevPack::<Backend>`, `NXDevAppManifest::<Module>`

---

## Pull Request Guidelines

- Ensure all host tests pass via `ctest`.
- Keep component boundaries decoupled (avoid circular dependencies between SDK, CLI, and IDE tooling).
- Update documentation when introducing new CLI subcommands or manifest fields.
