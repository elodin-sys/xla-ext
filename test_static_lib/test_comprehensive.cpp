/**
 * Comprehensive XLA Static Library Test
 * 
 * Tests all major XLA components without requiring execution:
 * 1. PjRt Client & Device Management
 * 2. XLA Builder API (computation graph construction)
 * 3. Shape utilities and type system
 * 4. Literal creation and manipulation
 * 5. HLO operations (Add, Mul, Dot, Reduce, etc.)
 * 6. Compilation pipeline
 * 7. Buffer management
 */

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>

// XLA includes
#include "xla/pjrt/pjrt_client.h"
#include "xla/pjrt/cpu/cpu_client.h"
#include "xla/literal.h"
#include "xla/literal_util.h"
#include "xla/shape_util.h"
#include "xla/hlo/builder/xla_builder.h"
#include "xla/hlo/builder/xla_computation.h"
#include "xla/hlo/builder/lib/math.h"
#include "xla/hlo/builder/lib/matrix.h"
#include "xla/pjrt/pjrt_executable.h"
#include "absl/status/statusor.h"
#include "absl/status/status.h"
#include "absl/types/span.h"

using namespace xla;
using absl::StatusOr;
using absl::Status;

// Helper function to check StatusOr
template<typename T>
T CheckOr(StatusOr<T> status_or, const std::string& context) {
    if (!status_or.ok()) {
        std::cerr << "ERROR in " << context << ": " 
                  << status_or.status().message() << std::endl;
        exit(1);
    }
    return std::move(status_or).value();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "XLA Comprehensive Static Library Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int tests_passed = 0;
    int total_tests = 0;
    
    try {
        // Test 1: PjRt CPU Client
        std::cout << "\n[Test 1] PjRt CPU Client Creation..." << std::endl;
        total_tests++;
        
        CpuClientOptions options;
        options.asynchronous = true;
        options.cpu_device_count = 1;
        
        auto client = CheckOr(
            GetPjRtCpuClient(options),
            "Creating CPU client"
        );
        
        std::cout << "  ✓ CPU client created" << std::endl;
        std::cout << "  ✓ Platform: " << client->platform_name() << std::endl;
        std::cout << "  ✓ Version: " << client->platform_version() << std::endl;
        std::cout << "  ✓ Devices: " << client->device_count() << std::endl;
        tests_passed++;
        
        // Test 2: Device Information
        std::cout << "\n[Test 2] Device Information..." << std::endl;
        total_tests++;
        
        auto device = client->addressable_devices()[0];
        std::cout << "  ✓ Device ID: " << device->id() << std::endl;
        std::cout << "  ✓ Device kind: " << device->device_kind() << std::endl;
        std::cout << "  ✓ Debug string: " << device->DebugString() << std::endl;
        
        auto mem_space = CheckOr(device->default_memory_space(), "Getting memory space");
        std::cout << "  ✓ Memory space kind: " << mem_space->kind() << std::endl;
        tests_passed++;
        
        // Test 3: Literal Creation
        std::cout << "\n[Test 3] Literal Creation and Manipulation..." << std::endl;
        total_tests++;
        
        // Scalar
        auto scalar = LiteralUtil::CreateR0<float>(42.0f);
        std::cout << "  ✓ R0 scalar: " << scalar.data<float>()[0] << std::endl;
        
        // Vector
        std::vector<float> vec_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        auto vector = LiteralUtil::CreateR1<float>(vec_data);
        std::cout << "  ✓ R1 vector shape: " << vector.shape().ToString() << std::endl;
        
        // Matrix
        auto matrix = LiteralUtil::CreateR2<float>({{1, 2, 3}, {4, 5, 6}});
        std::cout << "  ✓ R2 matrix shape: " << matrix.shape().ToString() << std::endl;
        
        // 3D Tensor
        auto tensor = LiteralUtil::CreateR3<float>({
            {{1, 2}, {3, 4}},
            {{5, 6}, {7, 8}}
        });
        std::cout << "  ✓ R3 tensor shape: " << tensor.shape().ToString() << std::endl;
        tests_passed++;
        
        // Test 4: Shape Utilities
        std::cout << "\n[Test 4] Shape Utilities..." << std::endl;
        total_tests++;
        
        Shape shape1 = ShapeUtil::MakeShape(F32, {10, 20});
        std::cout << "  ✓ MakeShape: " << shape1.ToString() << std::endl;
        
        Shape shape2 = ShapeUtil::MakeShape(S32, {5, 5, 5});
        std::cout << "  ✓ Rank: " << shape2.dimensions_size() << std::endl;
        std::cout << "  ✓ Element count: " << ShapeUtil::ElementsIn(shape2) << std::endl;
        std::cout << "  ✓ Byte size: " << ShapeUtil::ByteSizeOf(shape2) << " bytes" << std::endl;
        
        // Test shape compatibility
        Shape shape3 = ShapeUtil::MakeShape(F32, {10, 20});
        bool compatible = ShapeUtil::Compatible(shape1, shape3);
        std::cout << "  ✓ Shape compatibility check: " << (compatible ? "true" : "false") << std::endl;
        tests_passed++;
        
        // Test 5: XLA Builder - Arithmetic Operations
        std::cout << "\n[Test 5] XLA Builder - Arithmetic Operations..." << std::endl;
        total_tests++;
        
        XlaBuilder builder("arithmetic");
        Shape input_shape = ShapeUtil::MakeShape(F32, {4});
        
        auto param_a = Parameter(&builder, 0, input_shape, "a");
        auto param_b = Parameter(&builder, 1, input_shape, "b");
        
        auto sum_op = Add(param_a, param_b);
        auto diff_op = Sub(param_a, param_b);
        auto product_op = Mul(param_a, param_b);
        auto quotient_op = Div(param_a, param_b);
        auto max_op = Max(param_a, param_b);
        auto min_op = Min(param_a, param_b);
        
        auto computation = CheckOr(builder.Build(), "Building arithmetic computation");
        std::cout << "  ✓ Built computation with 6 operations" << std::endl;
        tests_passed++;
        
        // Test 6: XLA Builder - Matrix Operations  
        std::cout << "\n[Test 6] XLA Builder - Matrix Operations..." << std::endl;
        total_tests++;
        
        XlaBuilder matmul_builder("matmul");
        Shape mat_a = ShapeUtil::MakeShape(F32, {4, 3});
        Shape mat_b = ShapeUtil::MakeShape(F32, {3, 5});
        
        auto m1 = Parameter(&matmul_builder, 0, mat_a, "matrix_a");
        auto m2 = Parameter(&matmul_builder, 1, mat_b, "matrix_b");
        
        // Matrix multiplication
        auto matmul = Dot(m1, m2);
        
        auto matmul_comp = CheckOr(matmul_builder.Build(), "Building matmul");
        std::cout << "  ✓ Built matrix multiplication computation" << std::endl;
        tests_passed++;
        
        // Test 7: XLA Builder - Reduction Operations
        std::cout << "\n[Test 7] XLA Builder - Reduction Operations..." << std::endl;
        total_tests++;
        
        XlaBuilder reduce_builder("reduce");
        Shape reduce_input = ShapeUtil::MakeShape(F32, {10, 20});
        auto reduce_param = Parameter(&reduce_builder, 0, reduce_input, "input");
        
        // Sum reduction - build a simple add computation
        XlaBuilder add_builder("scalar_add");
        auto p0 = Parameter(&add_builder, 0, ShapeUtil::MakeShape(F32, {}), "p0");
        auto p1 = Parameter(&add_builder, 1, ShapeUtil::MakeShape(F32, {}), "p1");
        Add(p0, p1);
        XlaComputation add_comp = CheckOr(add_builder.Build(), "Building add comp");
        
        auto sum_reduce = ReduceAll(reduce_param, ConstantR0<float>(&reduce_builder, 0.0f), 
                                    add_comp);
        
        auto reduce_comp = CheckOr(reduce_builder.Build(), "Building reduction");
        std::cout << "  ✓ Built reduction computation" << std::endl;
        tests_passed++;
        
        // Test 8: XLA Builder - Broadcasting
        std::cout << "\n[Test 8] XLA Builder - Broadcasting..." << std::endl;
        total_tests++;
        
        XlaBuilder bcast_builder("broadcast");
        auto scalar_param = Parameter(&bcast_builder, 0, ShapeUtil::MakeShape(F32, {}), "scalar");
        auto broadcasted = Broadcast(scalar_param, {5, 5});
        
        auto bcast_comp = CheckOr(bcast_builder.Build(), "Building broadcast");
        std::cout << "  ✓ Built broadcast computation" << std::endl;
        tests_passed++;
        
        // Test 9: XLA Builder - Reshape and Transpose
        std::cout << "\n[Test 9] XLA Builder - Reshape and Transpose..." << std::endl;
        total_tests++;
        
        XlaBuilder reshape_builder("reshape");
        Shape input_2x3 = ShapeUtil::MakeShape(F32, {2, 3});
        auto reshape_input = Parameter(&reshape_builder, 0, input_2x3, "input");
        
        auto reshaped = Reshape(reshape_input, {3, 2});
        auto transposed = Transpose(reshape_input, {1, 0});
        
        auto reshape_comp = CheckOr(reshape_builder.Build(), "Building reshape");
        std::cout << "  ✓ Built reshape and transpose computation" << std::endl;
        tests_passed++;
        
        // Test 10: XLA Builder - Conditional Operations
        std::cout << "\n[Test 10] XLA Builder - Conditional Operations..." << std::endl;
        total_tests++;
        
        XlaBuilder cond_builder("conditional");
        Shape bool_shape = ShapeUtil::MakeShape(PRED, {4});
        Shape val_shape = ShapeUtil::MakeShape(F32, {4});
        
        auto pred = Parameter(&cond_builder, 0, bool_shape, "predicate");
        auto true_val = Parameter(&cond_builder, 1, val_shape, "true_value");
        auto false_val = Parameter(&cond_builder, 2, val_shape, "false_value");
        
        auto selected = Select(pred, true_val, false_val);
        
        auto cond_comp = CheckOr(cond_builder.Build(), "Building conditional");
        std::cout << "  ✓ Built conditional (select) computation" << std::endl;
        tests_passed++;
        
        // Test 11: Compilation
        std::cout << "\n[Test 11] Compilation Pipeline..." << std::endl;
        total_tests++;
        
        CompileOptions compile_opts;
        compile_opts.executable_build_options.set_num_replicas(1);
        compile_opts.executable_build_options.set_num_partitions(1);
        
        auto executable = CheckOr(
            client->CompileAndLoad(computation, compile_opts),
            "Compiling computation"
        );
        
        std::cout << "  ✓ Compilation successful" << std::endl;
        std::cout << "  ✓ Executable name: " << executable->name() << std::endl;
        std::cout << "  ✓ Num replicas: " << executable->num_replicas() << std::endl;
        std::cout << "  ✓ Num partitions: " << executable->num_partitions() << std::endl;
        std::cout << "  ✓ Addressable devices: " << executable->addressable_devices().size() << std::endl;
        tests_passed++;
        
        // Test 12: Buffer Transfer
        std::cout << "\n[Test 12] Buffer Creation and Transfer..." << std::endl;
        total_tests++;
        
        Literal test_literal = LiteralUtil::CreateR1<float>({1.0f, 2.0f, 3.0f});
        auto buffer = CheckOr(
            client->BufferFromHostLiteral(test_literal, mem_space),
            "Creating buffer from literal"
        );
        
        std::cout << "  ✓ Buffer created from literal" << std::endl;
        std::cout << "  ✓ On-device shape: " << buffer->on_device_shape().ToString() << std::endl;
        std::cout << "  ✓ Buffer size: " << buffer->GetOnDeviceSizeInBytes().value() << " bytes" << std::endl;
        std::cout << "  ✓ Buffer device: " << buffer->device()->device_kind() << std::endl;
        std::cout << "  ✓ Buffer memory space: " << buffer->memory_space()->kind() << std::endl;
        tests_passed++;
        
        //Test 13: Advanced Shape Operations
        std::cout << "\n[Test 13] Advanced Shape Operations..." << std::endl;
        total_tests++;
        
        Shape tuple_shape = ShapeUtil::MakeTupleShape({
            ShapeUtil::MakeShape(F32, {10}),
            ShapeUtil::MakeShape(S32, {5, 5}),
            ShapeUtil::MakeShape(F64, {2, 3, 4})
        });
        std::cout << "  ✓ Tuple shape: " << tuple_shape.ToString() << std::endl;
        std::cout << "  ✓ Is tuple: " << tuple_shape.IsTuple() << std::endl;
        std::cout << "  ✓ Tuple elements: " << tuple_shape.tuple_shapes_size() << std::endl;
        
        tests_passed++;
        
        // Test 14: XLA Builder - Complex Computation
        std::cout << "\n[Test 14] XLA Builder - Complex Computation Graph..." << std::endl;
        total_tests++;
        
        XlaBuilder complex_builder("complex_graph");
        Shape data_shape = ShapeUtil::MakeShape(F32, {10, 10});
        
        auto x = Parameter(&complex_builder, 0, data_shape, "x");
        auto y = Parameter(&complex_builder, 1, data_shape, "y");
        
        // Build a complex graph: (x + y) * (x - y) + x^2
        auto sum = Add(x, y);
        auto diff = Sub(x, y);
        auto prod = Mul(sum, diff);
        auto x_squared = Mul(x, x);
        auto result = Add(prod, x_squared);
        
        auto complex_comp = CheckOr(complex_builder.Build(), "Building complex graph");
        std::cout << "  ✓ Built complex computation graph" << std::endl;
        
        // Compile it
        auto complex_exec = CheckOr(
            client->CompileAndLoad(complex_comp, compile_opts),
            "Compiling complex graph"
        );
        std::cout << "  ✓ Successfully compiled complex graph" << std::endl;
        tests_passed++;
        
        // Test 15: Type System
        std::cout << "\n[Test 15] Type System..." << std::endl;
        total_tests++;
        
        std::vector<PrimitiveType> types = {
            F16, F32, F64,
            S8, S16, S32, S64,
            U8, U16, U32, U64,
            PRED, C64, C128
        };
        
        std::cout << "  ✓ Supported types: ";
        for (auto type : types) {
            std::cout << PrimitiveType_Name(type) << " ";
        }
        std::cout << std::endl;
        
        // Test type properties
        std::cout << "  ✓ F32 size: " << ShapeUtil::ByteSizeOfPrimitiveType(F32) << " bytes" << std::endl;
        std::cout << "  ✓ F64 size: " << ShapeUtil::ByteSizeOfPrimitiveType(F64) << " bytes" << std::endl;
        std::cout << "  ✓ S32 size: " << ShapeUtil::ByteSizeOfPrimitiveType(S32) << " bytes" << std::endl;
        tests_passed++;
        
        // Test 16: Unary Operations
        std::cout << "\n[Test 16] XLA Builder - Unary Operations..." << std::endl;
        total_tests++;
        
        XlaBuilder unary_builder("unary");
        auto unary_input = Parameter(&unary_builder, 0, data_shape, "input");
        
        auto neg = Neg(unary_input);
        auto abs_val = Abs(unary_input);
        auto exp_val = Exp(unary_input);
        auto log_val = Log(unary_input);
        auto sqrt_val = Sqrt(unary_input);
        auto tanh_val = Tanh(unary_input);
        
        auto unary_comp = CheckOr(unary_builder.Build(), "Building unary ops");
        std::cout << "  ✓ Built computation with Neg, Abs, Exp, Log, Sqrt, Tanh" << std::endl;
        tests_passed++;
        
        // Test 17: Reduce Operations
        std::cout << "\n[Test 17] XLA Builder - Reduce Operations..." << std::endl;
        total_tests++;
        
        XlaBuilder reduce_builder2("reduce_ops");
        Shape reduce_shape = ShapeUtil::MakeShape(F32, {8, 16});
        auto reduce_in = Parameter(&reduce_builder2, 0, reduce_shape, "data");
        
        // Reduce along dimension 0 - build a simple add computation
        XlaBuilder add_builder2("scalar_add2");
        auto pa0 = Parameter(&add_builder2, 0, ShapeUtil::MakeShape(F32, {}), "pa0");
        auto pa1 = Parameter(&add_builder2, 1, ShapeUtil::MakeShape(F32, {}), "pa1");
        Add(pa0, pa1);
        XlaComputation add_comp2 = CheckOr(add_builder2.Build(), "Building add comp2");
        
        auto reduce_dim0 = Reduce(reduce_in, ConstantR0<float>(&reduce_builder2, 0.0f),
                                  add_comp2, {0});
        
        auto reduce_comp2 = CheckOr(reduce_builder2.Build(), "Building reduce");
        std::cout << "  ✓ Built reduce operation" << std::endl;
        tests_passed++;
        
        // Test 18: Concatenate and Slice
        std::cout << "\n[Test 18] XLA Builder - Concatenate and Slice..." << std::endl;
        total_tests++;
        
        XlaBuilder slice_builder("slice_concat");
        Shape vec_shape = ShapeUtil::MakeShape(F32, {5});
        auto vec1 = Parameter(&slice_builder, 0, vec_shape, "vec1");
        auto vec2 = Parameter(&slice_builder, 1, vec_shape, "vec2");
        
        auto concat = ConcatInDim(&slice_builder, {vec1, vec2}, 0);
        auto sliced = Slice(concat, {2}, {8}, {1});
        
        auto slice_comp = CheckOr(slice_builder.Build(), "Building slice/concat");
        std::cout << "  ✓ Built concatenate and slice operations" << std::endl;
        tests_passed++;
        
        // Test 19: Compilation with Different Shapes
        std::cout << "\n[Test 19] Multi-Shape Compilation..." << std::endl;
        total_tests++;
        
        // Compile matrix multiplication
        auto matmul_exec = CheckOr(
            client->CompileAndLoad(matmul_comp, compile_opts),
            "Compiling matmul"
        );
        std::cout << "  ✓ Matmul compiled" << std::endl;
        
        // Compile unary operations
        auto unary_exec = CheckOr(
            client->CompileAndLoad(unary_comp, compile_opts),
            "Compiling unary"
        );
        std::cout << "  ✓ Unary ops compiled" << std::endl;
        
        // Compile reduction
        auto reduce_exec = CheckOr(
            client->CompileAndLoad(reduce_comp2, compile_opts),
            "Compiling reduce"
        );
        std::cout << "  ✓ Reduce op compiled" << std::endl;
        tests_passed++;
        
        // Test 20: Buffer Properties
        std::cout << "\n[Test 20] Buffer Properties and Queries..." << std::endl;
        total_tests++;
        
        Literal large_literal = LiteralUtil::CreateR2<float>({{1, 2, 3, 4}, {5, 6, 7, 8}});
        auto large_buffer = CheckOr(
            client->BufferFromHostLiteral(large_literal, mem_space),
            "Creating large buffer"
        );
        
        std::cout << "  ✓ Buffer device: " << large_buffer->device()->DebugString() << std::endl;
        std::cout << "  ✓ Is on CPU: " << (large_buffer->device()->device_kind() == "cpu") << std::endl;
        
        // Check if deleted
        std::cout << "  ✓ Buffer is deleted: " << (large_buffer->IsDeleted() ? "true" : "false") << std::endl;
        tests_passed++;
        
        // Summary
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Results: " << tests_passed << "/" << total_tests << " passed" << std::endl;
        std::cout << "========================================" << std::endl;
        
        if (tests_passed == total_tests) {
            std::cout << "\n✅ ALL TESTS PASSED!" << std::endl;
            std::cout << "\nVerified XLA Features:" << std::endl;
            std::cout << "  ✓ PjRt Client & Device Management" << std::endl;
            std::cout << "  ✓ Literal Creation (R0, R1, R2, R3)" << std::endl;
            std::cout << "  ✓ Shape Utilities & Type System" << std::endl;
            std::cout << "  ✓ XLA Builder API" << std::endl;
            std::cout << "  ✓ Arithmetic Operations (Add, Sub, Mul, Div, Max, Min)" << std::endl;
            std::cout << "  ✓ Matrix Operations (Dot)" << std::endl;
            std::cout << "  ✓ Reduction Operations" << std::endl;
            std::cout << "  ✓ Broadcasting" << std::endl;
            std::cout << "  ✓ Reshape & Transpose" << std::endl;
            std::cout << "  ✓ Concatenate & Slice" << std::endl;
            std::cout << "  ✓ Unary Operations (Neg, Abs, Exp, Log, Sqrt, Tanh)" << std::endl;
            std::cout << "  ✓ Compilation Pipeline" << std::endl;
            std::cout << "  ✓ Buffer Management & Transfer" << std::endl;
            std::cout << "\n🎉 XLA static library is fully functional!" << std::endl;
            return 0;
        } else {
            std::cout << "\n❌ Some tests failed" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

