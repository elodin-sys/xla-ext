# XLA Static Library Test

This directory contains test programs to verify the XLA static library build.

## XLA Version

This test suite is validated against XLA commit `9f150f6b75c08d6ea7b97697c4f393f1a0eb6121`, which matches [JAX v0.8.0](https://github.com/jax-ml/jax/blob/jax-v0.8.0/third_party/xla/revision.bzl).

## Test Coverage

The test program exercises the following XLA features:

1. **PjRt Client Creation** - Creates a CPU client for executing computations
2. **XLA Builder API** - Constructs computational graphs using the builder pattern
3. **Element-wise Operations** - Tests vector addition
4. **Compilation** - Compiles XLA computations to executable code
5. **Execution** - Runs compiled code with real data
6. **Matrix Operations** - Tests matrix multiplication
7. **Data Transfer** - Transfers data between host and device
8. **Result Verification** - Validates computational results

## Prerequisites

You must first build the XLA static library:

```bash
cd ..
XLA_BUILD=true mix
```

This will create the static library archive in `~/Library/Caches/xla/`.

## Test Programs

### 1. Simple Test (`test_simple.cpp`) ✅ Working

A basic test that verifies:
- PjRt CPU client creation
- Literal manipulation
- Shape utilities
- Device queries

**Build and run:**
```bash
make simple
make run-simple
```

Or directly:
```bash
./test_simple
```

### 2. Full Test (`test_xla.cpp`) 🚧 In Development

A comprehensive test that will include:
- Computation building with XLA Builder
- Compilation pipeline
- Execution with real data
- Matrix operations

**Build and run:**
```bash
make
make run
```

Note: This test is being updated for the latest XLA API changes.

## Expected Output (Simple Test)

```
========================================
XLA Static Library Simple Test
========================================

Test 1: Creating PjRt CPU client...
  ✓ CPU client created successfully
  ✓ Device count: 1
  ✓ Addressable device count: 1
  ✓ Platform name: cpu
  ✓ Platform version: cpu

Test 2: Creating literals...
  ✓ Created R1 literal with shape: f32[4]
  ✓ Literal data: [1, 2, 3, 4]

Test 3: Testing shape utilities...
  ✓ Created shape: f32[2,3]
  ✓ Element type: F32
  ✓ Rank: 2
  ✓ Dimensions: [2, 3]

Test 4: Getting device information...
  ✓ Device ID: 0
  ✓ Device kind: cpu
  ✓ ToString: TFRT_CPU_0
  ✓ Default memory space available
  ✓ Memory space kind: device

========================================
✓ All tests passed successfully!
========================================

The XLA static library is working correctly!
```

## Build Configuration

View build settings:

```bash
make info
```

## Cleaning

Remove build artifacts:

```bash
make clean
```

Remove everything including extracted XLA:

```bash
make clean-all
```

## Test Details

### Test 1: Vector Addition
- Input: Two float vectors `[1, 2, 3, 4]` and `[10, 20, 30, 40]`
- Expected: `[11, 22, 33, 44]`
- Tests: Parameter passing, element-wise operations, basic execution

### Test 2: Matrix Multiplication
- Input: 2x3 matrix × 3x2 matrix
- Expected: 2x2 result matrix
- Tests: Multi-dimensional arrays, Dot operation, complex computations

## Troubleshooting

### "XLA archive not found"
Make sure you've built XLA first:
```bash
cd .. && XLA_BUILD=true mix
```

### Linking errors
Check that the linker flags are correctly loaded:
```bash
cat xla_extension/lib/libxla_extension.link
```

### Runtime errors
Try running with verbose logging:
```bash
XLA_FLAGS=--xla_dump_to=/tmp/xla_dump ./test_xla
```

## What This Tests

This test validates that:
- ✅ The static library contains all necessary symbols
- ✅ Headers are correctly installed and accessible
- ✅ Linker flags are sufficient for linking
- ✅ Core XLA functionality works correctly
- ✅ PjRt CPU backend is functional
- ✅ Compilation and execution pipeline works
- ✅ Data transfers between host and device work
- ✅ Computational results are correct

## Next Steps

After successful testing, you can:
1. Use this as a template for your own XLA applications
2. Test more complex computations
3. Profile performance
4. Test GPU backends (if available)

