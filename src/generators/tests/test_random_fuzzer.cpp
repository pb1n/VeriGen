/**
 * Random Expression Fuzzer with Reference Model Verification
 *
 * This test demonstrates Phase 3 complete workflow:
 * 1. Generate random combinational expression
 * 2. Generate Verilog + C++ reference model from same AST
 * 3. Compare outputs for verification
 *
 * Note: This is a DEMONSTRATION - actual Icarus simulation would require
 * more infrastructure. For now, we verify the generation works correctly.
 */

#include "../../ast/ast_visitor.hpp"
#include "../../ast/reference_model_visitor.hpp"
#include "../random_expr_gen.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

int main() {
    std::cout << "=== Random Expression Fuzzer ===\n\n";

    // Configuration
    const unsigned seed = 12345;
    const int num_inputs = 4;
    const int expr_depth = 3;
    const std::string module_name = "random_comb";

    // Step 1: Generate random combinational module
    std::cout << "Step 1: Generate random combinational module\n";
    std::cout << "  - Seed: " << seed << "\n";
    std::cout << "  - Inputs: " << num_inputs << "\n";
    std::cout << "  - Expression depth: " << expr_depth << "\n\n";

    generators::RandomExprGenerator gen(seed);
    auto module = gen.generateRandomModule(module_name, num_inputs, expr_depth);

    // Step 2: Generate Verilog
    std::cout << "Step 2: Generate Verilog RTL\n";
    ast::EmitVisitor verilog_gen;
    module->accept(verilog_gen);
    std::string verilog_code = verilog_gen.getResult();

    std::cout << "Generated Verilog:\n";
    std::cout << "```verilog\n" << verilog_code << "```\n\n";

    // Save to file
    std::ofstream verilog_file("random_comb.v");
    verilog_file << verilog_code;
    verilog_file.close();
    std::cout << "✓ Saved to random_comb.v\n\n";

    // Step 3: Generate C++ reference model
    std::cout << "Step 3: Generate C++ Reference Model\n";
    ast::ReferenceModelVisitor refmodel_gen;
    module->accept(refmodel_gen);
    std::string cpp_model = refmodel_gen.getResult();

    std::cout << "Generated C++ Model:\n";
    std::cout << "```cpp\n" << cpp_model << "```\n\n";

    // Save to file
    std::ofstream cpp_file("random_comb_refmodel.hpp");
    cpp_file << "#pragma once\n#include <cstdint>\n\n";
    cpp_file << cpp_model;
    cpp_file.close();
    std::cout << "✓ Saved to random_comb_refmodel.hpp\n\n";

    // Step 4: Demonstrate golden value evaluation
    std::cout << "Step 4: Evaluate with test inputs\n";
    std::cout << "This demonstrates the 'golden value' approach from legacy system.\n\n";

    // Get the expression from the assign statement
    const auto& stmts = module->getStmts();
    if (stmts.empty()) {
        std::cerr << "Error: No statements in module\n";
        return 1;
    }

    // Test with some input values
    std::vector<std::vector<uint32_t>> test_cases = {
        {0, 0, 0, 0},
        {1, 2, 3, 4},
        {255, 128, 64, 32},
        {0xFFFFFFFF, 0, 0xAAAAAAAA, 0x55555555}
    };

    std::cout << "Test Cases:\n";
    std::cout << "in0      | in1      | in2      | in3      | Expected Output\n";
    std::cout << "---------|----------|----------|----------|----------------\n";

    for (const auto& inputs : test_cases) {
        // Print inputs
        for (size_t i = 0; i < inputs.size(); ++i) {
            std::cout << std::setw(8) << std::hex << inputs[i] << " | ";
        }

        // Evaluate using AST's eval() method
        // Note: This requires the expression to have the eval() method implemented
        std::cout << "(eval not yet implemented)\n";
    }

    std::cout << "\n=== Summary ===\n";
    std::cout << "✅ Random expression generated successfully\n";
    std::cout << "✅ Verilog RTL generated\n";
    std::cout << "✅ C++ reference model generated\n";
    std::cout << "\nNext steps for full fuzzing:\n";
    std::cout << "1. Simulate random_comb.v with Icarus Verilog\n";
    std::cout << "2. Parse simulation output\n";
    std::cout << "3. Compare against reference model or golden value\n";
    std::cout << "4. Report any mismatches\n";

    return 0;
}
