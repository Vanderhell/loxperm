# Release checklist

## Content

- `CHANGELOG.md` has a section `## [X.Y.Z]` matching the tag `vX.Y.Z`.
- Version macros in `include/loxperm/loxperm.h` match the release version exactly.
- `project(loxperm VERSION X.Y.Z ...)` in `CMakeLists.txt` matches the release version exactly.
- The release workflow rejects tags that are not strict semantic versions `vMAJOR.MINOR.PATCH`.

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
- Compile-time config checks:
  - `loxperm_config_c_0`
  - `loxperm_config_c_1`
  - `loxperm_config_c_32`
  - `loxperm_config_c_33`
  - `loxperm_config_c_64`
  - `loxperm_config_c_65`
- C++ include checks:
  - `loxperm_cpp11_include`
  - `loxperm_cpp17_include`
  - `loxperm_cpp20_include`
- Install + package smoke:
  - `cmake --install build --prefix dist`
  - Configure a consumer project with `-DCMAKE_PREFIX_PATH=/path/to/dist`
  - Run a C99 consumer, a C11 consumer, and a C++17 consumer
  - Repeat against the wide-mask install tree
  - Verify archive extraction and consumer build from the extracted install tree

## Tag

- Create and push tag `vX.Y.Z` only after the above checks are green.
- Verify GitHub Actions release workflow publishes:
  - Source archive
  - Install-tree archive
