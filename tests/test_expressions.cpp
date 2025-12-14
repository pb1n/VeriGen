#include "external/catch.hpp"
#include "ast/ast_expr.hpp"
#include <vector>

using namespace ast;

TEST_CASE("LiteralExpr creation and evaluation", "[expressions][literal]") {
    SECTION("Simple literal") {
        auto lit = make<LiteralExpr>(42);
        REQUIRE(lit->getValue() == 42);
        REQUIRE(lit->getWidth() == 32);

        auto result = lit->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }

    SECTION("Literal with explicit width") {
        auto lit = make<LiteralExpr>(15, 4);
        REQUIRE(lit->getValue() == 15);
        REQUIRE(lit->getWidth() == 4);
        REQUIRE(lit->emit() == "4'd15");
    }

    SECTION("Zero literal") {
        auto lit = make<LiteralExpr>(0);
        auto result = lit->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0);
    }
}

TEST_CASE("VarExpr creation and evaluation", "[expressions][variable]") {
    SECTION("Variable with valid index") {
        auto var = make<VarExpr>("x", 0);
        REQUIRE(var->getName() == "x");
        REQUIRE(var->getIndex() == 0);
        REQUIRE(var->emit() == "x");

        std::vector<uint32_t> values = {100};
        auto result = var->eval(values);
        REQUIRE(result.has_value());
        REQUIRE(*result == 100);
    }

    SECTION("Variable with out of bounds index") {
        auto var = make<VarExpr>("y", 5);
        std::vector<uint32_t> values = {1, 2, 3};
        auto result = var->eval(values);
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("BinaryExpr arithmetic operations", "[expressions][binary][arithmetic]") {
    SECTION("Addition") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<LiteralExpr>(10),
            make<LiteralExpr>(5)
        );

        REQUIRE(expr->emit() == "(10 + 5)");
        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 15);
    }

    SECTION("Subtraction") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Sub,
            make<LiteralExpr>(20),
            make<LiteralExpr>(7)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 13);
    }

    SECTION("Multiplication") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Mul,
            make<LiteralExpr>(6),
            make<LiteralExpr>(7)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }

    SECTION("Division") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Div,
            make<LiteralExpr>(20),
            make<LiteralExpr>(4)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 5);
    }

    SECTION("Division by zero") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Div,
            make<LiteralExpr>(10),
            make<LiteralExpr>(0)
        );

        auto result = expr->eval({});
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Modulo") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Mod,
            make<LiteralExpr>(17),
            make<LiteralExpr>(5)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 2);
    }

    SECTION("Modulo by zero") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Mod,
            make<LiteralExpr>(10),
            make<LiteralExpr>(0)
        );

        auto result = expr->eval({});
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("BinaryExpr bitwise operations", "[expressions][binary][bitwise]") {
    SECTION("Bitwise AND") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::And,
            make<LiteralExpr>(0xF0),
            make<LiteralExpr>(0x0F)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0x00);
    }

    SECTION("Bitwise OR") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Or,
            make<LiteralExpr>(0xF0),
            make<LiteralExpr>(0x0F)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0xFF);
    }

    SECTION("Bitwise XOR") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Xor,
            make<LiteralExpr>(0xFF),
            make<LiteralExpr>(0x55)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0xAA);
    }

    SECTION("Left shift") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Shl,
            make<LiteralExpr>(1),
            make<LiteralExpr>(4)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 16);
    }

    SECTION("Right shift") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Shr,
            make<LiteralExpr>(16),
            make<LiteralExpr>(2)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 4);
    }
}

TEST_CASE("BinaryExpr comparison operations", "[expressions][binary][comparison]") {
    SECTION("Equal") {
        auto expr1 = make<BinaryExpr>(
            BinaryExpr::Op::Eq,
            make<LiteralExpr>(5),
            make<LiteralExpr>(5)
        );
        REQUIRE(*expr1->eval({}) == 1);

        auto expr2 = make<BinaryExpr>(
            BinaryExpr::Op::Eq,
            make<LiteralExpr>(5),
            make<LiteralExpr>(3)
        );
        REQUIRE(*expr2->eval({}) == 0);
    }

    SECTION("Not equal") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Neq,
            make<LiteralExpr>(5),
            make<LiteralExpr>(3)
        );
        REQUIRE(*expr->eval({}) == 1);
    }

    SECTION("Less than") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Lt,
            make<LiteralExpr>(3),
            make<LiteralExpr>(5)
        );
        REQUIRE(*expr->eval({}) == 1);
    }

    SECTION("Greater than") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Gt,
            make<LiteralExpr>(5),
            make<LiteralExpr>(3)
        );
        REQUIRE(*expr->eval({}) == 1);
    }
}

TEST_CASE("BinaryExpr logical operations", "[expressions][binary][logical]") {
    SECTION("Logical AND") {
        auto expr1 = make<BinaryExpr>(
            BinaryExpr::Op::LAnd,
            make<LiteralExpr>(1),
            make<LiteralExpr>(1)
        );
        REQUIRE(*expr1->eval({}) == 1);

        auto expr2 = make<BinaryExpr>(
            BinaryExpr::Op::LAnd,
            make<LiteralExpr>(1),
            make<LiteralExpr>(0)
        );
        REQUIRE(*expr2->eval({}) == 0);
    }

    SECTION("Logical OR") {
        auto expr1 = make<BinaryExpr>(
            BinaryExpr::Op::LOr,
            make<LiteralExpr>(0),
            make<LiteralExpr>(1)
        );
        REQUIRE(*expr1->eval({}) == 1);

        auto expr2 = make<BinaryExpr>(
            BinaryExpr::Op::LOr,
            make<LiteralExpr>(0),
            make<LiteralExpr>(0)
        );
        REQUIRE(*expr2->eval({}) == 0);
    }
}

