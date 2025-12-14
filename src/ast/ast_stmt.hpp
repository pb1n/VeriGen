#pragma once

#include "ast_base.hpp"
#include "ast_expr.hpp"
#include <string>
#include <vector>

namespace ast {

// Assignment statement: lhs = rhs;
class AssignStmt : public Stmt {
public:
    enum class Kind {
        Blocking,        // = (blocking assignment)
        NonBlocking,     // <= (non-blocking assignment)
        Continuous       // assign (continuous assignment)
    };

    AssignStmt(Kind kind, Ptr<Expr> lhs, Ptr<Expr> rhs)
        : kind_(kind), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::string emit() const override {
        std::string op;
        switch (kind_) {
            case Kind::Blocking:    op = " = "; break;
            case Kind::NonBlocking: op = " <= "; break;
            case Kind::Continuous:  return "assign " + lhs_->emit() + " = " + rhs_->emit() + ";";
        }
        return lhs_->emit() + op + rhs_->emit() + ";";
    }

    Kind getKind() const { return kind_; }
    const Expr* getLhs() const { return lhs_.get(); }
    const Expr* getRhs() const { return rhs_.get(); }

private:
    Kind kind_;
    Ptr<Expr> lhs_;
    Ptr<Expr> rhs_;
};

// Block statement: begin ... end
class BlockStmt : public Stmt {
public:
    explicit BlockStmt(std::vector<Ptr<Stmt>> stmts, std::string name = "")
        : stmts_(std::move(stmts)), name_(std::move(name)) {}

    std::string emit() const override {
        std::string result = "begin";
        if (!name_.empty()) {
            result += " : " + name_;
        }
        result += "\n";

        for (const auto& stmt : stmts_) {
            result += "  " + stmt->emit() + "\n";
        }

        result += "end";
        return result;
    }

    const std::vector<Ptr<Stmt>>& getStmts() const { return stmts_; }
    const std::string& getName() const { return name_; }

private:
    std::vector<Ptr<Stmt>> stmts_;
    std::string name_;
};

// If statement: if (cond) then_stmt [else else_stmt]
class IfStmt : public Stmt {
public:
    IfStmt(Ptr<Expr> cond, Ptr<Stmt> then_stmt, Ptr<Stmt> else_stmt = nullptr)
        : cond_(std::move(cond)),
          then_stmt_(std::move(then_stmt)),
          else_stmt_(std::move(else_stmt)) {}

    std::string emit() const override {
        std::string result = "if (" + cond_->emit() + ")\n";
        result += "  " + then_stmt_->emit();

        if (else_stmt_) {
            result += "\nelse\n";
            result += "  " + else_stmt_->emit();
        }

        return result;
    }

    const Expr* getCond() const { return cond_.get(); }
    const Stmt* getThenStmt() const { return then_stmt_.get(); }
    const Stmt* getElseStmt() const { return else_stmt_.get(); }

private:
    Ptr<Expr> cond_;
    Ptr<Stmt> then_stmt_;
    Ptr<Stmt> else_stmt_;
};

// Case statement
class CaseStmt : public Stmt {
public:
    struct CaseItem {
        std::vector<Ptr<Expr>> conditions; // Multiple conditions per case
        Ptr<Stmt> stmt;
    };

    enum class Kind {
        Case,     // case
        Casex,    // casex (don't care X)
        Casez     // casez (don't care Z)
    };

    CaseStmt(Kind kind, Ptr<Expr> expr, std::vector<CaseItem> items, Ptr<Stmt> default_stmt = nullptr)
        : kind_(kind),
          expr_(std::move(expr)),
          items_(std::move(items)),
          default_stmt_(std::move(default_stmt)) {}

    std::string emit() const override {
        std::string result;

        switch (kind_) {
            case Kind::Case:  result = "case"; break;
            case Kind::Casex: result = "casex"; break;
            case Kind::Casez: result = "casez"; break;
        }

        result += " (" + expr_->emit() + ")\n";

        for (const auto& item : items_) {
            result += "  ";
            for (size_t i = 0; i < item.conditions.size(); ++i) {
                if (i > 0) result += ", ";
                result += item.conditions[i]->emit();
            }
            result += ": " + item.stmt->emit() + "\n";
        }

        if (default_stmt_) {
            result += "  default: " + default_stmt_->emit() + "\n";
        }

        result += "endcase";
        return result;
    }

