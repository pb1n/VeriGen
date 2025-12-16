#include "generators/case_gen.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_decl.hpp"
#include "ast/ast_stmt.hpp"
#include "ast/ast_expr.hpp"
#include "ast/ast_types.hpp"

namespace generators {

    ast::Ptr<ast::ModuleDefn> generateCaseTest(int width) {
        using namespace ast;

        // 1. Create Ports
        std::vector<Ptr<PortDecl>> ports;
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input, "a", makeWire(width)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Input, "b", makeWire(width)
        ));
        ports.push_back(make<PortDecl>(
            PortDecl::Direction::Output, "y", makeWire(width)
        ));

        // 2. Create Parameters
        std::vector<Ptr<ParamDecl>> params;
        params.push_back(make<ParamDecl>(
            ParamDecl::Kind::Parameter,
            "MODE",
            make<LiteralExpr>(0)
        ));

        // 3. Create Module with ports and params
        auto module = make<ModuleDefn>(
            "case_test_module",
            std::move(ports),
            std::move(params)
        );

        // 4. Create Case Items
        std::vector<GenerateStmt::CaseItem> cases;

        // --- Case 0: Addition (y = a + b) ---
        {
            GenerateStmt::CaseItem item;
            item.values.push_back(make<LiteralExpr>(0));

            auto add_expr = make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("a"),
                make<VarExpr>("b")
            );

            item.body = make<AssignStmt>(
                AssignStmt::Kind::Continuous,
                make<VarExpr>("y"),
                std::move(add_expr)
            );
            cases.push_back(std::move(item));
        }

        // --- Case 1, 2: Subtraction (y = a - b) ---
        {
            GenerateStmt::CaseItem item;
            item.values.push_back(make<LiteralExpr>(1));
            item.values.push_back(make<LiteralExpr>(2));

            auto sub_expr = make<BinaryExpr>(
                BinaryExpr::Op::Sub,
                make<VarExpr>("a"),
                make<VarExpr>("b")
            );

            item.body = make<AssignStmt>(
                AssignStmt::Kind::Continuous,
                make<VarExpr>("y"),
                std::move(sub_expr)
            );
            cases.push_back(std::move(item));
        }

        // 5. Default Block: y = 0
        auto zero_expr = make<LiteralExpr>(0);
        auto default_stmt = make<AssignStmt>(
            AssignStmt::Kind::Continuous,
            make<VarExpr>("y"),
            std::move(zero_expr)
        );

        // 6. Create the Generate-Case Statement
        auto switch_expr = make<VarExpr>("MODE");

        auto gen_case = make<GenerateStmt>(
            std::move(switch_expr),
            std::move(cases),
            std::move(default_stmt)
        );

        // 7. Add generate statement to module
        module->addStmt(std::move(gen_case));

        return module;
    }
}
