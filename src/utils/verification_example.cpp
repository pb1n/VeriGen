/**
 * Example showing how to use monitor_parser and reference_model_visitor
 * for verification
 *
 * This is a SKELETON/GUIDE - not meant to compile yet!
 * Shows the intended usage pattern once you implement the TODOs.
 */

#include "monitor_parser.hpp"
#include "../ast/reference_model_visitor.hpp"
#include "../ast/ast_visitor.hpp"
#include "../generators/counter_gen.hpp"
#include <iostream>

// Example of a manually-written reference model (for comparison)
class CounterRefModel {
private:
    uint32_t count;
    bool prev_clk;

public:
    CounterRefModel() : count(0), prev_clk(false) {}

    void tick(bool clk, bool rst) {
        // Detect posedge clk
        if (clk && !prev_clk) {
            count = rst ? 0 : (count + 1);
        }
        prev_clk = clk;
    }

    uint32_t getCount() const { return count; }
};

void exampleUsage() {
    std::cout << "=== Reference Model Verification Example ===\n\n";

    // TODO STEP 1: Generate AST for a counter
    std::cout << "Step 1: Generate counter AST\n";
    auto counter_ast = generators::generateCounter("test_counter", 8, true);

    // TODO STEP 2: Generate Verilog from AST
    std::cout << "Step 2: Generate Verilog RTL\n";
    ast::EmitVisitor verilog_gen;
    counter_ast->accept(verilog_gen);
    std::string verilog_code = verilog_gen.getResult();
    std::cout << "Generated Verilog:\n" << verilog_code << "\n\n";

    // TODO STEP 3: Generate C++ reference model from same AST
    std::cout << "Step 3: Generate C++ reference model\n";
    ast::ReferenceModelVisitor model_gen;
    counter_ast->accept(model_gen);
    std::string cpp_model = model_gen.getResult();
    std::cout << "Generated C++ model:\n" << cpp_model << "\n\n";

    // TODO STEP 4: Simulate Verilog (using IcarusSimulator)
    std::cout << "Step 4: Simulate Verilog\n";
    // IcarusSimulator sim("sim_work");
    // auto result = sim.simulateCounter(...);
    std::string simulation_output =
        "Time=0 clk=0 rst=1 count=  x\n"
        "Time=5000 clk=1 rst=1 count=  0\n"
        "Time=10000 clk=0 rst=1 count=  0\n"
        "Time=15000 clk=1 rst=1 count=  0\n"
        "Time=20000 clk=0 rst=0 count=  0\n"
        "Time=25000 clk=1 rst=0 count=  1\n"
        "Time=30000 clk=0 rst=0 count=  1\n"
        "Time=35000 clk=1 rst=0 count=  2\n";

    // TODO STEP 5: Parse monitor output
    std::cout << "Step 5: Parse simulation monitor output\n";
    auto cycles = verification::parseMonitorOutput(simulation_output);
    std::cout << "Parsed " << cycles.size() << " cycles\n\n";

    // TODO STEP 6: Verify cycle-by-cycle
    std::cout << "Step 6: Verify against reference model\n";
    CounterRefModel ref_model;

    for (const auto& cycle : cycles) {
        // Get inputs from simulation
        bool clk = cycle.signals.count("clk") ? cycle.signals.at("clk") : 0;
        bool rst = cycle.signals.count("rst") ? cycle.signals.at("rst") : 0;

        // Get expected value from reference model
        uint32_t expected = ref_model.getCount();

        // Get actual value from simulation
        uint32_t actual = cycle.signals.count("count") ? cycle.signals.at("count") : 0;

        // Compare (skip 'x' values at time 0)
        if (cycle.time > 0) {
            if (actual != expected) {
                std::cerr << "MISMATCH at time " << cycle.time
                          << ": expected=" << expected
                          << ", actual=" << actual << "\n";
                return;
            }
            std::cout << "  Time=" << cycle.time
                      << " clk=" << clk
                      << " rst=" << rst
                      << " count=" << actual
                      << " ✓\n";
        }

        // Step reference model forward
        ref_model.tick(clk, rst);
    }

    std::cout << "\n✓ All cycles verified successfully!\n";
}

// This is just a demonstration - won't compile until TODOs are implemented
int main() {
    std::cout << "This is a SKELETON example showing intended usage.\n";
    std::cout << "Implement the TODOs in monitor_parser.cpp and reference_model_visitor.cpp first!\n\n";

    // exampleUsage();  // Uncomment once TODOs are done

    return 0;
}