    Kind getKind() const { return kind_; }
    const Expr* getExpr() const { return expr_.get(); }
    const std::vector<CaseItem>& getItems() const { return items_; }
    const Stmt* getDefaultStmt() const { return default_stmt_.get(); }

private:
    Kind kind_;
    Ptr<Expr> expr_;
    std::vector<CaseItem> items_;
    Ptr<Stmt> default_stmt_;
};

// For loop statement
class ForStmt : public Stmt {
public:
    ForStmt(Ptr<Stmt> init, Ptr<Expr> cond, Ptr<Stmt> update, Ptr<Stmt> body)
        : init_(std::move(init)),
          cond_(std::move(cond)),
          update_(std::move(update)),
          body_(std::move(body)) {}

    std::string emit() const override {
        std::string result = "for (";
        result += init_->emit();
        result += " " + cond_->emit() + "; ";
        result += update_->emit();
        result += ")\n";
        result += "  " + body_->emit();
        return result;
    }

    const Stmt* getInit() const { return init_.get(); }
    const Expr* getCond() const { return cond_.get(); }
    const Stmt* getUpdate() const { return update_.get(); }
    const Stmt* getBody() const { return body_.get(); }

private:
    Ptr<Stmt> init_;
    Ptr<Expr> cond_;
    Ptr<Stmt> update_;
    Ptr<Stmt> body_;
};

// While loop statement
class WhileStmt : public Stmt {
public:
    WhileStmt(Ptr<Expr> cond, Ptr<Stmt> body)
        : cond_(std::move(cond)), body_(std::move(body)) {}

    std::string emit() const override {
        return "while (" + cond_->emit() + ")\n  " + body_->emit();
    }

    const Expr* getCond() const { return cond_.get(); }
    const Stmt* getBody() const { return body_.get(); }

private:
    Ptr<Expr> cond_;
    Ptr<Stmt> body_;
};

// Always block
class AlwaysStmt : public Stmt {
public:
    enum class Kind {
        Always,      // always
        AlwaysComb,  // always_comb
        AlwaysFF,    // always_ff
        AlwaysLatch  // always_latch
    };

    struct SensitivityItem {
        enum class Edge {
            None,    // No edge (level-sensitive)
            Pos,     // posedge
            Neg      // negedge
        };

        Edge edge;
        std::string signal;
    };

    AlwaysStmt(Kind kind, std::vector<SensitivityItem> sensitivity, Ptr<Stmt> body)
        : kind_(kind), sensitivity_(std::move(sensitivity)), body_(std::move(body)) {}

    std::string emit() const override {
        std::string result;

        switch (kind_) {
            case Kind::Always:      result = "always"; break;
            case Kind::AlwaysComb:  result = "always_comb"; break;
            case Kind::AlwaysFF:    result = "always_ff"; break;
            case Kind::AlwaysLatch: result = "always_latch"; break;
        }

        if (!sensitivity_.empty()) {
            result += " @(";
            for (size_t i = 0; i < sensitivity_.size(); ++i) {
                if (i > 0) result += " or ";

                const auto& item = sensitivity_[i];
                switch (item.edge) {
                    case SensitivityItem::Edge::Pos:
                        result += "posedge " + item.signal;
                        break;
                    case SensitivityItem::Edge::Neg:
                        result += "negedge " + item.signal;
                        break;
                    case SensitivityItem::Edge::None:
                        result += item.signal;
                        break;
                }
            }
            result += ")";
        }

        result += "\n" + body_->emit();
        return result;
    }

    Kind getKind() const { return kind_; }
    const std::vector<SensitivityItem>& getSensitivity() const { return sensitivity_; }
    const Stmt* getBody() const { return body_.get(); }

private:
    Kind kind_;
    std::vector<SensitivityItem> sensitivity_;
    Ptr<Stmt> body_;
};

// Initial block
class InitialStmt : public Stmt {
public:
    explicit InitialStmt(Ptr<Stmt> body)
        : body_(std::move(body)) {}

    std::string emit() const override {
        return "initial\n" + body_->emit();
    }

    const Stmt* getBody() const { return body_.get(); }

private:
    Ptr<Stmt> body_;
};

} // namespace ast
