#pragma once

#include "ast_base.hpp"
#include "ast_decl.hpp"
#include "ast_stmt.hpp"
#include "ast_expr.hpp"
#include <string>
#include <vector>

namespace ast {

// Module instance (instantiation of another module)
class ModuleInstance {
public:
    struct Connection {
        std::string port_name;
        Ptr<Expr> expr;
    };

    ModuleInstance(std::string instance_name,
                   std::vector<Connection> connections,
                   std::vector<Ptr<Expr>> params = {})
        : instance_name_(std::move(instance_name)),
          connections_(std::move(connections)),
          params_(std::move(params)) {}

    std::string emit(const std::string& module_name) const {
        std::string result = module_name;

        // Parameters
        if (!params_.empty()) {
            result += " #(";
            for (size_t i = 0; i < params_.size(); ++i) {
                if (i > 0) result += ", ";
                result += params_[i]->emit();
            }
            result += ")";
        }

        result += " " + instance_name_ + " (";

        // Connections
        for (size_t i = 0; i < connections_.size(); ++i) {
            if (i > 0) result += ", ";
            result += "\n  ." + connections_[i].port_name + "(" + connections_[i].expr->emit() + ")";
        }

        result += "\n);";
        return result;
    }

    const std::string& getInstanceName() const { return instance_name_; }
    const std::vector<Connection>& getConnections() const { return connections_; }
    const std::vector<Ptr<Expr>>& getParams() const { return params_; }

private:
    std::string instance_name_;
    std::vector<Connection> connections_;
    std::vector<Ptr<Expr>> params_;
};

// Module instantiation statement
class ModuleInstStmt : public Stmt {
public:
    ModuleInstStmt(std::string module_name, std::vector<ModuleInstance> instances)
        : module_name_(std::move(module_name)), instances_(std::move(instances)) {}

    std::string emit() const override {
        std::string result;
        for (const auto& inst : instances_) {
            result += inst.emit(module_name_) + "\n";
        }
        return result;
    }

    const std::string& getModuleName() const { return module_name_; }
    const std::vector<ModuleInstance>& getInstances() const { return instances_; }

private:
    std::string module_name_;
    std::vector<ModuleInstance> instances_;
};

// Generate block
class GenerateStmt : public Stmt {
public:
    enum class Kind {
        If,      // if generate
        For,     // for generate
        Case     // case generate
    };

    // For if generate
    GenerateStmt(Ptr<Expr> cond, Ptr<Stmt> then_block, Ptr<Stmt> else_block = nullptr)
        : kind_(Kind::If),
          cond_(std::move(cond)),
          then_block_(std::move(then_block)),
          else_block_(std::move(else_block)) {}

    // For for generate
    GenerateStmt(Ptr<GenvarDecl> genvar, Ptr<Expr> init, Ptr<Expr> cond,
                 Ptr<Stmt> update, Ptr<Stmt> body)
        : kind_(Kind::For),
          cond_(std::move(cond)),
          then_block_(std::move(body)),
          else_block_(nullptr),
          genvar_(std::move(genvar)),
          init_expr_(std::move(init)),
          update_stmt_(std::move(update)) {}

    std::string emit() const override {
        std::string result = "generate\n";

        switch (kind_) {
            case Kind::If:
                result += "  if (" + cond_->emit() + ") begin\n";
                result += "    " + then_block_->emit() + "\n";
                result += "  end";
                if (else_block_) {
                    result += " else begin\n";
                    result += "    " + else_block_->emit() + "\n";
                    result += "  end";
                }
                break;

            case Kind::For:
                result += "  for (" + genvar_->getName() + " = " + init_expr_->emit() + "; ";
                result += cond_->emit() + "; ";
                result += genvar_->getName() + " = " + update_stmt_->emit() + ") begin\n";
                result += "    " + then_block_->emit() + "\n";
                result += "  end";
                break;

            case Kind::Case:
                // Not implemented yet
                break;
        }

        result += "\nendgenerate";
        return result;
    }

    Kind getKind() const { return kind_; }

private:
    Kind kind_;

    // For if generate
    Ptr<Expr> cond_;
    Ptr<Stmt> then_block_;
    Ptr<Stmt> else_block_;

    // For for generate
    Ptr<GenvarDecl> genvar_;
    Ptr<Expr> init_expr_;
    Ptr<Stmt> update_stmt_;
};

// Module definition
class ModuleDefn : public Module {
public:
    ModuleDefn(std::string name,
               std::vector<Ptr<PortDecl>> ports,
               std::vector<Ptr<ParamDecl>> params = {})
        : name_(std::move(name)),
          ports_(std::move(ports)),
          params_(std::move(params)) {}

    // Add various module items
    void addDecl(Ptr<Decl> decl) {
        decls_.push_back(std::move(decl));
    }

    void addStmt(Ptr<Stmt> stmt) {
        stmts_.push_back(std::move(stmt));
    }

    void addFunction(Ptr<FunctionDecl> func) {
        functions_.push_back(std::move(func));
    }

    void addTask(Ptr<TaskDecl> task) {
        tasks_.push_back(std::move(task));
    }

    std::string emit() const override {
        std::string result = "module " + name_;

        // Parameters
        if (!params_.empty()) {
            result += " #(\n";
            for (size_t i = 0; i < params_.size(); ++i) {
                if (i > 0) result += ",\n";
                result += "  " + params_[i]->emitWithoutSemicolon();
            }
            result += "\n)";
        }

        // Ports
        result += " (\n";
        for (size_t i = 0; i < ports_.size(); ++i) {
            if (i > 0) result += ",\n";
            result += "  " + ports_[i]->emit();
        }
        result += "\n);\n\n";

        // Declarations
        for (const auto& decl : decls_) {
            result += "  " + decl->emit() + "\n";
        }
        if (!decls_.empty()) result += "\n";

        // Functions
        for (const auto& func : functions_) {
            result += "  " + func->emit() + "\n\n";
        }

        // Tasks
        for (const auto& task : tasks_) {
            result += "  " + task->emit() + "\n\n";
        }

        // Statements (always blocks, assigns, etc.)
        for (const auto& stmt : stmts_) {
            result += "  " + stmt->emit() + "\n\n";
        }

        result += "endmodule";
        return result;
    }

    std::string getName() const override { return name_; }

    const std::vector<Ptr<PortDecl>>& getPorts() const { return ports_; }
    const std::vector<Ptr<ParamDecl>>& getParams() const { return params_; }
    const std::vector<Ptr<Decl>>& getDecls() const { return decls_; }
    const std::vector<Ptr<Stmt>>& getStmts() const { return stmts_; }
    const std::vector<Ptr<FunctionDecl>>& getFunctions() const { return functions_; }
    const std::vector<Ptr<TaskDecl>>& getTasks() const { return tasks_; }

private:
    std::string name_;
    std::vector<Ptr<PortDecl>> ports_;
    std::vector<Ptr<ParamDecl>> params_;
    std::vector<Ptr<Decl>> decls_;
    std::vector<Ptr<Stmt>> stmts_;
    std::vector<Ptr<FunctionDecl>> functions_;
    std::vector<Ptr<TaskDecl>> tasks_;
};

// Top-level design (collection of modules)
class Design {
public:
    void addModule(Ptr<ModuleDefn> module) {
        modules_.push_back(std::move(module));
    }

    std::string emit() const {
        std::string result;
        for (const auto& module : modules_) {
            result += module->emit() + "\n\n";
        }
        return result;
    }

    const std::vector<Ptr<ModuleDefn>>& getModules() const { return modules_; }

private:
    std::vector<Ptr<ModuleDefn>> modules_;
};

} // namespace ast
