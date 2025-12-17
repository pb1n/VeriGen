#include "ast_base.hpp"
#include "ast_types.hpp"
#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_decl.hpp"
#include "ast_module.hpp"`
#include "ast_visitor.hpp"

#include <iostream>

using namespace ast;

int main() {
    // Example: Create a simple counter module
    // module counter #(
    //   parameter WIDTH = 8
    // ) (
    //   input wire clk,
    //   input wire rst,
    //   output reg [WIDTH-1:0] count
    // );
    //   always @(posedge clk) begin
    //     if (rst)
    //       count <= 0;
    //     else
    //       count <= count + 1;
    //   end
    // endmodule

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

    // Create parameters
    std::vector<Ptr<ParamDecl>> params;
    params.push_back(make<ParamDecl>(
        ParamDecl::Kind::Parameter,
        "WIDTH",
        make<LiteralExpr>(8)
    ));

    // Create module
    auto counter_module = make<ModuleDefn>("counter", std::move(ports), std::move(params));

    // Create always block body
    // if (rst) count <= 0; else count <= count + 1;
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

    // Create sensitivity list
    std::vector<AlwaysStmt::SensitivityItem> sensitivity;
    sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::Pos, "clk"});

    // Create always block
    auto always_block = make<AlwaysStmt>(
        AlwaysStmt::Kind::Always,
        std::move(sensitivity),
        std::move(if_stmt)
    );

    counter_module->addStmt(std::move(always_block));

    // Emit the module
    std::cout << "Generated Verilog:\n";
    std::cout << "==================\n\n";
    std::cout << counter_module->emit() << "\n\n";

    // Example: Test expression evaluation
    std::cout << "Expression Evaluation Examples:\n";
    std::cout << "================================\n\n";

    // Create expression: (5 + 3) * 2
    auto expr1 = make<BinaryExpr>(
        BinaryExpr::Op::Mul,
        make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<LiteralExpr>(5),
            make<LiteralExpr>(3)
        ),
        make<LiteralExpr>(2)
    );

    std::cout << "Expression: " << expr1->emit() << "\n";
    auto result1 = expr1->eval({});
    if (result1) {
        std::cout << "Result: " << *result1 << "\n\n";
    }

    // Create expression: a & b | c (with variables)
    auto expr2 = make<BinaryExpr>(
        BinaryExpr::Op::Or,
        make<BinaryExpr>(
            BinaryExpr::Op::And,
            make<VarExpr>("a", 0),
            make<VarExpr>("b", 1)
        ),
        make<VarExpr>("c", 2)
    );

    std::cout << "Expression: " << expr2->emit() << "\n";
    std::vector<uint32_t> values = {0xF0, 0x0F, 0x55};
    auto result2 = expr2->eval(values);
    if (result2) {
        std::cout << "With a=0xF0, b=0x0F, c=0x55\n";
        std::cout << "Result: 0x" << std::hex << *result2 << std::dec << "\n\n";
    }

    // Create conditional expression: (x > 10) ? x : 10
    auto expr3 = make<ConditionalExpr>(
        make<BinaryExpr>(
            BinaryExpr::Op::Gt,
            make<VarExpr>("x", 0),
            make<LiteralExpr>(10)
        ),
        make<VarExpr>("x", 0),
        make<LiteralExpr>(10)
    );

    std::cout << "Expression: " << expr3->emit() << "\n";
    std::cout << "With x=5:  Result = " << *expr3->eval({5}) << "\n";
    std::cout << "With x=15: Result = " << *expr3->eval({15}) << "\n\n";

    // Example: Create a more complex module with instantiation
    std::cout << "Complex Module with Instantiation:\n";
    std::cout << "===================================\n\n";

    // Create top module
    std::vector<Ptr<PortDecl>> top_ports;
    top_ports.push_back(make<PortDecl>(
        PortDecl::Direction::Input,
        "sys_clk",
        makeWire(1)
    ));
    top_ports.push_back(make<PortDecl>(
        PortDecl::Direction::Input,
        "sys_rst",
        makeWire(1)
    ));
    top_ports.push_back(make<PortDecl>(
        PortDecl::Direction::Output,
        "counter_out",
        makeWire(16)
    ));

    auto top_module = make<ModuleDefn>("top", std::move(top_ports));

    // Instantiate counter module
    std::vector<ModuleInstance::Connection> connections;
    connections.push_back({"clk", make<VarExpr>("sys_clk")});
    connections.push_back({"rst", make<VarExpr>("sys_rst")});
    connections.push_back({"count", make<VarExpr>("counter_out")});

    std::vector<Ptr<Expr>> inst_params;
    inst_params.push_back(make<LiteralExpr>(16));

    std::vector<ModuleInstance> instances;
    instances.push_back(ModuleInstance("u_counter", std::move(connections), std::move(inst_params)));

    auto inst_stmt = make<ModuleInstStmt>("counter", std::move(instances));
    top_module->addStmt(std::move(inst_stmt));

    std::cout << top_module->emit() << "\n\n";

    std::cout << "AST Example completed successfully!\n";

    return 0;
}
