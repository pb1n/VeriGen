#pragma once

#include "ast_base.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace ast {

// Literal expression (constant value)
class LiteralExpr : public Expr {
public:
    explicit LiteralExpr(uint32_t value, uint32_t width = 32)
        : value_(value), width_(width) {}

    std::string emit() const override {
        if (width_ == 32) {
            return std::to_string(value_);
        }
        return std::to_string(width_) + "'d" + std::to_string(value_);
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        (void)values; // Unused
        return value_;
    }

    uint32_t getValue() const { return value_; }
    uint32_t getWidth() const { return width_; }

private:
    uint32_t value_;
    uint32_t width_;
};

// Variable reference expression
class VarExpr : public Expr {
public:
    explicit VarExpr(std::string name, uint32_t index = 0)
        : name_(std::move(name)), index_(index) {}

    std::string emit() const override {
        return name_;
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        if (index_ >= values.size()) {
            return std::nullopt; // Index out of bounds
        }
        return values[index_];
    }

    const std::string& getName() const { return name_; }
    uint32_t getIndex() const { return index_; }

private:
    std::string name_;
    uint32_t index_; // Index into evaluation vector
};

// Binary operation expression
class BinaryExpr : public Expr {
public:
    enum class Op {
        // Arithmetic
        Add, Sub, Mul, Div, Mod,
        // Bitwise
        And, Or, Xor, Shl, Shr, AShr,
        // Logical
        LAnd, LOr,
        // Comparison
        Eq, Neq, Lt, Le, Gt, Ge
    };

    BinaryExpr(Op op, Ptr<Expr> left, Ptr<Expr> right)
        : op_(op), left_(std::move(left)), right_(std::move(right)) {}

    std::string emit() const override {
        return "(" + left_->emit() + " " + opToString(op_) + " " + right_->emit() + ")";
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        auto lval = left_->eval(values);
        auto rval = right_->eval(values);

        if (!lval || !rval) {
            return std::nullopt;
        }

        uint32_t l = *lval;
        uint32_t r = *rval;

        switch (op_) {
            // Arithmetic
            case Op::Add:  return l + r;
            case Op::Sub:  return l - r;
            case Op::Mul:  return l * r;
            case Op::Div:  return (r == 0) ? std::nullopt : std::optional<uint32_t>(l / r);
            case Op::Mod:  return (r == 0) ? std::nullopt : std::optional<uint32_t>(l % r);
            // Bitwise
            case Op::And:  return l & r;
            case Op::Or:   return l | r;
            case Op::Xor:  return l ^ r;
            case Op::Shl:  return l << r;
            case Op::Shr:  return l >> r;
            case Op::AShr: return static_cast<uint32_t>(static_cast<int32_t>(l) >> r);
            // Logical
            case Op::LAnd: return (l && r) ? 1 : 0;
            case Op::LOr:  return (l || r) ? 1 : 0;
            // Comparison
            case Op::Eq:   return (l == r) ? 1 : 0;
            case Op::Neq:  return (l != r) ? 1 : 0;
            case Op::Lt:   return (l < r) ? 1 : 0;
            case Op::Le:   return (l <= r) ? 1 : 0;
            case Op::Gt:   return (l > r) ? 1 : 0;
            case Op::Ge:   return (l >= r) ? 1 : 0;
        }

        return std::nullopt;
    }

    Op getOp() const { return op_; }
    const Expr* getLeft() const { return left_.get(); }
    const Expr* getRight() const { return right_.get(); }

private:
    static std::string opToString(Op op) {
        switch (op) {
            case Op::Add:  return "+";
            case Op::Sub:  return "-";
            case Op::Mul:  return "*";
            case Op::Div:  return "/";
            case Op::Mod:  return "%";
            case Op::And:  return "&";
            case Op::Or:   return "|";
            case Op::Xor:  return "^";
            case Op::Shl:  return "<<";
            case Op::Shr:  return ">>";
            case Op::AShr: return ">>>";
            case Op::LAnd: return "&&";
            case Op::LOr:  return "||";
            case Op::Eq:   return "==";
            case Op::Neq:  return "!=";
            case Op::Lt:   return "<";
            case Op::Le:   return "<=";
            case Op::Gt:   return ">";
            case Op::Ge:   return ">=";
        }
        return "?";
    }

    Op op_;
    Ptr<Expr> left_;
    Ptr<Expr> right_;
};

// Unary operation expression
class UnaryExpr : public Expr {
public:
    enum class Op {
        Neg,    // -x (arithmetic negation)
        Not,    // ~x (bitwise NOT)
        LNot,   // !x (logical NOT)
        RedAnd, // &x (reduction AND)
        RedOr,  // |x (reduction OR)
        RedXor  // ^x (reduction XOR)
    };

    UnaryExpr(Op op, Ptr<Expr> operand)
        : op_(op), operand_(std::move(operand)) {}

    std::string emit() const override {
        return opToString(op_) + operand_->emit();
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        auto val = operand_->eval(values);
        if (!val) {
            return std::nullopt;
        }

        uint32_t v = *val;

        switch (op_) {
            case Op::Neg:    return -v;
            case Op::Not:    return ~v;
            case Op::LNot:   return v ? 0 : 1;
            case Op::RedAnd: return reductionAnd(v);
            case Op::RedOr:  return reductionOr(v);
            case Op::RedXor: return reductionXor(v);
        }

        return std::nullopt;
    }

