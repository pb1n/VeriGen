/**
 * Random Expression Generator for new AST
 *
 * Generates random combinational expressions for fuzzing/verification.
 * Similar to legacy/ast.hpp but adapted for the new visitor-based AST.
 *
 * Features:
 * - Random binary operations (Add, Xor, And, Or)
 * - Random expression trees of configurable depth
 * - Evaluation function for golden value computation
 */

#pragma once

#include "../ast/ast_expr.hpp"
#include "../ast/ast_module.hpp"
#include <random>
#include <vector>
#include <cstdint>

namespace generators {

class RandomExprGenerator {
public:
    RandomExprGenerator(unsigned seed = 42)
        : rng_(seed), seed_(seed) {}

    /**
     * Generate a random combinational expression tree
     *
     * @param depth Maximum tree depth
     * @param num_inputs Number of input variables
     * @param input_names Names of input variables (e.g., {"a", "b", "c"})
     * @return Random expression AST node
     */
    ast::Ptr<ast::Expr> generateRandomExpr(
        int depth,
        int num_inputs,
        const std::vector<std::string>& input_names
    );

    /**
     * Generate a complete combinational module with random logic
     *
     * @param module_name Name of the module
     * @param num_inputs Number of input ports
     * @param expr_depth Depth of random expression tree
     * @return Module with random combinational logic
     */
    ast::Ptr<ast::ModuleDefn> generateRandomModule(
        const std::string& module_name,
        int num_inputs,
        int expr_depth
    );

    /**
     * Evaluate expression with given input values
     * Returns the golden value for verification
     *
     * @param expr Expression to evaluate
     * @param input_values Values for each input variable
     * @return Evaluated result
     */
    uint32_t evaluateExpr(
        const ast::Expr* expr,
        const std::vector<uint32_t>& input_values
    );

    unsigned getSeed() const { return seed_; }

private:
    std::mt19937 rng_;
    unsigned seed_;

    // Helper: Generate random expression node
    ast::Ptr<ast::Expr> generateNode(
        int remaining_depth,
        int num_inputs,
        const std::vector<std::string>& input_names
    );

    // Helper: Choose random binary operator
    ast::BinaryExpr::Op randomBinaryOp();

    // Helper: Choose random operand count (2-4 for n-ary expressions)
    int randomOperandCount();
};

} // namespace generators
