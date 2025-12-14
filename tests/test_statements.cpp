#include "external/catch.hpp"
#include "ast/ast_stmt.hpp"
#include "ast/ast_expr.hpp"
#include <string>

using namespace ast;

TEST_CASE("AssignStmt creation and emission", "[statements][assign]") {
    SECTION("Blocking assignment") {
        auto stmt = make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("a"),
            make<LiteralExpr>(5)
        );

        REQUIRE(stmt->getKind() == AssignStmt::Kind::Blocking);
        REQUIRE(stmt->emit() == "a = 5;");
    }

    SECTION("Non-blocking assignment") {
        auto stmt = make<AssignStmt>(
            AssignStmt::Kind::NonBlocking,
            make<VarExpr>("count"),
            make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("count"),
                make<LiteralExpr>(1, 8)
            )
        );

        REQUIRE(stmt->getKind() == AssignStmt::Kind::NonBlocking);
        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("<=") != std::string::npos);
    }

    SECTION("Continuous assignment") {
        auto stmt = make<AssignStmt>(
            AssignStmt::Kind::Continuous,
            make<VarExpr>("output_signal"),
            make<VarExpr>("input_signal")
        );

        REQUIRE(stmt->getKind() == AssignStmt::Kind::Continuous);
        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("assign") != std::string::npos);
    }
}

TEST_CASE("BlockStmt creation and emission", "[statements][block]") {
    SECTION("Empty block") {
        auto stmt = make<BlockStmt>(std::vector<Ptr<Stmt>>{});
        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("begin") != std::string::npos);
        REQUIRE(emitted.find("end") != std::string::npos);
    }

    SECTION("Block with statements") {
        std::vector<Ptr<Stmt>> stmts;
        stmts.push_back(make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("a"),
            make<LiteralExpr>(1)
        ));
        stmts.push_back(make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("b"),
            make<LiteralExpr>(2)
        ));

        auto block = make<BlockStmt>(std::move(stmts));
        std::string emitted = block->emit();
        REQUIRE(emitted.find("begin") != std::string::npos);
        REQUIRE(emitted.find("end") != std::string::npos);
        REQUIRE(emitted.find("a = 1") != std::string::npos);
        REQUIRE(emitted.find("b = 2") != std::string::npos);
    }

    SECTION("Named block") {
        auto stmt = make<BlockStmt>(std::vector<Ptr<Stmt>>{}, "my_block");
        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("begin : my_block") != std::string::npos);
    }
}

TEST_CASE("IfStmt creation and emission", "[statements][if]") {
    SECTION("If without else") {
        auto stmt = make<IfStmt>(
            make<VarExpr>("reset"),
            make<AssignStmt>(
                AssignStmt::Kind::NonBlocking,
                make<VarExpr>("count"),
                make<LiteralExpr>(0)
            )
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("if (reset)") != std::string::npos);
        REQUIRE(emitted.find("else") == std::string::npos);
    }

    SECTION("If with else") {
        auto stmt = make<IfStmt>(
            make<BinaryExpr>(
                BinaryExpr::Op::Gt,
                make<VarExpr>("x"),
                make<LiteralExpr>(10)
            ),
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("y"),
                make<LiteralExpr>(1)
            ),
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("y"),
                make<LiteralExpr>(0)
            )
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("if") != std::string::npos);
        REQUIRE(emitted.find("else") != std::string::npos);
    }
}

TEST_CASE("AlwaysStmt creation and emission", "[statements][always]") {
    SECTION("Always with posedge clock") {
        std::vector<AlwaysStmt::SensitivityItem> sensitivity;
        sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::Pos, "clk"});

        auto stmt = make<AlwaysStmt>(
            AlwaysStmt::Kind::Always,
            std::move(sensitivity),
            make<AssignStmt>(
                AssignStmt::Kind::NonBlocking,
                make<VarExpr>("q"),
                make<VarExpr>("d")
            )
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("always") != std::string::npos);
        REQUIRE(emitted.find("posedge clk") != std::string::npos);
    }

    SECTION("Always with negedge") {
        std::vector<AlwaysStmt::SensitivityItem> sensitivity;
        sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::Neg, "reset"});

        auto stmt = make<AlwaysStmt>(
            AlwaysStmt::Kind::Always,
            std::move(sensitivity),
            make<AssignStmt>(
                AssignStmt::Kind::NonBlocking,
                make<VarExpr>("state"),
                make<LiteralExpr>(0)
            )
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("negedge reset") != std::string::npos);
    }

    SECTION("Always with multiple sensitivities") {
        std::vector<AlwaysStmt::SensitivityItem> sensitivity;
        sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::None, "a"});
        sensitivity.push_back({AlwaysStmt::SensitivityItem::Edge::None, "b"});

        auto stmt = make<AlwaysStmt>(
            AlwaysStmt::Kind::Always,
            std::move(sensitivity),
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("c"),
                make<BinaryExpr>(
                    BinaryExpr::Op::Add,
                    make<VarExpr>("a"),
                    make<VarExpr>("b")
                )
            )
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("always @(a or b)") != std::string::npos);
    }

    SECTION("Always_comb") {
        auto stmt = make<AlwaysStmt>(
            AlwaysStmt::Kind::AlwaysComb,
            std::vector<AlwaysStmt::SensitivityItem>{},
            make<AssignStmt>(
                AssignStmt::Kind::Blocking,
                make<VarExpr>("sum"),
                make<BinaryExpr>(
                    BinaryExpr::Op::Add,
                    make<VarExpr>("a"),
                    make<VarExpr>("b")
                )
            )
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("always_comb") != std::string::npos);
    }
}

