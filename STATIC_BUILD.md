# XLA Static Library Build

This fork builds XLA as a **static library** (`.a`) instead of the original **shared library** (`.so`/`.dylib`).

## Current Version

**XLA**: `9f150f6b75c08d6ea7b97697c4f393f1a0eb6121` (JAX v0.8.0)  
**Status**: ✅ Builds and tests successfully on macOS aarch64

## Quick Start

### Build
```bash
XLA_BUILD=true mix
```

### Test
```bash
cd test_static_lib && make run-comprehensive
```
Expected: 20/20 tests pass

### Use
```bash
tar -xzf xla_extension-*.tar.gz
clang++ -std=c++17 -I./xla_extension/include \
  -L./xla_extension/lib -lxla_extension \
  $(cat xla_extension/lib/libxla_extension.link) \
  -o app app.cpp
```

## Upgrading XLA Version

### 1. Update the Makefile

```makefile
# In Makefile, update this line:
OPENXLA_GIT_REV ?= <new_xla_commit_hash>
```

To match a JAX version, find the commit in:  
https://github.com/jax-ml/jax/blob/jax-vX.Y.Z/third_party/xla/revision.bzl

### 2. Clear Build Cache
```bash
rm -rf ~/Library/Caches/xla_build
```

### 3. Attempt Build
```bash
XLA_BUILD=true mix
```

### 4. Fix Package Path Errors

If you see errors like:
```
ERROR: no such package 'xla/some/path': BUILD file not found
```

The XLA codebase structure changed. Update `extension/BUILD`:

**In `cc_static_library` deps** (lines 15-102):
- Find the broken path in the error message
- Search XLA repo for where it moved
- Update the path

**In `transitive_hdrs` deps** (lines 107-185):
- Keep this section **synchronized** with `cc_static_library` deps
- Same packages, same order (mostly)

Common changes between XLA versions:
- Paths reorganize (e.g., `xla/translate/` → `xla/hlo/translate/`)
- Packages split or merge
- New dependencies added
- Old dependencies deprecated

### 5. Test
```bash
cd test_static_lib && make run-comprehensive
```

Fix any API changes in test files if needed.

## Key Files

### Build Configuration
- `Makefile` - XLA version, build flags
- `extension/BUILD` - Bazel build definition, **KEY FILE FOR UPGRADES**
- `extension/static-lib.bzl` - Static library rule (rarely needs changes)
- `WORKSPACE` - Bazel dependencies (rules_apple)

### Test Programs
- `test_static_lib/test_comprehensive.cpp` - 20-test validation suite (recommended)
- `test_static_lib/test_simple.cpp` - 4-test quick validation
- `test_static_lib/Makefile` - Test build system

## Technical Details

### Static Library Build

The key change from original:

**Original** (`cc_binary`):
```python
cc_binary(
  name = "libxla_extension.so",
  linkshared = 1,
  # ... dynamic library options
)
```

**This Fork** (`cc_static_library`):
```python
cc_static_library(
  name = "libxla_extension",
  deps = [
    # All XLA dependencies
  ],
)
```

### Platform Detection

`extension/static-lib.bzl` automatically uses:
- **macOS**: `libtool -static` 
- **Linux**: GNU `ar` with MRI script

### Package Path Synchronization

**Critical**: `extension/BUILD` has two dependency lists that must stay synchronized:

1. **`cc_static_library` deps** (lines 17-101) - What gets linked
2. **`transitive_hdrs` deps** (lines 109-184) - What headers get exported

When upgrading XLA, both lists need the same packages (though `transitive_hdrs` may exclude some impl-only deps).

## Common XLA Upgrade Issues

### Issue: "no such package"
**Cause**: XLA reorganized package structure  
**Fix**: Search XLA repo for new location, update path in BUILD

### Issue: Build succeeds but tests fail
**Cause**: API changes in XLA  
**Fix**: Update test files to match new API

### Issue: Missing symbols at link time
**Cause**: New XLA dependencies not in deps list  
**Fix**: Add missing packages to both `cc_static_library` and `transitive_hdrs`

### Issue: Headers not found when compiling tests
**Cause**: New headers not in `transitive_hdrs`  
**Fix**: Add package providing the header to `transitive_hdrs`

## Testing

### Quick Validation (4 tests, 30 seconds)
```bash
cd test_static_lib && make run-simple
```

### Full Validation (20 tests, 30 seconds)
```bash
cd test_static_lib && make run-comprehensive
```

Tests validate:
- PjRt client/device APIs
- Literal creation
- Shape utilities
- XLA Builder (all major operations)
- Compilation pipeline
- Buffer management
- Type system

## Platforms

| Platform | Status |
|----------|--------|
| macOS aarch64 | ✅ Tested, working |
| macOS x86_64 | Expected to work |
| Linux x86_64 | Expected to work |
| Linux aarch64 | Expected to work |

## Output

Build produces:
```
xla_extension-0.9.1-<platform>-cpu.tar.gz
├── lib/libxla_extension.a (553 MB static library)
├── lib/libxla_extension.link (required linker flags)
└── include/ (all headers)
```

## References

- **Original repo**: https://github.com/elixir-nx/xla
- **JAX releases**: https://github.com/jax-ml/jax/releases
- **XLA source**: https://github.com/openxla/xla
- **Find JAX's XLA commit**: `https://github.com/jax-ml/jax/blob/jax-vX.Y.Z/third_party/xla/revision.bzl`