TEST_CASE("UnaryExpr operations", "[expressions][unary]") {
    SECTION("Negation") {
        auto expr = make<UnaryExpr>(
            UnaryExpr::Op::Neg,
            make<LiteralExpr>(5)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == static_cast<uint32_t>(-5));
    }

    SECTION("Bitwise NOT") {
        auto expr = make<UnaryExpr>(
            UnaryExpr::Op::Not,
            make<LiteralExpr>(0x0000FFFF)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0xFFFF0000);
    }

    SECTION("Logical NOT") {
        auto expr1 = make<UnaryExpr>(
            UnaryExpr::Op::LNot,
            make<LiteralExpr>(0)
        );
        REQUIRE(*expr1->eval({}) == 1);

        auto expr2 = make<UnaryExpr>(
            UnaryExpr::Op::LNot,
            make<LiteralExpr>(42)
        );
        REQUIRE(*expr2->eval({}) == 0);
    }

    SECTION("Reduction OR") {
        auto expr1 = make<UnaryExpr>(
            UnaryExpr::Op::RedOr,
            make<LiteralExpr>(0)
        );
        REQUIRE(*expr1->eval({}) == 0);

        auto expr2 = make<UnaryExpr>(
            UnaryExpr::Op::RedOr,
            make<LiteralExpr>(0x00000001)
        );
        REQUIRE(*expr2->eval({}) == 1);
    }
}

TEST_CASE("ConditionalExpr operations", "[expressions][conditional]") {
    SECTION("True condition") {
        auto expr = make<ConditionalExpr>(
            make<LiteralExpr>(1),
            make<LiteralExpr>(42),
            make<LiteralExpr>(0)
        );

        REQUIRE(expr->emit() == "(1 ? 42 : 0)");
        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }

    SECTION("False condition") {
        auto expr = make<ConditionalExpr>(
            make<LiteralExpr>(0),
            make<LiteralExpr>(42),
            make<LiteralExpr>(99)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 99);
    }

    SECTION("Nested conditional") {
        auto expr = make<ConditionalExpr>(
            make<BinaryExpr>(
                BinaryExpr::Op::Gt,
                make<VarExpr>("x", 0),
                make<LiteralExpr>(10)
            ),
            make<VarExpr>("x", 0),
            make<LiteralExpr>(10)
        );

        REQUIRE(*expr->eval({5}) == 10);
        REQUIRE(*expr->eval({15}) == 15);
    }
}

TEST_CASE("BitSelectExpr operations", "[expressions][bitselect]") {
    SECTION("Select bit 0") {
        auto expr = make<BitSelectExpr>(
            make<LiteralExpr>(0b1010),
            make<LiteralExpr>(0)
        );

        REQUIRE(*expr->eval({}) == 0);
    }

    SECTION("Select bit 1") {
        auto expr = make<BitSelectExpr>(
            make<LiteralExpr>(0b1010),
            make<LiteralExpr>(1)
        );

        REQUIRE(*expr->eval({}) == 1);
    }

    SECTION("Select bit out of range") {
        auto expr = make<BitSelectExpr>(
            make<LiteralExpr>(0xFF),
            make<LiteralExpr>(100)
        );

        auto result = expr->eval({});
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("RangeSelectExpr operations", "[expressions][rangeselect]") {
    SECTION("Select range [3:0]") {
        auto expr = make<RangeSelectExpr>(
            make<LiteralExpr>(0xFF),
            make<LiteralExpr>(3),
            make<LiteralExpr>(0)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0x0F);
    }

    SECTION("Select range [7:4]") {
        auto expr = make<RangeSelectExpr>(
            make<LiteralExpr>(0xFF),
            make<LiteralExpr>(7),
            make<LiteralExpr>(4)
        );

        auto result = expr->eval({});
        REQUIRE(result.has_value());
        REQUIRE(*result == 0x0F);
    }

    SECTION("Invalid range (msb < lsb)") {
        auto expr = make<RangeSelectExpr>(
            make<LiteralExpr>(0xFF),
            make<LiteralExpr>(0),
            make<LiteralExpr>(7)
        );

        auto result = expr->eval({});
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("Complex nested expressions", "[expressions][complex]") {
    SECTION("Arithmetic expression: (a + b) * (c - d)") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Mul,
            make<BinaryExpr>(
                BinaryExpr::Op::Add,
                make<VarExpr>("a", 0),
                make<VarExpr>("b", 1)
            ),
            make<BinaryExpr>(
                BinaryExpr::Op::Sub,
                make<VarExpr>("c", 2),
                make<VarExpr>("d", 3)
            )
        );

        std::vector<uint32_t> values = {3, 4, 10, 2};  // a=3, b=4, c=10, d=2
        auto result = expr->eval(values);
        REQUIRE(result.has_value());
        REQUIRE(*result == (3 + 4) * (10 - 2));  // 7 * 8 = 56
    }

    SECTION("Mixed operations: ((a & b) | c) == d") {
        auto expr = make<BinaryExpr>(
            BinaryExpr::Op::Eq,
            make<BinaryExpr>(
                BinaryExpr::Op::Or,
                make<BinaryExpr>(
                    BinaryExpr::Op::And,
                    make<VarExpr>("a", 0),
                    make<VarExpr>("b", 1)
                ),
                make<VarExpr>("c", 2)
            ),
            make<VarExpr>("d", 3)
        );

        std::vector<uint32_t> values = {0xF0, 0x0F, 0x55, 0x55};
        auto result = expr->eval(values);
        REQUIRE(result.has_value());
        REQUIRE(*result == 1);  // ((0xF0 & 0x0F) | 0x55) == 0x55 => (0 | 0x55) == 0x55 => true
    }
}
