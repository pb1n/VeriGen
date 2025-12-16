#include "../counter_gen.hpp"
#include "../../ast/ast_visitor.hpp"
// Legacy include removed
#include <iostream>
#include <fstream>
#include <sstream>

void saveModule(const ast::Ptr<ast::ModuleDefn>& module, const std::string& filename, const std::string& description) {
    std::cout << "=== " << description << " ===\n" << std::flush;

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing\n" << std::flush;
        return;
    }

    // Use visitor pattern for code generation
    ast::EmitVisitor visitor;
    module->accept(visitor);
    std::string verilog = visitor.getResult();

    file << verilog;
    file.close();

    std::cout << "Saved to: " << filename << "\n";
    std::cout << verilog << "\n" << std::flush;
}

void simulateCounter(
    IcarusSimulator& sim,
    const std::string& filename,
    const std::string& module_name,
    int width,
    bool has_reset,
    bool has_load
) {
    std::cout << "  Simulating...\n" << std::flush;
    auto result = sim.simulateCounter(filename, module_name, width, has_reset, has_load, 10);

    if (result.success) {
        std::cout << "  ✓ Simulation PASSED\n";
        if (!result.output.empty()) {
            std::cout << "  Output:\n";
            std::istringstream iss(result.output);
            std::string line;
            while (std::getline(iss, line)) {
                std::cout << "    " << line << "\n";
            }
        }
    } else {
        std::cout << "  ✗ Simulation FAILED\n";
        std::cout << "  Check log: " << result.log_file << "\n";
    }
    std::cout << "\n" << std::flush;
}

int main() {
    std::cout << "Starting test...\n" << std::flush;

    IcarusSimulator sim("generatedFiles/sim_work", false);

    std::cout << "Generating and simulating counter modules...\n\n" << std::flush;

    // Test basic counter with reset
    auto counter1 = generators::generateCounter("basic_counter", 8, true);
    saveModule(counter1, "generatedFiles/verilog/basic_counter.v", "Basic Counter WITH reset");
    simulateCounter(sim, "generatedFiles/verilog/basic_counter.v", "basic_counter", 8, true, false);

    // Test counter without reset
    auto counter2 = generators::generateCounter("simple_counter", 16, false);
    saveModule(counter2, "generatedFiles/verilog/simple_counter.v", "Basic Counter WITHOUT reset");
    simulateCounter(sim, "generatedFiles/verilog/simple_counter.v", "simple_counter", 16, false, false);

    // Test counter with reset and load
    auto counter3 = generators::generateCounter("loadable_counter", 8, true, true);
    saveModule(counter3, "generatedFiles/verilog/loadable_counter.v", "Counter WITH reset AND load");
    simulateCounter(sim, "generatedFiles/verilog/loadable_counter.v", "loadable_counter", 8, true, true);

    // Test counter with load but no reset
    auto counter4 = generators::generateCounter("loadable_no_reset", 8, false, true);
    saveModule(counter4, "generatedFiles/verilog/loadable_no_reset.v", "Counter WITH load, WITHOUT reset");
    simulateCounter(sim, "generatedFiles/verilog/loadable_no_reset.v", "loadable_no_reset", 8, false, true);

    // Test down-counter with reset and load
    auto counter5 = generators::generateCounter("down_counter", 8, true, true, true);
    saveModule(counter5, "generatedFiles/verilog/down_counter.v", "Down-Counter WITH reset AND load");
    simulateCounter(sim, "generatedFiles/verilog/down_counter.v", "down_counter", 8, true, true);

    // Test down-counter without reset or load
    auto counter6 = generators::generateCounter("simple_down", 8, false, false, true);
    saveModule(counter6, "generatedFiles/verilog/simple_down.v", "Down-Counter WITHOUT reset or load");
    simulateCounter(sim, "generatedFiles/verilog/simple_down.v", "simple_down", 8, false, false);

    std::cout << "All modules generated and simulated!\n";
    return 0;
}