    Op getOp() const { return op_; }
    const Expr* getOperand() const { return operand_.get(); }

private:
    static std::string opToString(Op op) {
        switch (op) {
            case Op::Neg:    return "-";
            case Op::Not:    return "~";
            case Op::LNot:   return "!";
            case Op::RedAnd: return "&";
            case Op::RedOr:  return "|";
            case Op::RedXor: return "^";
        }
        return "?";
    }

    static uint32_t reductionAnd(uint32_t v) {
        return (v == 0xFFFFFFFF) ? 1 : 0;
    }

    static uint32_t reductionOr(uint32_t v) {
        return (v != 0) ? 1 : 0;
    }

    static uint32_t reductionXor(uint32_t v) {
        uint32_t result = 0;
        for (int i = 0; i < 32; ++i) {
            result ^= (v >> i) & 1;
        }
        return result;
    }

    Op op_;
    Ptr<Expr> operand_;
};

// Conditional (ternary) expression: cond ? true_expr : false_expr
class ConditionalExpr : public Expr {
public:
    ConditionalExpr(Ptr<Expr> cond, Ptr<Expr> true_expr, Ptr<Expr> false_expr)
        : cond_(std::move(cond)),
          true_expr_(std::move(true_expr)),
          false_expr_(std::move(false_expr)) {}

    std::string emit() const override {
        return "(" + cond_->emit() + " ? " +
               true_expr_->emit() + " : " +
               false_expr_->emit() + ")";
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        auto cond_val = cond_->eval(values);
        if (!cond_val) {
            return std::nullopt;
        }

        if (*cond_val) {
            return true_expr_->eval(values);
        } else {
            return false_expr_->eval(values);
        }
    }

    const Expr* getCond() const { return cond_.get(); }
    const Expr* getTrueExpr() const { return true_expr_.get(); }
    const Expr* getFalseExpr() const { return false_expr_.get(); }

private:
    Ptr<Expr> cond_;
    Ptr<Expr> true_expr_;
    Ptr<Expr> false_expr_;
};

// Bit-select expression: signal[index]
class BitSelectExpr : public Expr {
public:
    BitSelectExpr(Ptr<Expr> base, Ptr<Expr> index)
        : base_(std::move(base)), index_(std::move(index)) {}

    std::string emit() const override {
        return base_->emit() + "[" + index_->emit() + "]";
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        auto base_val = base_->eval(values);
        auto index_val = index_->eval(values);

        if (!base_val || !index_val) {
            return std::nullopt;
        }

        uint32_t idx = *index_val;
        if (idx >= 32) {
            return std::nullopt; // Out of bounds
        }

        return (*base_val >> idx) & 1;
    }

    const Expr* getBase() const { return base_.get(); }
    const Expr* getIndex() const { return index_.get(); }

private:
    Ptr<Expr> base_;
    Ptr<Expr> index_;
};

// Range-select expression: signal[msb:lsb]
class RangeSelectExpr : public Expr {
public:
    RangeSelectExpr(Ptr<Expr> base, Ptr<Expr> msb, Ptr<Expr> lsb)
        : base_(std::move(base)), msb_(std::move(msb)), lsb_(std::move(lsb)) {}

    std::string emit() const override {
        return base_->emit() + "[" + msb_->emit() + ":" + lsb_->emit() + "]";
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        auto base_val = base_->eval(values);
        auto msb_val = msb_->eval(values);
        auto lsb_val = lsb_->eval(values);

        if (!base_val || !msb_val || !lsb_val) {
            return std::nullopt;
        }

        uint32_t m = *msb_val;
        uint32_t l = *lsb_val;

        if (m >= 32 || l > m) {
            return std::nullopt; // Invalid range
        }

        uint32_t width = m - l + 1;
        uint32_t mask = (width == 32) ? 0xFFFFFFFF : ((1u << width) - 1);

        return (*base_val >> l) & mask;
    }

    const Expr* getBase() const { return base_.get(); }
    const Expr* getMsb() const { return msb_.get(); }
    const Expr* getLsb() const { return lsb_.get(); }

private:
    Ptr<Expr> base_;
    Ptr<Expr> msb_;
    Ptr<Expr> lsb_;
};

// Concatenation expression: {expr1, expr2, ...}
class ConcatExpr : public Expr {
public:
    explicit ConcatExpr(std::vector<Ptr<Expr>> exprs)
        : exprs_(std::move(exprs)) {}

    std::string emit() const override {
        std::string result = "{";
        for (size_t i = 0; i < exprs_.size(); ++i) {
            if (i > 0) result += ", ";
            result += exprs_[i]->emit();
        }
        result += "}";
        return result;
    }

    std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const override {
        // Simplified: concatenate bits (assumes each expr is 1 bit for simplicity)
        uint32_t result = 0;
        for (const auto& expr : exprs_) {
            auto val = expr->eval(values);
            if (!val) {
                return std::nullopt;
            }
            result = (result << 1) | (*val & 1);
        }
        return result;
    }

    const std::vector<Ptr<Expr>>& getExprs() const { return exprs_; }

private:
    std::vector<Ptr<Expr>> exprs_;
};

} // namespace ast
