#include "external/catch.hpp"
#include "ast/ast_visitor.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_decl.hpp"
#include "ast/ast_stmt.hpp"
#include "ast/ast_expr.hpp"
#include "ast/ast_types.hpp"

using namespace ast;

// Custom visitor that counts node types
class NodeCounterVisitor : public BaseVisitor {
public:
    int literal_count = 0;
    int var_count = 0;
    int binary_count = 0;
    int assign_count = 0;
    int if_count = 0;

    void visit(LiteralExpr& node) override {
        (void)node;
        literal_count++;
    }

    void visit(VarExpr& node) override {
        (void)node;
        var_count++;
    }

    void visit(BinaryExpr& node) override {
        (void)node;
        binary_count++;
    }

    void visit(AssignStmt& node) override {
        (void)node;
        assign_count++;
    }

    void visit(IfStmt& node) override {
        (void)node;
        if_count++;
    }
};

// Recursive visitor that collects variable names
class VarNameCollector : public RecursiveVisitor {
public:
    using RecursiveVisitor::visit;  // Bring in all overloaded visit methods

    std::vector<std::string> var_names;

    void visit(VarExpr& node) override {
        var_names.push_back(node.getName());
        // VarExpr is a leaf node, no need to recurse
    }
};

TEST_CASE("BaseVisitor basic functionality", "[visitor][base]") {
    SECTION("Visit literal expression") {
        NodeCounterVisitor visitor;
        auto lit = make<LiteralExpr>(42);

        lit->accept(visitor);

        REQUIRE(visitor.literal_count == 1);
        REQUIRE(visitor.var_count == 0);
        REQUIRE(visitor.binary_count == 0);
    }

    SECTION("Visit variable expression") {
        NodeCounterVisitor visitor;
        auto var = make<VarExpr>("x");

        var->accept(visitor);

        REQUIRE(visitor.literal_count == 0);
        REQUIRE(visitor.var_count == 1);
    }

    SECTION("Visit binary expression") {
        NodeCounterVisitor visitor;
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<VarExpr>("a"),
            make<VarExpr>("b")
        );

        expr->accept(visitor);

        REQUIRE(visitor.binary_count == 1);
        // Note: BaseVisitor doesn't recurse by default
        REQUIRE(visitor.var_count == 0);
    }
}

TEST_CASE("RecursiveVisitor traversal", "[visitor][recursive]") {
    SECTION("Traverse simple expression") {
        VarNameCollector visitor;
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<VarExpr>("x"),
            make<VarExpr>("y")
        );

        expr->accept(visitor);

        REQUIRE(visitor.var_names.size() == 2);
        REQUIRE(std::find(visitor.var_names.begin(), visitor.var_names.end(), "x") != visitor.var_names.end());
        REQUIRE(std::find(visitor.var_names.begin(), visitor.var_names.end(), "y") != visitor.var_names.end());
    }

    SECTION("Traverse nested expression") {
        VarNameCollector visitor;
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<BinaryExpr>(
                BinaryExpr::Op::Mul,
                make<VarExpr>("a"),
                make<VarExpr>("b")
            ),
            make<VarExpr>("c")
        );

        expr->accept(visitor);

        REQUIRE(visitor.var_names.size() == 3);
        REQUIRE(std::find(visitor.var_names.begin(), visitor.var_names.end(), "a") != visitor.var_names.end());
        REQUIRE(std::find(visitor.var_names.begin(), visitor.var_names.end(), "b") != visitor.var_names.end());
        REQUIRE(std::find(visitor.var_names.begin(), visitor.var_names.end(), "c") != visitor.var_names.end());
    }

    SECTION("Traverse conditional expression") {
        VarNameCollector visitor;
        auto expr = make<ConditionalExpr>(
            make<VarExpr>("cond"),
            make<VarExpr>("true_val"),
            make<VarExpr>("false_val")
        );

        expr->accept(visitor);

        REQUIRE(visitor.var_names.size() == 3);
    }
}

