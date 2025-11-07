# XLA Static Library Tests

Tests to validate the XLA static library build.

## Running Tests

### Comprehensive Test (Recommended)
```bash
make run-comprehensive
```
**Validates**: 20 XLA features including Builder API, compilation, buffers, all operations  
**Expected**: All 20 tests pass

### Simple Test (Quick Check)
```bash
make run-simple
```
**Validates**: Basic client, literals, shapes, devices  
**Expected**: All 4 tests pass

## Test Files

| File | Tests | Purpose |
|------|-------|---------|
| `test_comprehensive.cpp` | 20 | Full XLA feature validation ✅ |
| `test_simple.cpp` | 4 | Quick smoke test ✅ |
| `test_xla.cpp` | - | Execution test (has runtime issues) ⚠️ |

## Prerequisites

Build the XLA library first:
```bash
cd .. && XLA_BUILD=true mix
```

## What's Tested

- ✅ PjRt client & device management
- ✅ All primitive types (F16-F64, S8-S64, U8-U64, PRED, C64, C128)
- ✅ Literals (R0 through R3)
- ✅ Shape utilities & tuple shapes
- ✅ XLA Builder API
- ✅ Arithmetic ops (Add, Sub, Mul, Div, Max, Min)
- ✅ Matrix ops (Dot)
- ✅ Unary ops (Neg, Abs, Exp, Log, Sqrt, Tanh)
- ✅ Shape ops (Reshape, Transpose, Broadcast, Concat, Slice)
- ✅ Reductions (Reduce, ReduceAll)
- ✅ Conditionals (Select)
- ✅ Compilation pipeline (CompileAndLoad)
- ✅ Buffer creation and queries

## Build Commands

```bash
make simple          # Build simple test
make comprehensive   # Build comprehensive test
make clean           # Remove build artifacts
make clean-all       # Remove everything including extracted XLA
```

## Troubleshooting

**"XLA archive not found"**  
→ Build the library first: `cd .. && XLA_BUILD=true mix`

**Linking errors**  
→ Check link flags: `cat xla_extension/lib/libxla_extension.link`

**Tests fail after XLA upgrade**  
→ Update test code for API changes in new XLA version
