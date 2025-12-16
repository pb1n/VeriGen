/**
 * Test program for Reference Model generation
 *
 * This program:
 * 1. Generates a counter AST
 * 2. Emits Verilog code using EmitVisitor
 * 3. Generates C++ reference model using ReferenceModelVisitor
 * 4. Prints both for comparison
 */

#include "../../ast/ast_visitor.hpp"
#include "../../ast/reference_model_visitor.hpp"
#include "../counter_gen.hpp"
#include <iostream>
#include <fstream>

int main() {
    std::cout << "=== Reference Model Generation Test ===\n\n";

    // Generate a simple counter (8-bit, with reset, without load)
    std::cout << "Generating counter AST...\n";
    auto counter = generators::generateCounter("test_counter", 8, true, false);

    // Generate Verilog using EmitVisitor
    std::cout << "\n--- Step 1: Generate Verilog RTL ---\n";
    ast::EmitVisitor verilog_gen;
    counter->accept(verilog_gen);
    std::string verilog_code = verilog_gen.getResult();

    std::cout << verilog_code << "\n";

    // Generate C++ Reference Model
    std::cout << "\n--- Step 2: Generate C++ Reference Model ---\n";
    ast::ReferenceModelVisitor refmodel_gen;
    counter->accept(refmodel_gen);
    std::string cpp_model = refmodel_gen.getResult();

    std::cout << cpp_model << "\n";

    // Save both to files for inspection
    std::cout << "\n--- Step 3: Save to files ---\n";

    std::ofstream verilog_file("test_counter_refmodel.v");
    verilog_file << verilog_code;
    verilog_file.close();
    std::cout << "✓ Saved Verilog to: test_counter_refmodel.v\n";

    std::ofstream cpp_file("test_counter_refmodel.hpp");
    cpp_file << "#pragma once\n";
    cpp_file << "#include <cstdint>\n\n";
    cpp_file << cpp_model;
    cpp_file.close();
    std::cout << "✓ Saved C++ model to: test_counter_refmodel.hpp\n";

    std::cout << "\n=== Test Complete ===\n";
    std::cout << "You can now inspect the generated files to verify correctness.\n";

    return 0;
}
