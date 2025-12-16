/**
 * Comprehensive test for all generate constructs: if, for, and case
 */

#include "generators/case_gen.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_decl.hpp"
#include "ast/ast_stmt.hpp"
#include "ast/ast_expr.hpp"
#include "ast/ast_types.hpp"
#include "ast/ast_visitor.hpp"
#include <iostream>
#include <fstream>
#include <memory>

using namespace ast;

/**
 * Test 1: Generate-If
 * Creates a module with a generate-if block
 * Logic: if (WIDTH == 32) result = a + b; else result = a - b;
 */
Ptr<ModuleDefn> generateIfTest() {
    // Create ports
    std::vector<Ptr<PortDecl>> ports;
    ports.push_back(make<PortDecl>(PortDecl::Direction::Input, "a", makeWire(32)));
    ports.push_back(make<PortDecl>(PortDecl::Direction::Input, "b", makeWire(32)));
    ports.push_back(make<PortDecl>(PortDecl::Direction::Output, "result", makeWire(32)));

    // Create parameters
    std::vector<Ptr<ParamDecl>> params;
    params.push_back(make<ParamDecl>(ParamDecl::Kind::Parameter, "WIDTH", make<LiteralExpr>(32)));

    // Create module
    auto module = make<ModuleDefn>("if_test_module", std::move(ports), std::move(params));

    // Create if condition: WIDTH == 32
    auto if_cond = make<BinaryExpr>(
        BinaryExpr::Op::Eq,
        make<VarExpr>("WIDTH"),
        make<LiteralExpr>(32)
    );

    // Then block: result = a + b
    auto then_stmt = make<AssignStmt>(
        AssignStmt::Kind::Continuous,
        make<VarExpr>("result"),
        make<BinaryExpr>(BinaryExpr::Op::Add, make<VarExpr>("a"), make<VarExpr>("b"))
    );

    // Else block: result = a - b
    auto else_stmt = make<AssignStmt>(
        AssignStmt::Kind::Continuous,
        make<VarExpr>("result"),
        make<BinaryExpr>(BinaryExpr::Op::Sub, make<VarExpr>("a"), make<VarExpr>("b"))
    );

    // Create generate-if
    auto gen_if = make<GenerateStmt>(
        std::move(if_cond),
        std::move(then_stmt),
        std::move(else_stmt)
    );

    module->addStmt(std::move(gen_if));
    return module;
}

/**
 * Test 2: Generate-For
 * Creates a module with a generate-for block
 * Logic: for (genvar i = 0; i < 4; i++) assign data = data + i;
 */
Ptr<ModuleDefn> generateForTest() {
    // Create ports
    std::vector<Ptr<PortDecl>> ports;
    ports.push_back(make<PortDecl>(PortDecl::Direction::Output, "data", makeWire(32)));

    // Create module
    auto module = make<ModuleDefn>("for_test_module", std::move(ports));

    // Create genvar
    auto genvar = make<GenvarDecl>("i");

    // Init: i = 0
    auto init_expr = make<LiteralExpr>(0);

    // Condition: i < 4
    auto cond_expr = make<BinaryExpr>(
        BinaryExpr::Op::Lt,
        make<VarExpr>("i"),
        make<LiteralExpr>(4)
    );

    // Update: i + 1
    auto update_expr = make<BinaryExpr>(
        BinaryExpr::Op::Add,
        make<VarExpr>("i"),
        make<LiteralExpr>(1)
    );

    // Body: assign data = data + i
    auto body_stmt = make<AssignStmt>(
        AssignStmt::Kind::Continuous,
        make<VarExpr>("data"),
        make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<VarExpr>("data"),
            make<VarExpr>("i")
        )
    );

    // Create generate-for
    auto gen_for = make<GenerateStmt>(
        std::move(genvar),
        std::move(init_expr),
        std::move(cond_expr),
        std::move(update_expr),
        std::move(body_stmt)
    );

    module->addStmt(std::move(gen_for));
    return module;
}

/**
 * Test 3: Generate-Case (using the case_gen.cpp implementation)
 */
Ptr<ModuleDefn> generateCaseTestWrapper() {
    return generators::generateCaseTest(32);
}

/**
 * Test 4: Nested Generate Constructs
 * Combines if and for generates
 */
Ptr<ModuleDefn> generateNestedTest() {
    // Create ports
    std::vector<Ptr<PortDecl>> ports;
    ports.push_back(make<PortDecl>(PortDecl::Direction::Input, "clk", makeWire(1)));
    ports.push_back(make<PortDecl>(PortDecl::Direction::Output, "out", makeWire(32)));

    // Create parameters
    std::vector<Ptr<ParamDecl>> params;
    params.push_back(make<ParamDecl>(ParamDecl::Kind::Parameter, "ENABLE", make<LiteralExpr>(1)));

    // Create module
    auto module = make<ModuleDefn>("nested_test_module", std::move(ports), std::move(params));

    // Create inner for-generate
    auto genvar = make<GenvarDecl>("j");
    auto init_expr = make<LiteralExpr>(0);
    auto cond_expr = make<BinaryExpr>(BinaryExpr::Op::Lt, make<VarExpr>("j"), make<LiteralExpr>(2));
    auto update_expr = make<BinaryExpr>(BinaryExpr::Op::Add, make<VarExpr>("j"), make<LiteralExpr>(1));
    auto for_body = make<AssignStmt>(
        AssignStmt::Kind::Continuous,
        make<VarExpr>("out"),
        make<VarExpr>("j")
    );

    auto inner_for = make<GenerateStmt>(
        std::move(genvar),
        std::move(init_expr),
        std::move(cond_expr),
        std::move(update_expr),
        std::move(for_body)
    );

    // Wrap in if-generate
    auto if_cond = make<BinaryExpr>(
        BinaryExpr::Op::Eq,
        make<VarExpr>("ENABLE"),
        make<LiteralExpr>(1)
    );

    auto outer_if = make<GenerateStmt>(
        std::move(if_cond),
        std::move(inner_for),
        nullptr  // No else block
    );

    module->addStmt(std::move(outer_if));
    return module;
}

/**
 * Write module to file and display
 */
void testAndWriteModule(const std::string& test_name, Ptr<ModuleDefn> module) {
    std::cout << "\n========================================\n";
    std::cout << "Test: " << test_name << "\n";
    std::cout << "========================================\n";

    // Use visitor pattern for code generation
    ast::EmitVisitor visitor;
    module->accept(visitor);
    std::string verilog = visitor.getResult();

    std::cout << verilog << std::endl;

    // Write to file
    std::string filename = "generatedFiles/verilog/" + module->getName() + ".v";
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << verilog;
        outfile.close();
        std::cout << "\n✓ Written to: " << filename << "\n";
    } else {
        std::cerr << "✗ Failed to write: " << filename << "\n";
    }
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "VeriGen Generate Constructs Test Suite\n";
    std::cout << "===========================================\n";

    try {
        // Test 1: Generate-If
        testAndWriteModule("Generate-If", generateIfTest());

        // Test 2: Generate-For
        testAndWriteModule("Generate-For", generateForTest());

        // Test 3: Generate-Case
        testAndWriteModule("Generate-Case", generateCaseTestWrapper());

        // Test 4: Nested Generates
        testAndWriteModule("Nested Generate (If + For)", generateNestedTest());

        std::cout << "\n===========================================\n";
        std::cout << "All tests completed successfully! ✓\n";
        std::cout << "===========================================\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
