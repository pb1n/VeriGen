#include "external/catch.hpp"
#include "ast/ast_decl.hpp"
#include "ast/ast_types.hpp"
#include "ast/ast_expr.hpp"
#include "ast/ast_stmt.hpp"

using namespace ast;

TEST_CASE("VarDecl creation and emission", "[declarations][variable]") {
    SECTION("Simple wire declaration") {
        auto decl = make<VarDecl>(
            "data",
            makeWire(8)
        );

        REQUIRE(decl->getName() == "data");
        std::string emitted = decl->emit();
        REQUIRE(emitted.find("wire") != std::string::npos);
        REQUIRE(emitted.find("data") != std::string::npos);
        REQUIRE(emitted.find(";") != std::string::npos);
    }

    SECTION("Reg declaration with initialization") {
        auto decl = make<VarDecl>(
            "counter",
            makeReg(16),
            make<LiteralExpr>(0, 16)
        );

        REQUIRE(decl->getName() == "counter");
        std::string emitted = decl->emit();
        REQUIRE(emitted.find("reg") != std::string::npos);
        REQUIRE(emitted.find("counter") != std::string::npos);
        REQUIRE(emitted.find("=") != std::string::npos);
    }

    SECTION("Logic declaration (SystemVerilog)") {
        auto decl = make<VarDecl>(
            "state",
            makeLogic(2)
        );

        std::string emitted = decl->emit();
        REQUIRE(emitted.find("logic") != std::string::npos);
    }

    SECTION("Signed declaration") {
        auto decl = make<VarDecl>(
            "signed_val",
            makeReg(16, true)
        );

        std::string emitted = decl->emit();
        REQUIRE(emitted.find("signed") != std::string::npos);
    }
}

TEST_CASE("PortDecl creation and emission", "[declarations][port]") {
    SECTION("Input port") {
        auto port = make<PortDecl>(
            PortDecl::Direction::Input,
            "clk",
            makeWire(1)
        );

        REQUIRE(port->getName() == "clk");
        REQUIRE(port->getDirection() == PortDecl::Direction::Input);
        std::string emitted = port->emit();
        REQUIRE(emitted.find("input") != std::string::npos);
        REQUIRE(emitted.find("clk") != std::string::npos);
    }

    SECTION("Output port") {
        auto port = make<PortDecl>(
            PortDecl::Direction::Output,
            "data_out",
            makeWire(32)
        );

        REQUIRE(port->getDirection() == PortDecl::Direction::Output);
        std::string emitted = port->emit();
        REQUIRE(emitted.find("output") != std::string::npos);
        REQUIRE(emitted.find("[31:0]") != std::string::npos);
    }

    SECTION("Inout port") {
        auto port = make<PortDecl>(
            PortDecl::Direction::Inout,
            "bus",
            makeWire(8)
        );

        REQUIRE(port->getDirection() == PortDecl::Direction::Inout);
        std::string emitted = port->emit();
        REQUIRE(emitted.find("inout") != std::string::npos);
    }
}

TEST_CASE("ParamDecl creation and emission", "[declarations][parameter]") {
    SECTION("Simple parameter") {
        auto param = make<ParamDecl>(
            ParamDecl::Kind::Parameter,
            "WIDTH",
            make<LiteralExpr>(8)
        );

        REQUIRE(param->getName() == "WIDTH");
        REQUIRE(param->getKind() == ParamDecl::Kind::Parameter);
        std::string emitted = param->emit();
        REQUIRE(emitted.find("parameter") != std::string::npos);
        REQUIRE(emitted.find("WIDTH") != std::string::npos);
        REQUIRE(emitted.find("8") != std::string::npos);
    }

    SECTION("Localparam") {
        auto param = make<ParamDecl>(
            ParamDecl::Kind::LocalParam,
            "INTERNAL_SIZE",
            make<LiteralExpr>(16)
        );

        REQUIRE(param->getKind() == ParamDecl::Kind::LocalParam);
        std::string emitted = param->emit();
        REQUIRE(emitted.find("localparam") != std::string::npos);
    }

    SECTION("Parameter with type") {
        auto param = make<ParamDecl>(
            ParamDecl::Kind::Parameter,
            "DATA_WIDTH",
            make<LiteralExpr>(32),
            makeLogic(8)
        );

        std::string emitted = param->emit();
        REQUIRE(emitted.find("parameter") != std::string::npos);
        REQUIRE(emitted.find("logic") != std::string::npos);
    }
}

TEST_CASE("FunctionDecl creation and emission", "[declarations][function]") {
    SECTION("Simple function") {
        std::vector<Ptr<PortDecl>> params;
        params.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "x",
            makeWire(8)
        ));

        auto func = make<FunctionDecl>(
            "double_value",
            makeLogic(8),
            std::move(params),
            std::vector<Ptr<VarDecl>>{},
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("double_value"),
                make<BinaryExpr>(
                    BinaryExpr::Op::Mul,
                    make<VarExpr>("x"),
                    make<LiteralExpr>(2, 8)
                )
            )
        );

        REQUIRE(func->getName() == "double_value");
        std::string emitted = func->emit();
        REQUIRE(emitted.find("function") != std::string::npos);
        REQUIRE(emitted.find("double_value") != std::string::npos);
        REQUIRE(emitted.find("endfunction") != std::string::npos);
    }

    SECTION("Function with local variables") {
        std::vector<Ptr<PortDecl>> params;
        params.push_back(make<PortDecl>(
            PortDecl::Direction::Input,
            "a",
            makeWire(8)
        ));

        std::vector<Ptr<VarDecl>> locals;
        locals.push_back(make<VarDecl>(
            "temp",
            makeReg(8)
        ));

        auto func = make<FunctionDecl>(
            "my_func",
            makeLogic(8),
            std::move(params),
            std::move(locals),
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("temp"),
                make<VarExpr>("a")
            )
        );

        std::string emitted = func->emit();
        REQUIRE(emitted.find("temp") != std::string::npos);
    }
}

TEST_CASE("TaskDecl creation and emission", "[declarations][task]") {
    SECTION("Simple task") {
        std::vector<Ptr<PortDecl>> params;
        params.push_back(make<PortDecl>(
            PortDecl::Direction::Output,
            "result",
            makeReg(8)
        ));

        auto task = make<TaskDecl>(
            "compute",
            std::move(params),
            std::vector<Ptr<VarDecl>>{},
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("result"),
                make<LiteralExpr>(42, 8)
            )
        );

        REQUIRE(task->getName() == "compute");
        std::string emitted = task->emit();
        REQUIRE(emitted.find("task") != std::string::npos);
        REQUIRE(emitted.find("compute") != std::string::npos);
        REQUIRE(emitted.find("endtask") != std::string::npos);
    }
}

TEST_CASE("GenvarDecl creation and emission", "[declarations][genvar]") {
    auto genvar = make<GenvarDecl>("i");

    REQUIRE(genvar->getName() == "i");
    std::string emitted = genvar->emit();
    REQUIRE(emitted.find("genvar") != std::string::npos);
    REQUIRE(emitted.find("i") != std::string::npos);
}