TEST_CASE("WhileStmt creation and emission", "[statements][while]") {
    auto stmt = make<WhileStmt>(
        make<BinaryExpr>(
            BinaryExpr::Op::Lt,
            make<VarExpr>("i"),
            make<LiteralExpr>(10)
        ),
        make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("i"),
            make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("i"),
                make<LiteralExpr>(1)
            )
        )
    );

    std::string emitted = stmt->emit();
    REQUIRE(emitted.find("while") != std::string::npos);
    REQUIRE(emitted.find("i < 10") != std::string::npos);
}

TEST_CASE("ForStmt creation and emission", "[statements][for]") {
    auto stmt = make<ForStmt>(
        make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("i"),
            make<LiteralExpr>(0)
        ),
        make<BinaryExpr>(
            BinaryExpr::Op::Lt,
            make<VarExpr>("i"),
            make<LiteralExpr>(10)
        ),
        make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("i"),
            make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("i"),
                make<LiteralExpr>(1)
            )
        ),
        make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("sum"),
            make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("sum"),
                make<VarExpr>("i")
            )
        )
    );

    std::string emitted = stmt->emit();
    REQUIRE(emitted.find("for") != std::string::npos);
    REQUIRE(emitted.find("i = 0") != std::string::npos);
    REQUIRE(emitted.find("i < 10") != std::string::npos);
}

TEST_CASE("InitialStmt creation and emission", "[statements][initial]") {
    auto stmt = make<InitialStmt>(
        make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("reset"),
            make<LiteralExpr>(1)
        )
    );

    std::string emitted = stmt->emit();
    REQUIRE(emitted.find("initial") != std::string::npos);
    REQUIRE(emitted.find("reset = 1") != std::string::npos);
}

TEST_CASE("CaseStmt creation and emission", "[statements][case]") {
    SECTION("Simple case statement") {
        std::vector<CaseStmt::CaseItem> items;

        CaseStmt::CaseItem item1;
        item1.conditions.push_back(make<LiteralExpr>(0, 2));
        item1.stmt = make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("out"),
            make<LiteralExpr>(10)
        );
        items.push_back(std::move(item1));

        CaseStmt::CaseItem item2;
        item2.conditions.push_back(make<LiteralExpr>(1, 2));
        item2.stmt = make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("out"),
            make<LiteralExpr>(20)
        );
        items.push_back(std::move(item2));

        auto stmt = make<CaseStmt>(
            CaseStmt::Kind::Case,
            make<VarExpr>("sel"),
            std::move(items)
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("case (sel)") != std::string::npos);
        REQUIRE(emitted.find("endcase") != std::string::npos);
    }

    SECTION("Case with default") {
        std::vector<CaseStmt::CaseItem> items;

        CaseStmt::CaseItem item;
        item.conditions.push_back(make<LiteralExpr>(0));
        item.stmt = make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("out"),
            make<LiteralExpr>(1)
        );
        items.push_back(std::move(item));

        auto default_stmt = make<AssignStmt>(
            AssignStmt::Kind::Blocking,
            make<VarExpr>("out"),
            make<LiteralExpr>(0)
        );

        auto stmt = make<CaseStmt>(
            CaseStmt::Kind::Case,
            make<VarExpr>("state"),
            std::move(items),
            std::move(default_stmt)
        );

        std::string emitted = stmt->emit();
        REQUIRE(emitted.find("default") != std::string::npos);
    }
}
