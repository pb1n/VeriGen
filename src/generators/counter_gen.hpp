#pragma once

#include "ast/ast_module.hpp"
#include "ast/ast_decl.hpp"
#include "ast/ast_stmt.hpp"
#include "ast/ast_expr.hpp"
#include "ast/ast_types.hpp"

namespace generators {

// TODO

// Creates a simple up-counter module
// Parameters:
//   - name: module name
//   - width: counter bit width
//   - has_reset: whether to include reset logic

ast::Ptr<ast::ModuleDefn> generateCounter(
    const std::string& name,
    uint32_t width,
    bool has_reset = true,
    bool has_load = false,
    bool descend = false
) {
    using namespace ast;
    // Create ports
    std::vector<Ptr<PortDecl>> ports;
    ports.push_back(make<PortDecl>(
        PortDecl::Direction::Input,
        "clk",
        makeWire(1)
    ));
    if (has_reset) ports.push_back(make<PortDecl>(
        PortDecl::Direction::Input,
        "rst",
        makeWire(1)
    ));
    if (has_load) { 
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "load_enable",
            makeWire(1)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "data_in",
            makeWire(width)
        ));
    }
    ports.push_back(make<PortDecl>(
        PortDecl::Direction::Output,
        "count",
        makeReg(width)
    ));

    // Create parameters
    std::vector<Ptr<ParamDecl>> params;
    params.push_back(make<ParamDecl>(
        ParamDecl::Kind::Parameter,
        "WIDTH",
        make<LiteralExpr>(width)
    ));
    // Create module
    auto counter_module = make<ModuleDefn>(name, std::move(ports), std::move(params));

    // Create always block body
    Ptr<Expr> increment_expr = has_load ?
        static_cast<Ptr<Expr>>(make<ConditionalExpr>(
            make<VarExpr>("load_enable"),
            make<VarExpr>("data_in"),
            make<BinaryExpr>(
                descend ? BinaryExpr::Op::Sub : BinaryExpr::Op::Add,
                make<VarExpr>("count"),
                make<LiteralExpr>(1, width)
            )
        )) :
        static_cast<Ptr<Expr>>(make<BinaryExpr>(
            descend ? BinaryExpr::Op::Sub : BinaryExpr::Op::Add,
            make<VarExpr>("count"),
            make<LiteralExpr>(1, width)
        ));

    // Body statement
    Ptr<Expr> final_expr;
    if (has_reset) {
        final_expr = make<ConditionalExpr>(
            make<VarExpr>("rst"),
            make<LiteralExpr>(0, width),
            std::move(increment_expr)
        );
    } else {
        final_expr = std::move(increment_expr);
    }

    Ptr<Stmt> body_stmt = make<AssignStmt>(
        AssignStmt::Kind::NonBlocking,
        make<VarExpr>("count"),
        std::move(final_expr)
    );

    // Create sensitivity list
    std::vector<AlwaysStmt::SensitivityItem> sensitivity;
    sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::Pos, "clk"});

    // Create always block
    auto always_block = make<AlwaysStmt>(
        AlwaysStmt::Kind::Always,
        std::move(sensitivity),
        std::move(body_stmt)
    );

    counter_module->addStmt(std::move(always_block));
    return counter_module;
}
} // namespace generators