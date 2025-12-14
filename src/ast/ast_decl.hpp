#pragma once

#include "ast_base.hpp"
#include "ast_types.hpp"
#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include <string>
#include <vector>

namespace ast {

// Variable declaration (wire, reg, logic, etc.)
class VarDecl : public Decl {
public:
    VarDecl(std::string name, Ptr<Type> type, Ptr<Expr> init = nullptr)
        : name_(std::move(name)), type_(std::move(type)), init_(std::move(init)) {}

    std::string emit() const override {
        std::string result = type_->emit() + " " + name_;
        if (init_) {
            result += " = " + init_->emit();
        }
        result += ";";
        return result;
    }

    std::string getName() const override { return name_; }
    const Type* getType() const { return type_.get(); }
    const Expr* getInit() const { return init_.get(); }

private:
    std::string name_;
    Ptr<Type> type_;
    Ptr<Expr> init_;
};

// Port declaration
class PortDecl : public Decl {
public:
    enum class Direction {
        Input,
        Output,
        Inout
    };

    PortDecl(Direction dir, std::string name, Ptr<Type> type)
        : dir_(dir), name_(std::move(name)), type_(std::move(type)) {}

    std::string emit() const override {
        std::string result;
        switch (dir_) {
            case Direction::Input:  result = "input "; break;
            case Direction::Output: result = "output "; break;
            case Direction::Inout:  result = "inout "; break;
        }
        result += type_->emit() + " " + name_;
        return result;
    }

    std::string getName() const override { return name_; }
    Direction getDirection() const { return dir_; }
    const Type* getType() const { return type_.get(); }

private:
    Direction dir_;
    std::string name_;
    Ptr<Type> type_;
};

// Parameter declaration
class ParamDecl : public Decl {
public:
    enum class Kind {
        Parameter,   // parameter
        LocalParam   // localparam
    };

    ParamDecl(Kind kind, std::string name, Ptr<Expr> value, Ptr<Type> type = nullptr)
        : kind_(kind),
          name_(std::move(name)),
          value_(std::move(value)),
          type_(std::move(type)) {}

    std::string emit() const override {
        return emitWithSemicolon();
    }

    // Emit without semicolon (for module parameter list)
    std::string emitWithoutSemicolon() const {
        std::string result;
        switch (kind_) {
            case Kind::Parameter:  result = "parameter "; break;
            case Kind::LocalParam: result = "localparam "; break;
        }

        if (type_) {
            result += type_->emit() + " ";
        }

        result += name_ + " = " + value_->emit();
        return result;
    }

    // Emit with semicolon (for module body declarations)
    std::string emitWithSemicolon() const {
        return emitWithoutSemicolon() + ";";
    }

    std::string getName() const override { return name_; }
    Kind getKind() const { return kind_; }
    const Expr* getValue() const { return value_.get(); }
    const Type* getType() const { return type_.get(); }

private:
    Kind kind_;
    std::string name_;
    Ptr<Expr> value_;
    Ptr<Type> type_;
};

// Function declaration
class FunctionDecl : public Decl {
public:
    FunctionDecl(std::string name,
                 Ptr<Type> return_type,
                 std::vector<Ptr<PortDecl>> params,
                 std::vector<Ptr<VarDecl>> local_vars,
                 Ptr<Stmt> body)
        : name_(std::move(name)),
          return_type_(std::move(return_type)),
          params_(std::move(params)),
          local_vars_(std::move(local_vars)),
          body_(std::move(body)) {}

    std::string emit() const override {
        std::string result = "function ";
        if (return_type_) {
            result += return_type_->emit() + " ";
        }
        result += name_ + ";\n";

        // Parameters
        for (const auto& param : params_) {
            result += "  " + param->emit() + ";\n";
        }

        // Local variables
        for (const auto& var : local_vars_) {
            result += "  " + var->emit() + "\n";
        }

        // Body
        result += "  " + body_->emit() + "\n";
        result += "endfunction";

        return result;
    }

    std::string getName() const override { return name_; }
    const Type* getReturnType() const { return return_type_.get(); }
    const std::vector<Ptr<PortDecl>>& getParams() const { return params_; }
    const std::vector<Ptr<VarDecl>>& getLocalVars() const { return local_vars_; }
    const Stmt* getBody() const { return body_.get(); }

private:
    std::string name_;
    Ptr<Type> return_type_;
    std::vector<Ptr<PortDecl>> params_;
    std::vector<Ptr<VarDecl>> local_vars_;
    Ptr<Stmt> body_;
};

// Task declaration
class TaskDecl : public Decl {
public:
    TaskDecl(std::string name,
             std::vector<Ptr<PortDecl>> params,
             std::vector<Ptr<VarDecl>> local_vars,
             Ptr<Stmt> body)
        : name_(std::move(name)),
          params_(std::move(params)),
          local_vars_(std::move(local_vars)),
          body_(std::move(body)) {}

    std::string emit() const override {
        std::string result = "task " + name_ + ";\n";

        // Parameters
        for (const auto& param : params_) {
            result += "  " + param->emit() + ";\n";
        }

        // Local variables
        for (const auto& var : local_vars_) {
            result += "  " + var->emit() + "\n";
        }

        // Body
        result += "  " + body_->emit() + "\n";
        result += "endtask";

        return result;
    }

    std::string getName() const override { return name_; }
    const std::vector<Ptr<PortDecl>>& getParams() const { return params_; }
    const std::vector<Ptr<VarDecl>>& getLocalVars() const { return local_vars_; }
    const Stmt* getBody() const { return body_.get(); }

private:
    std::string name_;
    std::vector<Ptr<PortDecl>> params_;
    std::vector<Ptr<VarDecl>> local_vars_;
    Ptr<Stmt> body_;
};

// Genvar declaration (for generate blocks)
class GenvarDecl : public Decl {
public:
    explicit GenvarDecl(std::string name)
        : name_(std::move(name)) {}

    std::string emit() const override {
        return "genvar " + name_ + ";";
    }

    std::string getName() const override { return name_; }

private:
    std::string name_;
};

} // namespace ast
