#include "external/catch.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_decl.hpp"
#include "ast/ast_stmt.hpp"
#include "ast/ast_expr.hpp"
#include "ast/ast_types.hpp"

using namespace ast;

TEST_CASE("Complete module emission", "[emission][module]") {
    SECTION("Empty module") {
        std::vector<Ptr<PortDecl>> ports;
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "clk",
            makeWire(1)
        ));

        auto module = make<ModuleDefn>("empty_module", std::move(ports));

        std::string emitted = module->emit();
        REQUIRE(emitted.find("module empty_module") != std::string::npos);
        REQUIRE(emitted.find("input wire clk") != std::string::npos);
        REQUIRE(emitted.find("endmodule") != std::string::npos);
    }

    SECTION("Module with parameters") {
        std::vector<Ptr<PortDecl>> ports;
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "data",
            makeWire(8)
        ));

        std::vector<Ptr<ParamDecl>> params;
        params.push_back(make<ParamDecl>(
            ParamDecl::Kind::Parameter,
            "WIDTH",
            make<LiteralExpr>(8)
        ));

        auto module = make<ModuleDefn>("param_module", std::move(ports), std::move(params));

        std::string emitted = module->emit();
        REQUIRE(emitted.find("module param_module #(") != std::string::npos);
        REQUIRE(emitted.find("parameter WIDTH = 8") != std::string::npos);
    }

    SECTION("Counter module") {
        // Create ports
        std::vector<Ptr<PortDecl>> ports;
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "clk",
            makeWire(1)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "rst",
            makeWire(1)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Output,
            "count",
            makeReg(8)
        ));

        auto module = make<ModuleDefn>("counter", std::move(ports));

        // Add always block
        auto reset_assign = make<AssignStmt>(
            AssignStmt::Kind::NonBlocking,
            make<VarExpr>("count"),
            make<LiteralExpr>(0, 8)
        );

        auto increment_assign = make<AssignStmt>(
            AssignStmt::Kind::NonBlocking,
            make<VarExpr>("count"),
            make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("count"),
                make<LiteralExpr>(1, 8)
            )
        );

        auto if_stmt = make<IfStmt>(
            make<VarExpr>("rst"),
            std::move(reset_assign),
            std::move(increment_assign)
        );

        std::vector<AlwaysStmt::SensitivityItem> sensitivity;
        sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::Pos, "clk"});

        auto always_block = make<AlwaysStmt>(
            AlwaysStmt::Kind::Always,
            std::move(sensitivity),
            std::move(if_stmt)
        );

        module->addStmt(std::move(always_block));

        std::string emitted = module->emit();
        REQUIRE(emitted.find("module counter") != std::string::npos);
        REQUIRE(emitted.find("always @(posedge clk)") != std::string::npos);
        REQUIRE(emitted.find("if (rst)") != std::string::npos);
        REQUIRE(emitted.find("count <= 8'd0") != std::string::npos);
        REQUIRE(emitted.find("endmodule") != std::string::npos);
    }

    SECTION("Module with internal signals") {
        std::vector<Ptr<PortDecl>> ports;
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "a",
            makeWire(8)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Output,
            "b",
            makeWire(8)
        ));

        auto module = make<ModuleDefn>("simple_module", std::move(ports));

        // Add internal signal
        module->addDecl(make<VarDecl>(
            "internal",
            makeWire(8)
        ));

        // Add continuous assignment
        module->addStmt(make<AssignStmt>(
            AssignStmt::Kind::Continuous,
            make<VarExpr>("b"),
            make<VarExpr>("a")
        ));

        std::string emitted = module->emit();
        REQUIRE(emitted.find("wire [7:0] internal") != std::string::npos);
        REQUIRE(emitted.find("assign b = a") != std::string::npos);
    }
}

TEST_CASE("Module instantiation emission", "[emission][instantiation]") {
    SECTION("Simple instantiation") {
        std::vector<ModuleInstance::Connection> connections;
        connections.push_back({"clk", make<VarExpr>("sys_clk")});
        connections.push_back({"data", make<VarExpr>("data_in")});

        std::vector<ModuleInstance> instances;
        instances.push_back(ModuleInstance("u_inst", std::move(connections)));

        auto inst_stmt = make<ModuleInstStmt>("my_module", std::move(instances));

        std::string emitted = inst_stmt->emit();
        REQUIRE(emitted.find("my_module u_inst") != std::string::npos);
        REQUIRE(emitted.find(".clk(sys_clk)") != std::string::npos);
        REQUIRE(emitted.find(".data(data_in)") != std::string::npos);
    }

    SECTION("Instantiation with parameters") {
        std::vector<ModuleInstance::Connection> connections;
        connections.push_back({"in", make<VarExpr>("x")});

        std::vector<Ptr<Expr>> params;
        params.push_back(make<LiteralExpr>(16));

        std::vector<ModuleInstance> instances;
        instances.push_back(ModuleInstance("u_wide", std::move(connections), std::move(params)));

        auto inst_stmt = make<ModuleInstStmt>("parameterized_mod", std::move(instances));

        std::string emitted = inst_stmt->emit();
        REQUIRE(emitted.find("#(16)") != std::string::npos);
    }
}

