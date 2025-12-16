#pragma once

#include "ast_base.hpp"
#include "ast_decl.hpp"
#include "ast_stmt.hpp"
#include "ast_expr.hpp"
#include <string>
#include <vector>
#include <sstream>

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
                 Ptr<Expr> update, Ptr<Stmt> body)
        : kind_(Kind::For),
          cond_(std::move(cond)),
          then_block_(std::move(body)),
          else_block_(nullptr),
          genvar_(std::move(genvar)),
          init_expr_(std::move(init)),
          update_expr_(std::move(update)) {}
          
    // For case generate
    struct CaseItem {
        std::vector<Ptr<Expr>> values;
        Ptr<Stmt> body;
    };

    GenerateStmt(Ptr<Expr> case_expr, std::vector<CaseItem> items, Ptr<Stmt> default_stmt = nullptr)
        : kind_(Kind::Case),
          // Initialize unused members in declaration order
          cond_(nullptr), then_block_(nullptr), else_block_(nullptr),
          genvar_(nullptr), init_expr_(nullptr), update_expr_(nullptr),
          case_expr_(std::move(case_expr)),
          case_items_(std::move(items)),
          case_default_(std::move(default_stmt)) {}

    std::string emit() const override {
        return "generate\n" + indent(emitBody(), "  ") + "\nendgenerate";
    }

    // Emit just the generate construct content without wrapper (unindented)
    std::string emitBody() const {
        std::string result;

        switch (kind_) {
            case Kind::If:
                result += "if (" + cond_->emit() + ") begin\n";
                result += indent(emitStmtBody(then_block_.get()), "  ") + "\n";
                result += "end";
                if (else_block_) {
                    result += " else begin\n";
                    result += indent(emitStmtBody(else_block_.get()), "  ") + "\n";
                    result += "end";
                }
                break;

            case Kind::For:
                result += "for (" + genvar_->getName() + " = " + init_expr_->emit() + "; ";
                result += cond_->emit() + "; ";
                result += genvar_->getName() + " = " + update_expr_->emit() + ") begin\n";
                result += indent(emitStmtBody(then_block_.get()), "  ") + "\n";
                result += "end";
                break;

            case Kind::Case:
                // 1. Emit the switch expression
                result += "case (" + case_expr_->emit() + ")\n";

                // 2. Emit each case item
                for (const auto& item : case_items_) {
                    result += "  ";
                    // Handle comma-separated values: "val1, val2"
                    for (size_t i = 0; i < item.values.size(); ++i) {
                        result += item.values[i]->emit();
                        if (i < item.values.size() - 1) {
                            result += ", ";
                        }
                    }
                    result += ": begin\n";
                    if (item.body) {
                        result += "    " + item.body->emit() + "\n";
                    }
                    result += "  end\n";
                }

                // 3. Emit default block if it exists
                if (case_default_) {
                    result += "  default: begin\n";
                    result += "    " + case_default_->emit() + "\n";
                    result += "  end\n";
                }

                result += "endcase\n";
                break;
        }

        return result;
    }

private:
    // Helper to indent a multi-line string
    static std::string indent(const std::string& str, const std::string& prefix) {
        if (str.empty()) return str;

        std::string result;
        size_t start = 0;
        size_t end = str.find('\n');

        while (end != std::string::npos) {
            if (!result.empty()) result += "\n";
            result += prefix + str.substr(start, end - start);
            start = end + 1;
            end = str.find('\n', start);
        }

        // Handle the last line (or only line if no newlines)
        if (!result.empty()) result += "\n";
        result += prefix + str.substr(start);

        return result;
    }

    // Helper to emit statement body - if it's a GenerateStmt, emit without wrapper
    static std::string emitStmtBody(const Stmt* stmt) {
        if (auto* gen = dynamic_cast<const GenerateStmt*>(stmt)) {
            return gen->emitBody();
        }
        return stmt->emit();
    }

public:

    Kind getKind() const { return kind_; }

    // Accessors for visitor pattern
    const Expr* getCond() const { return cond_.get(); }
    const Stmt* getThenBlock() const { return then_block_.get(); }
    const Stmt* getElseBlock() const { return else_block_.get(); }
    const GenvarDecl* getGenvar() const { return genvar_.get(); }
    const Expr* getInitExpr() const { return init_expr_.get(); }
    const Expr* getUpdateExpr() const { return update_expr_.get(); }
    const Expr* getCaseExpr() const { return case_expr_.get(); }
    const std::vector<CaseItem>& getCaseItems() const { return case_items_; }
    const Stmt* getCaseDefault() const { return case_default_.get(); }

private:
    Kind kind_;

    // For if generate
    Ptr<Expr> cond_;
    Ptr<Stmt> then_block_;
    Ptr<Stmt> else_block_;

    // For for generate
    Ptr<GenvarDecl> genvar_;
    Ptr<Expr> init_expr_;
    Ptr<Expr> update_expr_;
    
    // For case generate
    Ptr<Expr> case_expr_;
    std::vector<CaseItem> case_items_;
    Ptr<Stmt> case_default_;
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
