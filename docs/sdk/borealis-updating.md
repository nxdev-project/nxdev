# Updating Pinned Borealis Backend

This document describes the procedure for NXDev maintainers when updating the pinned `jvrcruzGAMES/borealis` backend revision.

---

## Pinned Revision Information

- **Upstream Repository**: `https://github.com/jvrcruzGAMES/borealis`
- **Tracked Branch**: `wiliwili`
- **Current Commit SHA**: `5f08b286f3df737f3321d2247a6fe633fcead03c`
- **API Version**: `1`

---

## Update Procedure

1. **Inspect Upstream Changes**:
   ```bash
   git clone --recurse-submodules https://github.com/jvrcruzGAMES/borealis.git /tmp/borealis-new
   cd /tmp/borealis-new
   git log <current-sha>..HEAD --oneline
   ```

2. **Verify Licensing & Resources**:
   - Confirm all added source code and resources comply with redistribution requirements (Apache 2.0, MIT, zlib).
   - Ensure no copyrighted or proprietary assets are added.

3. **Update Files in `third_party/borealis/`**:
   - Update `third_party/borealis/library/` and `third_party/borealis/resources/`.
   - Update `third_party/borealis/REVISION.json` with the new commit SHA and date.

4. **Verify Wrapper Compatibility**:
   - Compile `NXDev::Borealis` and ensure no public `nxdev::ui` API changes occurred.
   - Run unit tests and public header isolation checks.

5. **Run Verification & Demos**:
   ```bash
   nxdev configure
   nxdev build
   ctest --test-dir build --output-on-failure
   ```

6. **Update Documentation & Notices**:
   - Update `THIRD_PARTY_NOTICES.md` if any dependency revisions or authors changed.