TEST_CASE("Design with multiple modules", "[emission][design]") {
    Design design;

    // Create first module
    std::vector<Ptr<PortDecl>> ports1;
    ports1.push_back(make<PortDecl>(
        PortDecl::Direction::Input,
        "in",
        makeWire(1)
    ));
    ports1.push_back(make<PortDecl>(
        PortDecl::Direction::Output,
        "out",
        makeWire(1)
    ));

    auto module1 = make<ModuleDefn>("module_a", std::move(ports1));
    module1->addStmt(make<AssignStmt>(
        AssignStmt::Kind::Continuous,
        make<VarExpr>("out"),
        make<VarExpr>("in")
    ));

    design.addModule(std::move(module1));

    // Create second module
    std::vector<Ptr<PortDecl>> ports2;
    ports2.push_back(make<PortDecl>(
        PortDecl::Direction::Input,
        "x",
        makeWire(1)
    ));

    auto module2 = make<ModuleDefn>("module_b", std::move(ports2));

    design.addModule(std::move(module2));

    std::string emitted = design.emit();
    REQUIRE(emitted.find("module module_a") != std::string::npos);
    REQUIRE(emitted.find("module module_b") != std::string::npos);
    REQUIRE(emitted.find("endmodule") != std::string::npos);
}

TEST_CASE("Type emission", "[emission][types]") {
    SECTION("Wire type") {
        auto type = makeWire(8);
        REQUIRE(type->emit() == "wire [7:0]");
        REQUIRE(type->getWidth() == 8);
        REQUIRE_FALSE(type->isSigned());
    }

    SECTION("Signed reg type") {
        auto type = makeReg(16, true);
        std::string emitted = type->emit();
        REQUIRE(emitted.find("signed") != std::string::npos);
        REQUIRE(emitted.find("reg") != std::string::npos);
        REQUIRE(type->getWidth() == 16);
        REQUIRE(type->isSigned());
    }

    SECTION("Logic type") {
        auto type = makeLogic(32);
        std::string emitted = type->emit();
        REQUIRE(emitted.find("logic") != std::string::npos);
        REQUIRE(type->getWidth() == 32);
    }

    SECTION("Single-bit wire") {
        auto type = makeWire(1);
        REQUIRE(type->emit() == "wire");
    }
}

TEST_CASE("Complex expressions emission", "[emission][expressions]") {
    SECTION("Nested binary operations") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<BinaryExpr>(
                BinaryExpr::Op::Mul,
                make<VarExpr>("a"),
                make<VarExpr>("b")
            ),
            make<BinaryExpr>(
                BinaryExpr::Op::Div,
                make<VarExpr>("c"),
                make<VarExpr>("d")
            )
        );

        std::string emitted = expr->emit();
        REQUIRE(emitted.find("(a * b)") != std::string::npos);
        REQUIRE(emitted.find("(c / d)") != std::string::npos);
    }

    SECTION("Conditional expression") {
        auto expr = make<ConditionalExpr>(
            make<BinaryExpr>(
                BinaryExpr::Op::Gt,
                make<VarExpr>("x"),
                make<LiteralExpr>(0)
            ),
            make<VarExpr>("x"),
            make<UnaryExpr>(
                UnaryExpr::Op::Neg,
                make<VarExpr>("x")
            )
        );

        std::string emitted = expr->emit();
        REQUIRE(emitted.find("?") != std::string::npos);
        REQUIRE(emitted.find(":") != std::string::npos);
    }

    SECTION("Bit and range select") {
        auto bit_sel = make<BitSelectExpr>(
            make<VarExpr>("data"),
            make<LiteralExpr>(5)
        );
        REQUIRE(bit_sel->emit() == "data[5]");

        auto range_sel = make<RangeSelectExpr>(
            make<VarExpr>("data"),
            make<LiteralExpr>(7),
            make<LiteralExpr>(0)
        );
        REQUIRE(range_sel->emit() == "data[7:0]");
    }

    SECTION("Concatenation") {
        std::vector<Ptr<Expr>> exprs;
        exprs.push_back(make<VarExpr>("a"));
        exprs.push_back(make<VarExpr>("b"));
        exprs.push_back(make<VarExpr>("c"));

        auto concat = make<ConcatExpr>(std::move(exprs));
        REQUIRE(concat->emit() == "{a, b, c}");
    }
}
