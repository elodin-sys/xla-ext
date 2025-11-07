/**
 * Simple XLA Static Library Test
 * 
 * This is a simplified test that just verifies:
 * 1. The static library links correctly
 * 2. We can create a PjRt CPU client
 * 3. Basic XLA types and functions work
 */

#include <iostream>
#include <memory>

// XLA includes
#include "xla/pjrt/pjrt_client.h"
#include "xla/pjrt/cpu/cpu_client.h"
#include "xla/literal.h"
#include "xla/literal_util.h"
#include "xla/shape_util.h"
#include "absl/status/statusor.h"
#include "absl/status/status.h"

using namespace xla;
using absl::StatusOr;
using absl::Status;

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "XLA Static Library Simple Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Test 1: Create PjRt CPU Client
    std::cout << "\nTest 1: Creating PjRt CPU client..." << std::endl;
    
    CpuClientOptions options;
    options.asynchronous = true;
    options.cpu_device_count = 1;
    
    StatusOr<std::unique_ptr<PjRtClient>> client_or = GetPjRtCpuClient(options);
    
    if (!client_or.ok()) {
        std::cerr << "  ✗ Failed to create CPU client: " 
                  << client_or.status().message() << std::endl;
        return 1;
    }
    
    auto client = std::move(client_or).value();
    
    std::cout << "  ✓ CPU client created successfully" << std::endl;
    std::cout << "  ✓ Device count: " << client->device_count() << std::endl;
    std::cout << "  ✓ Addressable device count: " 
              << client->addressable_device_count() << std::endl;
    std::cout << "  ✓ Platform name: " << client->platform_name() << std::endl;
    std::cout << "  ✓ Platform version: " << client->platform_version() << std::endl;
    
    // Test 2: Create a simple literal
    std::cout << "\nTest 2: Creating literals..." << std::endl;
    
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    Literal literal = LiteralUtil::CreateR1<float>(data);
    
    std::cout << "  ✓ Created R1 literal with shape: " 
              << literal.shape().ToString() << std::endl;
    std::cout << "  ✓ Literal data: [";
    for (size_t i = 0; i < data.size(); i++) {
        std::cout << literal.data<float>()[i];
        if (i < data.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Test 3: Test shape utilities
    std::cout << "\nTest 3: Testing shape utilities..." << std::endl;
    
    Shape shape = ShapeUtil::MakeShape(F32, {2, 3});
    std::cout << "  ✓ Created shape: " << shape.ToString() << std::endl;
    std::cout << "  ✓ Element type: " << PrimitiveType_Name(shape.element_type()) << std::endl;
    std::cout << "  ✓ Rank: " << shape.dimensions_size() << std::endl;
    std::cout << "  ✓ Dimensions: [";
    for (int i = 0; i < shape.dimensions_size(); i++) {
        std::cout << shape.dimensions(i);
        if (i < shape.dimensions_size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Test 4: Get device information
    std::cout << "\nTest 4: Getting device information..." << std::endl;
    
    if (client->addressable_device_count() > 0) {
        auto device = client->addressable_devices()[0];
        std::cout << "  ✓ Device ID: " << device->id() << std::endl;
        std::cout << "  ✓ Device kind: " << device->device_kind() << std::endl;
        std::cout << "  ✓ ToString: " << device->DebugString() << std::endl;
        
        // Check memory space
        auto memory_space_or = device->default_memory_space();
        if (memory_space_or.ok()) {
            auto memory_space = memory_space_or.value();
            std::cout << "  ✓ Default memory space available" << std::endl;
            std::cout << "  ✓ Memory space kind: " << memory_space->kind() << std::endl;
        } else {
            std::cout << "  ⚠ Could not get default memory space" << std::endl;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "✓ All tests passed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nThe XLA static library is working correctly!" << std::endl;
    
    return 0;
}

