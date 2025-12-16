/**
 * Random Expression Generator Implementation
 */

#include "random_expr_gen.hpp"
#include "../ast/ast_types.hpp"
#include "../ast/ast_decl.hpp"
#include <stdexcept>

namespace generators {

ast::BinaryExpr::Op RandomExprGenerator::randomBinaryOp() {
    // Use only Add and Xor for now (like legacy)
    static const std::vector<ast::BinaryExpr::Op> ops = {
        ast::BinaryExpr::Op::Add,
        ast::BinaryExpr::Op::Xor,
        ast::BinaryExpr::Op::And,
        ast::BinaryExpr::Op::Or
    };

    std::uniform_int_distribution<size_t> dist(0, ops.size() - 1);
    return ops[dist(rng_)];
}

int RandomExprGenerator::randomOperandCount() {
    std::uniform_int_distribution<int> dist(2, 4);  // 2-4 operands
    return dist(rng_);
}

ast::Ptr<ast::Expr> RandomExprGenerator::generateNode(
    int remaining_depth,
    int num_inputs,
    const std::vector<std::string>& input_names
) {
    // Base case: generate a leaf node (literal or variable)
    if (remaining_depth <= 0) {
        std::uniform_int_distribution<int> choice(0, 1);

        if (choice(rng_) == 0 || num_inputs == 0) {
            // Generate random literal
            std::uniform_int_distribution<uint32_t> val_dist(0, 255);
            uint32_t value = val_dist(rng_);
            return ast::make<ast::LiteralExpr>(value, 32);
        } else {
            // Generate variable reference
            std::uniform_int_distribution<int> var_dist(0, num_inputs - 1);
            int var_idx = var_dist(rng_);
            return ast::make<ast::VarExpr>(input_names[var_idx]);
        }
    }

    // Recursive case: generate binary expression
    ast::BinaryExpr::Op op = randomBinaryOp();

    // Generate left and right operands
    auto left = generateNode(remaining_depth - 1, num_inputs, input_names);
    auto right = generateNode(remaining_depth - 1, num_inputs, input_names);

    return ast::make<ast::BinaryExpr>(op, std::move(left), std::move(right));
}

ast::Ptr<ast::Expr> RandomExprGenerator::generateRandomExpr(
    int depth,
    int num_inputs,
    const std::vector<std::string>& input_names
) {
    if (input_names.size() != static_cast<size_t>(num_inputs)) {
        throw std::invalid_argument("input_names size must match num_inputs");
    }

    return generateNode(depth, num_inputs, input_names);
}

ast::Ptr<ast::ModuleDefn> RandomExprGenerator::generateRandomModule(
    const std::string& module_name,
    int num_inputs,
    int expr_depth
) {
    // Create input port declarations
    std::vector<ast::Ptr<ast::PortDecl>> ports;

    std::vector<std::string> input_names;
    for (int i = 0; i < num_inputs; ++i) {
        std::string name = "in" + std::to_string(i);
        input_names.push_back(name);
        ports.push_back(ast::make<ast::PortDecl>(
            ast::PortDecl::Direction::Input,
            name,
            ast::makeWire(32)  // 32-bit inputs
        ));
    }

    // Create output port
    ports.push_back(ast::make<ast::PortDecl>(
        ast::PortDecl::Direction::Output,
        "out",
        ast::makeWire(32)
    ));

    // Create module
    auto module = ast::make<ast::ModuleDefn>(module_name, std::move(ports));

    // Generate random expression
    auto expr = generateRandomExpr(expr_depth, num_inputs, input_names);

    // Create continuous assignment: assign out = <random_expr>;
    auto assign = ast::make<ast::AssignStmt>(
        ast::AssignStmt::Kind::Continuous,  // "assign" statement
        ast::make<ast::VarExpr>("out"),
        std::move(expr)
    );

    module->addStmt(std::move(assign));

    return module;
}

uint32_t RandomExprGenerator::evaluateExpr(
    const ast::Expr* expr,
    const std::vector<uint32_t>& input_values
) {
    // Use the eval() method that should be on the expression
    auto result = expr->eval(input_values);

    if (!result.has_value()) {
        throw std::runtime_error("Expression evaluation failed");
    }

    return result.value();
}

} // namespace generators
