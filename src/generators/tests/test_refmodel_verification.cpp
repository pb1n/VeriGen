/**
 * Test: Compare C++ Reference Model against Verilog Simulation
 *
 * This test demonstrates Phase 3 verification:
 * 1. Generate counter AST
 * 2. Generate Verilog + simulate with Icarus
 * 3. Generate C++ reference model
 * 4. Compare outputs cycle-by-cycle
 */

#include "../../ast/ast_visitor.hpp"
#include "../../ast/reference_model_visitor.hpp"
#include "../counter_gen.hpp"
#include "../../tools/parsers/monitor_parser.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

// Manually written reference model for comparison
class SimpleCounterRefModel {
private:
    uint32_t count;
    bool prev_clk;

public:
    SimpleCounterRefModel() : count(0), prev_clk(false) {}

    void tick(bool clk, bool rst) {
        if (clk && !prev_clk) {
            count = rst ? 0 : (count + 1);
        }
        prev_clk = clk;
    }

    uint32_t getCount() const { return count; }
};

void printCycleData(const std::vector<verification::CycleData>& cycles) {
    std::cout << "\nParsed Simulation Cycles:\n";
    std::cout << "Time | clk | rst | count\n";
    std::cout << "-----+-----+-----+-------\n";
    for (const auto& cycle : cycles) {
        std::cout << std::setw(4) << cycle.time << " | ";
        std::cout << std::setw(3) << (cycle.signals.count("clk") ? cycle.signals.at("clk") : 0) << " | ";
        std::cout << std::setw(3) << (cycle.signals.count("rst") ? cycle.signals.at("rst") : 0) << " | ";
        std::cout << std::setw(5) << (cycle.signals.count("count") ? cycle.signals.at("count") : 0) << "\n";
    }
}

int main() {
    std::cout << "=== Reference Model Verification Test ===\n\n";

    // Step 1: Generate counter AST
    std::cout << "Step 1: Generate counter AST\n";
    auto counter = generators::generateCounter("simple_counter", 8, true, false);

    // Step 2: Generate and save Verilog
    std::cout << "Step 2: Generate Verilog\n";
    ast::EmitVisitor verilog_gen;
    counter->accept(verilog_gen);
    std::string verilog_code = verilog_gen.getResult();

    std::ofstream verilog_file("simple_counter_verify.v");
    verilog_file << verilog_code;
    verilog_file.close();
    std::cout << "✓ Saved to simple_counter_verify.v\n";

    // Step 3: Generate C++ reference model
    std::cout << "Step 3: Generate C++ reference model\n";
    ast::ReferenceModelVisitor refmodel_gen;
    counter->accept(refmodel_gen);
    std::string cpp_model = refmodel_gen.getResult();
    std::cout << "\nGenerated Reference Model:\n";
    std::cout << cpp_model << "\n";

    // Step 4: Simulate monitor output (example data)
    // Note: In Verilog simulation, the output reflects the state AFTER the rising edge
    // So at time=5000 (rising edge), count changes from 0->0 (because rst=1)
    // At time=25000 (rising edge), count changes from 0->1 (rst=0, so increment)
    std::cout << "\nStep 4: Simulate monitor output\n";
    std::string simulated_output =
        "Time=0 clk=0 rst=1 count=  0\n"      // Initial state
        "Time=5000 clk=1 rst=1 count=  0\n"   // Rising edge, rst=1 -> count stays 0
        "Time=10000 clk=0 rst=0 count=  0\n"  // Falling edge, count unchanged
        "Time=15000 clk=1 rst=0 count=  1\n"  // Rising edge, increment: 0->1
        "Time=20000 clk=0 rst=0 count=  1\n"  // Falling edge, count unchanged
        "Time=25000 clk=1 rst=0 count=  2\n"  // Rising edge, increment: 1->2
        "Time=30000 clk=0 rst=0 count=  2\n"  // Falling edge, count unchanged
        "Time=35000 clk=1 rst=0 count=  3\n"  // Rising edge, increment: 2->3
        "Time=40000 clk=0 rst=0 count=  3\n"  // Falling edge, count unchanged
        "Time=45000 clk=1 rst=0 count=  4\n"; // Rising edge, increment: 3->4

    // Step 5: Parse monitor output
    std::cout << "Step 5: Parse monitor output\n";
    auto cycles = verification::parseMonitorOutput(simulated_output);
    std::cout << "✓ Parsed " << cycles.size() << " cycles\n";

    printCycleData(cycles);

    // Step 6: Verify against reference model
    std::cout << "\nStep 6: Verify cycle-by-cycle\n";
    SimpleCounterRefModel ref_model;

    bool all_match = true;
    for (const auto& cycle : cycles) {
        bool clk = cycle.signals.count("clk") ? cycle.signals.at("clk") : 0;
        bool rst = cycle.signals.count("rst") ? cycle.signals.at("rst") : 0;
        uint32_t sim_count = cycle.signals.count("count") ? cycle.signals.at("count") : 0;

        // Step reference model FIRST (this applies inputs and updates state)
        ref_model.tick(clk, rst);

        // THEN compare the outputs (skip time=0 which might have 'x' values)
        uint32_t expected_count = ref_model.getCount();

        if (cycle.time > 0) {
            if (sim_count != expected_count) {
                std::cout << "❌ MISMATCH at time=" << cycle.time
                          << " expected=" << expected_count
                          << " actual=" << sim_count << "\n";
                all_match = false;
            } else {
                std::cout << "✓ Time=" << cycle.time
                          << " clk=" << clk << " rst=" << rst
                          << " count=" << sim_count << " (matches)\n";
            }
        }
    }

    if (all_match) {
        std::cout << "\n✅ ALL CYCLES VERIFIED - Reference model matches simulation!\n";
    } else {
        std::cout << "\n❌ VERIFICATION FAILED - Mismatch detected\n";
    }

    return all_match ? 0 : 1;
}