TEST_CASE("Visitor on statements", "[visitor][statements]") {
    SECTION("Visit assignment") {
        NodeCounterVisitor visitor;
        auto stmt = make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("a"),
            make<LiteralExpr>(5)
        );

        stmt->accept(visitor);

        REQUIRE(visitor.assign_count == 1);
    }

    SECTION("Visit if statement") {
        NodeCounterVisitor visitor;
        auto stmt = make<IfStmt>(
            make<VarExpr>("cond"),
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("a"),
                make<LiteralExpr>(1)
            )
        );

        stmt->accept(visitor);

        REQUIRE(visitor.if_count == 1);
    }

    SECTION("Recursive visit on block statement") {
        VarNameCollector visitor;

        std::vector<Ptr<Stmt>> stmts;
        stmts.push_back(make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("x"),
            make<LiteralExpr>(1)
        ));
        stmts.push_back(make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("y"),
            make<VarExpr>("x")
        ));

        auto block = make<BlockStmt>(std::move(stmts));
        block->accept(visitor);

        REQUIRE(visitor.var_names.size() == 3);  // x, y, x
    }
}

TEST_CASE("Visitor on module", "[visitor][module]") {
    SECTION("Visit simple module") {
        VarNameCollector visitor;

        std::vector<Ptr<PortDecl>> ports;
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "clk",
            makeWire(1)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Output,
            "out",
            makeReg(8)
        ));

        auto module = make<ModuleDefn>("test_mod", std::move(ports));

        module->addStmt(make<AssignStmt>(
            AssignStmt::Kind::Continuous,
            make<VarExpr>("out"),
            make<VarExpr>("in")
        ));

        module->accept(visitor);

        // Should find "out" and "in" variables
        REQUIRE(visitor.var_names.size() >= 2);
    }
}

// Custom visitor for expression optimization (constant folding example)
class ConstantFolder : public RecursiveVisitor {
public:
    using RecursiveVisitor::visit;  // Bring in all overloaded visit methods

    bool modified = false;

    void visit(BinaryExpr& node) override {
        // This is a simple example - in practice, you'd need to
        // modify the AST, which requires more infrastructure
        const auto* left_lit = dynamic_cast<const LiteralExpr*>(node.getLeft());
        const auto* right_lit = dynamic_cast<const LiteralExpr*>(node.getRight());

        if (left_lit && right_lit) {
            // Both operands are literals - could be folded
            modified = true;
        }

        RecursiveVisitor::visit(node);
    }
};

TEST_CASE("Custom visitor for optimization", "[visitor][optimization]") {
    SECTION("Detect constant expressions") {
        ConstantFolder folder;

        auto expr1 = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<LiteralExpr>(5),
            make<LiteralExpr>(3)
        );

        expr1->accept(folder);
        REQUIRE(folder.modified == true);
    }

    SECTION("Detect non-constant expressions") {
        ConstantFolder folder;

        auto expr2 = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<VarExpr>("x"),
            make<LiteralExpr>(3)
        );

        expr2->accept(folder);
        REQUIRE(folder.modified == false);
    }
}

// Visitor that validates expression depths
class DepthChecker : public RecursiveVisitor {
public:
    using RecursiveVisitor::visit;  // Bring in all overloaded visit methods

    int max_depth = 0;
    int current_depth = 0;

    void visit(BinaryExpr& node) override {
        current_depth++;
        if (current_depth > max_depth) {
            max_depth = current_depth;
        }

        RecursiveVisitor::visit(node);

        current_depth--;
    }

    void visit(UnaryExpr& node) override {
        current_depth++;
        if (current_depth > max_depth) {
            max_depth = current_depth;
        }

        RecursiveVisitor::visit(node);

        current_depth--;
    }
};

TEST_CASE("Depth checking visitor", "[visitor][depth]") {
    SECTION("Flat expression") {
        DepthChecker checker;
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<LiteralExpr>(1),
            make<LiteralExpr>(2)
        );

        expr->accept(checker);
        REQUIRE(checker.max_depth == 1);
    }

    SECTION("Nested expression") {
        DepthChecker checker;
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<BinaryExpr>(
                BinaryExpr::Op::Mul,
                make<BinaryExpr>(
                    BinaryExpr::Op::Sub,
                    make<LiteralExpr>(5),
                    make<LiteralExpr>(2)
                ),
                make<LiteralExpr>(3)
            ),
            make<LiteralExpr>(1)
        );

        expr->accept(checker);
        REQUIRE(checker.max_depth == 3);
    }
}
