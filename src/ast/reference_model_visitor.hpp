#pragma once

#include "ast_visitor.hpp"
#include "ast_module.hpp"
#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_decl.hpp"
#include <string>
#include <vector>
#include <sstream>

namespace ast {

/**
 * Visitor that generates a C++ reference model from Verilog AST
 *
 * Purpose: Create a behavioral C++ model that can be executed alongside
 * simulation to verify correctness cycle-by-cycle.
 *
 * Example transformation:
 *   Verilog: always @(posedge clk) count <= rst ? 0 : count + 1;
 *   C++:     void tick(bool clk, bool rst) {
 *                if (clk && !prev_clk) { count = rst ? 0 : count + 1; }
 *                prev_clk = clk;
 *            }
 */
class ReferenceModelVisitor : public BaseVisitor {
public:
    ReferenceModelVisitor() : indent_level_(0) {}

    /**
     * Get the generated C++ reference model code
     * @return Complete C++ class definition as string
     */
    std::string getResult() const { return result_; }

    // ===== Visitor methods (override from BaseVisitor) =====

    /**
     * Generate reference model from module
     *
     * TODO: Implement module translation
     * - Generate class with module name + "RefModel"
     * - Extract input ports → tick() parameters
     * - Extract output ports → getter methods
     * - Extract reg declarations → class member variables
     * - Extract always blocks → tick() method body
     * - Extract assign statements → compute() method or tick() assignments
     */
    void visit(ModuleDefn& node) override;

    /**
     * Translate Verilog expressions to C++
     *
     * TODO: Implement expression translation for each type
     */
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(ConditionalExpr& node) override;  // Note: ConditionalExpr not CondExpr!
    void visit(LiteralExpr& node) override;
    void visit(VarExpr& node) override;

    /**
     * Translate always blocks to tick() method
     *
     * TODO: Implement always block translation
     * - Extract sensitivity list (posedge clk, negedge rst, etc.)
     * - Generate edge detection code (clk && !prev_clk for posedge)
     * - Translate body statement to C++
     * - Handle blocking (=) vs non-blocking (<=) assignments
     */
    void visit(AlwaysStmt& node) override;

    /**
     * Translate continuous assignments
     *
     * TODO: Implement assign translation
     * - For combinational: generate compute() method or inline assignment
     * - For mixed: add to tick() method
     */
    void visit(AssignStmt& node) override;

    /**
     * Track register declarations as class members
     *
     * TODO: Implement variable declaration tracking
     * - Identify reg vs wire (reg → state variable, wire → combinational)
     * - Add to state_vars_ list
     * - Generate member variable declaration
     */
    void visit(VarDecl& node) override;

private:
    std::string result_;                      // Generated C++ code
    int indent_level_;                        // Current indentation level

    // Tracking for code generation
    std::vector<std::string> state_vars_;     // Register names (outputs/regs)
    std::vector<std::string> input_vars_;     // Input port names
    std::string tick_body_;                   // Body of tick() method
    std::string class_name_;                  // Generated class name

    // FIX 1: Track non-blocking assignments
    std::vector<std::string> nonblocking_vars_;  // Variables with non-blocking assignments

    // FIX 2: Track signals used in edge detection
    std::vector<std::string> prev_signals_;   // Signals needing prev_ copies

    /**
     * Helper: Translate expression recursively to C++ syntax
     */
    std::string translateExpr(Expr* expr);

    /**
     * Helper: Generate indentation string
     */
    std::string indent() const;

    /**
     * Helper: Choose appropriate C++ type based on bit width
     * FIX 4: Proper type selection (uint8_t, uint16_t, uint32_t, uint64_t)
     */
    std::string chooseCppType(int width);

    /**
     * Helper: Register a signal for edge detection (adds prev_ variable)
     * FIX 2: Centralized tracking of prev_ signals
     */
    void registerPrevSignal(const std::string& signal);

    /**
     * Helper: Translate Verilog type to C++ type (DEPRECATED)
     */
    std::string translateType(const std::string& verilog_type);
};

} // namespace ast
