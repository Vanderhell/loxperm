# Release checklist

## Content

- `CHANGELOG.md` has a section `## [X.Y.Z]` matching the tag `vX.Y.Z`.
- Version macros in `include/loxperm/loxperm.h` match the release version.

## Local verification

- Configure/build/test (Release):
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`
- Wide mask:
  - `cmake -S . -B build-wide -DLOXPERM_WIDE_MASK=ON -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build-wide`
  - `ctest --test-dir build-wide --output-on-failure`
- Sanitizers (Linux, GCC/Clang):
  - `cmake -S . -B build-sanitize -DLOXPERM_ENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo`
  - `cmake --build build-sanitize`
  - `ctest --test-dir build-sanitize --output-on-failure`
- Install + package smoke:
  - `cmake --install build --prefix dist`
  - Configure a consumer project with `-DCMAKE_PREFIX_PATH=/path/to/dist`

## Tag

- Create and push tag `vX.Y.Z`.
- Verify GitHub Actions release workflow publishes:
  - Source archive
  - Install-tree archive

