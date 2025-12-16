/**
 * Test reference model generation with a more complex counter (with load)
 */

#include "../../ast/ast_visitor.hpp"
#include "../../ast/reference_model_visitor.hpp"
#include "../counter_gen.hpp"
#include <iostream>
#include <fstream>

int main() {
    std::cout << "=== Complex Counter Reference Model Test ===\n\n";

    // Generate a loadable counter (8-bit, with reset, WITH load)
    std::cout << "Generating loadable counter AST...\n";
    auto counter = generators::generateCounter("loadable_counter", 8, true, true);

    // Generate Verilog
    std::cout << "\n--- Verilog RTL ---\n";
    ast::EmitVisitor verilog_gen;
    counter->accept(verilog_gen);
    std::cout << verilog_gen.getResult() << "\n";

    // Generate C++ Reference Model
    std::cout << "\n--- C++ Reference Model ---\n";
    ast::ReferenceModelVisitor refmodel_gen;
    counter->accept(refmodel_gen);
    std::string cpp_model = refmodel_gen.getResult();
    std::cout << cpp_model << "\n";

    // Save to file
    std::ofstream cpp_file("loadable_counter_refmodel.hpp");
    cpp_file << "#pragma once\n";
    cpp_file << "#include <cstdint>\n\n";
    cpp_file << cpp_model;
    cpp_file.close();
    std::cout << "✓ Saved to: loadable_counter_refmodel.hpp\n";

    std::cout << "\n=== Analysis ===\n";
    std::cout << "The generated reference model should have:\n";
    std::cout << "  - tick() parameters: bool clk, bool rst, bool load_enable, uint32_t data_in\n";
    std::cout << "  - State variables: count, prev_clk\n";
    std::cout << "  - Edge detection: (clk && !prev_clk)\n";
    std::cout << "  - Ternary logic: rst ? 0 : (load_enable ? data_in : count + 1)\n";

    return 0;
}
