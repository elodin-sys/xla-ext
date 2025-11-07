/**
 * XLA Static Library Test
 * 
 * This program tests the static XLA library by:
 * 1. Creating a PjRt CPU client
 * 2. Building a simple XLA computation (element-wise addition)
 * 3. Compiling the computation
 * 4. Executing it with real data
 * 5. Verifying the results
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
#include "xla/pjrt/pjrt_executable.h"
#include "xla/tsl/platform/statusor.h"
#include "xla/tsl/platform/status.h"
#include "absl/status/statusor.h"
#include "absl/status/status.h"
#include "absl/types/span.h"

using namespace xla;
using absl::StatusOr;
using absl::Status;

// Helper function to check StatusOr and exit on error
template<typename T>
T CheckStatusOr(StatusOr<T> status_or, const std::string& context) {
    if (!status_or.ok()) {
        std::cerr << "ERROR in " << context << ": " 
                  << status_or.status().message() << std::endl;
        exit(1);
    }
    return std::move(status_or).value();
}

// Helper function to check Status and exit on error
void CheckStatus(const Status& status, const std::string& context) {
    if (!status.ok()) {
        std::cerr << "ERROR in " << context << ": " 
                  << status.message() << std::endl;
        exit(1);
    }
}

// Test 1: Create PjRt CPU Client
std::unique_ptr<PjRtClient> CreateCpuClient() {
    std::cout << "Test 1: Creating PjRt CPU client..." << std::endl;
    
    CpuClientOptions options;
    options.asynchronous = true;
    options.cpu_device_count = 1;
    
    auto client = CheckStatusOr(
        GetPjRtCpuClient(options),
        "Creating CPU client"
    );
    
    std::cout << "  ✓ CPU client created successfully" << std::endl;
    std::cout << "  ✓ Device count: " << client->device_count() << std::endl;
    std::cout << "  ✓ Addressable device count: " 
              << client->addressable_device_count() << std::endl;
    
    return client;
}

// Test 2: Build a simple XLA computation (element-wise add)
XlaComputation BuildAddComputation() {
    std::cout << "\nTest 2: Building XLA computation (a + b)..." << std::endl;
    
    XlaBuilder builder("add_computation");
    
    // Define input shapes: [4] arrays of f32
    Shape input_shape = ShapeUtil::MakeShape(F32, {4});
    
    // Create parameters
    auto a = Parameter(&builder, 0, input_shape, "a");
    auto b = Parameter(&builder, 1, input_shape, "b");
    
    // Perform addition
    Add(a, b);
    
    auto computation = CheckStatusOr(
        builder.Build(),
        "Building computation"
    );
    
    std::cout << "  ✓ Computation built successfully" << std::endl;
    return computation;
}

// Test 3: Compile and load the computation
std::unique_ptr<PjRtLoadedExecutable> CompileComputation(
    PjRtClient* client,
    const XlaComputation& computation) {
    
    std::cout << "\nTest 3: Compiling and loading computation..." << std::endl;
    
    CompileOptions options;
    options.executable_build_options.set_num_replicas(1);
    options.executable_build_options.set_num_partitions(1);
    
    // Use CompileAndLoad to get PjRtLoadedExecutable which has Execute methods
    auto executable = CheckStatusOr(
        client->CompileAndLoad(computation, options),
        "Compiling and loading computation"
    );
    
    std::cout << "  ✓ Compilation successful" << std::endl;
    std::cout << "  ✓ Executable name: " << executable->name() << std::endl;
    
    return executable;
}

// Test 4: Execute the computation with real data
void ExecuteAndVerify(
    PjRtClient* client,
    PjRtLoadedExecutable* executable) {
    
    std::cout << "\nTest 4: Executing computation..." << std::endl;
    
    // Create input data: a = [1.0, 2.0, 3.0, 4.0], b = [10.0, 20.0, 30.0, 40.0]
    std::vector<float> a_data = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> b_data = {10.0f, 20.0f, 30.0f, 40.0f};
    std::vector<float> expected = {11.0f, 22.0f, 33.0f, 44.0f};
    
    std::cout << "  Input a: [";
    for (size_t i = 0; i < a_data.size(); i++) {
        std::cout << a_data[i];
        if (i < a_data.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    std::cout << "  Input b: [";
    for (size_t i = 0; i < b_data.size(); i++) {
        std::cout << b_data[i];
        if (i < b_data.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Create literals
    Literal a_literal = LiteralUtil::CreateR1<float>(a_data);
    Literal b_literal = LiteralUtil::CreateR1<float>(b_data);
    
    // Transfer to device
    auto device = client->addressable_devices()[0];
    auto memory_space = CheckStatusOr(
        device->default_memory_space(),
        "Getting default memory space"
    );
    
    auto a_buffer = CheckStatusOr(
        client->BufferFromHostLiteral(a_literal, memory_space),
        "Transferring a to device"
    );
    
    auto b_buffer = CheckStatusOr(
        client->BufferFromHostLiteral(b_literal, memory_space),
        "Transferring b to device"
    );
    
    // Execute with multi-replica API (one replica with multiple arguments)
    std::vector<std::vector<PjRtBuffer*>> argument_handles = {
        {a_buffer.get(), b_buffer.get()}
    };
    
    ExecuteOptions execute_options;
    auto results = CheckStatusOr(
        executable->Execute(argument_handles, execute_options),
        "Executing computation"
    );
    
    std::cout << "  ✓ Execution successful" << std::endl;
    
    // Get result back to host
    auto result_literal = CheckStatusOr(
        results[0][0]->ToLiteralSync(),
        "Transferring result to host"
    );
    
    // Verify results
    std::cout << "  Result:  [";
    bool all_correct = true;
    for (int i = 0; i < 4; i++) {
        float value = result_literal->data<float>()[i];
        std::cout << value;
        if (i < 3) std::cout << ", ";
        
        if (std::abs(value - expected[i]) > 1e-5) {
            all_correct = false;
        }
    }
    std::cout << "]" << std::endl;
    
    if (all_correct) {
        std::cout << "  ✓ Results verified correct!" << std::endl;
    } else {
        std::cerr << "  ✗ Results incorrect!" << std::endl;
        exit(1);
    }
}

// Test 5: Test with matrix multiplication
XlaComputation BuildMatMulComputation() {
    std::cout << "\nTest 5: Building matrix multiplication computation..." << std::endl;
    
    XlaBuilder builder("matmul_computation");
    
    // Define shapes: [2, 3] x [3, 2] = [2, 2]
    Shape a_shape = ShapeUtil::MakeShape(F32, {2, 3});
    Shape b_shape = ShapeUtil::MakeShape(F32, {3, 2});
    
    auto a = Parameter(&builder, 0, a_shape, "a");
    auto b = Parameter(&builder, 1, b_shape, "b");
    
    // Matrix multiplication
    Dot(a, b);
    
    auto computation = CheckStatusOr(
        builder.Build(),
        "Building matmul computation"
    );
    
    std::cout << "  ✓ Matrix multiplication computation built" << std::endl;
    return computation;
}

void ExecuteMatMul(PjRtClient* client, PjRtLoadedExecutable* executable) {
    std::cout << "  Executing matrix multiplication..." << std::endl;
    
    // A = [[1, 2, 3],    B = [[1, 2],
    //      [4, 5, 6]]         [3, 4],
    //                         [5, 6]]
    // Result should be [[22, 28], [49, 64]]
    
    Literal a_literal = LiteralUtil::CreateR2<float>({{1, 2, 3}, {4, 5, 6}});
    Literal b_literal = LiteralUtil::CreateR2<float>({{1, 2}, {3, 4}, {5, 6}});
    
    auto device = client->addressable_devices()[0];
    auto memory_space = CheckStatusOr(
        device->default_memory_space(),
        "Getting default memory space"
    );
    
    auto a_buffer = CheckStatusOr(
        client->BufferFromHostLiteral(a_literal, memory_space),
        "Transferring a to device"
    );
    
    auto b_buffer = CheckStatusOr(
        client->BufferFromHostLiteral(b_literal, memory_space),
        "Transferring b to device"
    );
    
    // Execute with multi-replica API (one replica with multiple arguments)
    std::vector<std::vector<PjRtBuffer*>> argument_handles = {
        {a_buffer.get(), b_buffer.get()}
    };
    
    ExecuteOptions execute_options;
    auto results = CheckStatusOr(
        executable->Execute(argument_handles, execute_options),
        "Executing matmul"
    );
    
    auto result_literal = CheckStatusOr(
        results[0][0]->ToLiteralSync(),
        "Transferring result to host"
    );
    
    std::cout << "  Result matrix:" << std::endl;
    std::cout << "    [[" << result_literal->data<float>()[0] 
              << ", " << result_literal->data<float>()[1] << "]," << std::endl;
    std::cout << "     [" << result_literal->data<float>()[2] 
              << ", " << result_literal->data<float>()[3] << "]]" << std::endl;
    
    // Verify: [[22, 28], [49, 64]]
    float expected[] = {22.0f, 28.0f, 49.0f, 64.0f};
    bool correct = true;
    for (int i = 0; i < 4; i++) {
        if (std::abs(result_literal->data<float>()[i] - expected[i]) > 1e-5) {
            correct = false;
        }
    }
    
    if (correct) {
        std::cout << "  ✓ Matrix multiplication verified correct!" << std::endl;
    } else {
        std::cerr << "  ✗ Matrix multiplication incorrect!" << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "XLA Static Library Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // Test 1: Create client
        auto client = CreateCpuClient();
        
        // Test 2-4: Simple addition
        auto add_computation = BuildAddComputation();
        auto add_executable = CompileComputation(client.get(), add_computation);
        ExecuteAndVerify(client.get(), add_executable.get());
        
        // Test 5: Matrix multiplication
        auto matmul_computation = BuildMatMulComputation();
        auto matmul_executable = CompileComputation(client.get(), matmul_computation);
        ExecuteMatMul(client.get(), matmul_executable.get());
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "✓ All tests passed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

