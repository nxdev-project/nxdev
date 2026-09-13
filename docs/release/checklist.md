# Release Checklist

Use this pre-flight checklist prior to tagging any new NXDev release.

---

## 1. Version & Metadata Alignment

- [ ] Check single source of truth in top-level `VERSION` file.
- [ ] Verify `sdk/core/include/nxdev/version.hpp` and `types.hpp` matches `VERSION`.
- [ ] Verify `toolkit/vscode/package.json` matches `VERSION`.
- [ ] Verify `CMakeLists.txt` project version matches `VERSION`.
- [ ] Update `CHANGELOG.md` with new version header, release date, and full list of changes.

---

## 2. Test Suite & Code Quality

- [ ] Clean build of all host C++ targets:
  ```bash
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j$(nproc)
  ```
- [ ] Run complete host test suite:
  ```bash
  ctest --test-dir build --output-on-failure
  ```
- [ ] Run VS Code extension unit and integration tests:
  ```bash
  cd toolkit/vscode
  npm ci
  npm test
  npm run lint
  ```
- [ ] Verify project templates build with target Switch toolchain:
  ```bash
  nxdev new test-smoke --template console-cpp
  cd test-smoke && nxdev configure && nxdev build && nxdev pack nro
  ```

---

## 3. Security & Licensing

- [ ] Ensure **no** private keys (`prod.keys`, `title.keys`), secrets, or tokens are committed in Git.
- [ ] Verify `THIRD_PARTY_NOTICES` includes all redistributed or bundled components (`hacBrewPack`, `yaml-cpp`, `nlohmann/json`, etc.).
- [ ] Confirm no copyrighted Nintendo assets/binaries exist in `templates/`, `examples/`, or `assets/`.

---

## 4. Release Artifacts & Packaging

- [ ] Package VS Code Extension:
  ```bash
  cd toolkit/vscode
  npx @vscode/vsce package --out ../../dist/nxdev-toolkit-<version>.vsix
  ```
- [ ] Generate archive tarball of host CLI and tools.
- [ ] Generate cryptographic checksums:
  ```bash
  cd dist
  sha256sum * > SHA256SUMS
  ```

---

## 5. Tagging & Publishing

- [ ] Create signed Git tag: `git tag -s v<version> -m "Release v<version>"`
- [ ] Push tag: `git push origin v<version>`
- [ ] Verify GitHub Actions release workflow successfully builds and attaches assets.
- [ ] Mark prerelease flag on GitHub if tagging `-alpha` or `-beta`.
