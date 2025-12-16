#include "ast_base.hpp"
#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_decl.hpp"
#include "ast_types.hpp"
#include "ast_module.hpp"
#include "ast_visitor.hpp"

namespace ast {

// Base class accept implementations
void Expr::accept(Visitor& visitor) {
    // Dispatch to appropriate derived class
    if (auto* p = dynamic_cast<LiteralExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<VarExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<BinaryExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<UnaryExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<ConditionalExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<BitSelectExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<RangeSelectExpr*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<ConcatExpr*>(this)) {
        visitor.visit(*p);
    }
}

void Stmt::accept(Visitor& visitor) {
    if (auto* p = dynamic_cast<AssignStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<BlockStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<IfStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<CaseStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<ForStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<WhileStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<AlwaysStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<InitialStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<ModuleInstStmt*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<GenerateStmt*>(this)) {
        visitor.visit(*p);
    }
}

void Decl::accept(Visitor& visitor) {
    if (auto* p = dynamic_cast<VarDecl*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<PortDecl*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<ParamDecl*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<FunctionDecl*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<TaskDecl*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<GenvarDecl*>(this)) {
        visitor.visit(*p);
    }
}

void Type::accept(Visitor& visitor) {
    if (auto* p = dynamic_cast<IntType*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<ArrayType*>(this)) {
        visitor.visit(*p);
    } else if (auto* p = dynamic_cast<PackedArrayType*>(this)) {
        visitor.visit(*p);
    }
}

void Module::accept(Visitor& visitor) {
    if (auto* p = dynamic_cast<ModuleDefn*>(this)) {
        visitor.visit(*p);
    }
}

// RecursiveVisitor implementations
void RecursiveVisitor::visit(BinaryExpr& node) {
    if (node.getLeft()) {
        const_cast<Expr*>(node.getLeft())->accept(*this);
    }
    if (node.getRight()) {
        const_cast<Expr*>(node.getRight())->accept(*this);
    }
}

void RecursiveVisitor::visit(UnaryExpr& node) {
    if (node.getOperand()) {
        const_cast<Expr*>(node.getOperand())->accept(*this);
    }
}

void RecursiveVisitor::visit(ConditionalExpr& node) {
    if (node.getCond()) {
        const_cast<Expr*>(node.getCond())->accept(*this);
    }
    if (node.getTrueExpr()) {
        const_cast<Expr*>(node.getTrueExpr())->accept(*this);
    }
    if (node.getFalseExpr()) {
        const_cast<Expr*>(node.getFalseExpr())->accept(*this);
    }
}

void RecursiveVisitor::visit(BitSelectExpr& node) {
    if (node.getBase()) {
        const_cast<Expr*>(node.getBase())->accept(*this);
    }
    if (node.getIndex()) {
        const_cast<Expr*>(node.getIndex())->accept(*this);
    }
}

void RecursiveVisitor::visit(RangeSelectExpr& node) {
    if (node.getBase()) {
        const_cast<Expr*>(node.getBase())->accept(*this);
    }
    if (node.getMsb()) {
        const_cast<Expr*>(node.getMsb())->accept(*this);
    }
    if (node.getLsb()) {
        const_cast<Expr*>(node.getLsb())->accept(*this);
    }
}

void RecursiveVisitor::visit(ConcatExpr& node) {
    for (const auto& expr : node.getExprs()) {
        expr->accept(*this);
    }
}

void RecursiveVisitor::visit(AssignStmt& node) {
    if (node.getLhs()) {
        const_cast<Expr*>(node.getLhs())->accept(*this);
    }
    if (node.getRhs()) {
        const_cast<Expr*>(node.getRhs())->accept(*this);
    }
}

void RecursiveVisitor::visit(BlockStmt& node) {
    for (const auto& stmt : node.getStmts()) {
        stmt->accept(*this);
    }
}

void RecursiveVisitor::visit(IfStmt& node) {
    if (node.getCond()) {
        const_cast<Expr*>(node.getCond())->accept(*this);
    }
    if (node.getThenStmt()) {
        const_cast<Stmt*>(node.getThenStmt())->accept(*this);
    }
    if (node.getElseStmt()) {
        const_cast<Stmt*>(node.getElseStmt())->accept(*this);
    }
}

void RecursiveVisitor::visit(AlwaysStmt& node) {
    if (node.getBody()) {
        const_cast<Stmt*>(node.getBody())->accept(*this);
    }
}

void RecursiveVisitor::visit(InitialStmt& node) {
    if (node.getBody()) {
        const_cast<Stmt*>(node.getBody())->accept(*this);
    }
}

void RecursiveVisitor::visit(CaseStmt& node) {
    if (node.getExpr()) {
        const_cast<Expr*>(node.getExpr())->accept(*this);
    }
    for (const auto& item : node.getItems()) {
        for (const auto& cond : item.conditions) {
            cond->accept(*this);
        }
        if (item.stmt) {
            item.stmt->accept(*this);
        }
    }
    if (node.getDefaultStmt()) {
        const_cast<Stmt*>(node.getDefaultStmt())->accept(*this);
    }
}

void RecursiveVisitor::visit(ForStmt& node) {
    if (node.getInit()) {
        const_cast<Stmt*>(node.getInit())->accept(*this);
    }
    if (node.getCond()) {
        const_cast<Expr*>(node.getCond())->accept(*this);
    }
    if (node.getUpdate()) {
        const_cast<Stmt*>(node.getUpdate())->accept(*this);
    }
    if (node.getBody()) {
        const_cast<Stmt*>(node.getBody())->accept(*this);
    }
}

void RecursiveVisitor::visit(WhileStmt& node) {
    if (node.getCond()) {
        const_cast<Expr*>(node.getCond())->accept(*this);
    }
    if (node.getBody()) {
        const_cast<Stmt*>(node.getBody())->accept(*this);
    }
}

void RecursiveVisitor::visit(GenerateStmt& node) {
    switch (node.getKind()) {
        case GenerateStmt::Kind::If:
            if (node.getCond()) {
                const_cast<Expr*>(node.getCond())->accept(*this);
            }
            if (node.getThenBlock()) {
                const_cast<Stmt*>(node.getThenBlock())->accept(*this);
            }
            if (node.getElseBlock()) {
                const_cast<Stmt*>(node.getElseBlock())->accept(*this);
            }
            break;

        case GenerateStmt::Kind::For:
            if (node.getGenvar()) {
                const_cast<GenvarDecl*>(node.getGenvar())->accept(*this);
            }
            if (node.getInitExpr()) {
                const_cast<Expr*>(node.getInitExpr())->accept(*this);
            }
            if (node.getCond()) {
                const_cast<Expr*>(node.getCond())->accept(*this);
            }
            if (node.getUpdateExpr()) {
                const_cast<Expr*>(node.getUpdateExpr())->accept(*this);
            }
            if (node.getThenBlock()) {
                const_cast<Stmt*>(node.getThenBlock())->accept(*this);
            }
            break;

        case GenerateStmt::Kind::Case:
            if (node.getCaseExpr()) {
                const_cast<Expr*>(node.getCaseExpr())->accept(*this);
            }
            for (const auto& item : node.getCaseItems()) {
                for (const auto& val : item.values) {
                    val->accept(*this);
                }
                if (item.body) {
                    item.body->accept(*this);
                }
            }
            if (node.getCaseDefault()) {
                const_cast<Stmt*>(node.getCaseDefault())->accept(*this);
            }
            break;
    }
}

void RecursiveVisitor::visit(ModuleDefn& node) {
    for (const auto& port : node.getPorts()) {
        port->accept(*this);
    }
    for (const auto& param : node.getParams()) {
        param->accept(*this);
    }
    for (const auto& decl : node.getDecls()) {
        decl->accept(*this);
    }
    for (const auto& stmt : node.getStmts()) {
        stmt->accept(*this);
    }
    for (const auto& func : node.getFunctions()) {
        func->accept(*this);
    }
    for (const auto& task : node.getTasks()) {
        task->accept(*this);
    }
}

// EmitVisitor implementations - Expressions
void EmitVisitor::visit(LiteralExpr& node) {
    if (node.getWidth() == 32) {
        result_ = std::to_string(node.getValue());
    } else {
        result_ = std::to_string(node.getWidth()) + "'d" + std::to_string(node.getValue());
    }
}

void EmitVisitor::visit(VarExpr& node) {
    result_ = node.getName();
}

void EmitVisitor::visit(BinaryExpr& node) {
    std::string op_str;
    switch (node.getOp()) {
        case BinaryExpr::Op::Add:  op_str = "+"; break;
        case BinaryExpr::Op::Sub:  op_str = "-"; break;
        case BinaryExpr::Op::Mul:  op_str = "*"; break;
        case BinaryExpr::Op::Div:  op_str = "/"; break;
        case BinaryExpr::Op::Mod:  op_str = "%"; break;
        case BinaryExpr::Op::And:  op_str = "&"; break;
        case BinaryExpr::Op::Or:   op_str = "|"; break;
        case BinaryExpr::Op::Xor:  op_str = "^"; break;
        case BinaryExpr::Op::Shl:  op_str = "<<"; break;
        case BinaryExpr::Op::Shr:  op_str = ">>"; break;
        case BinaryExpr::Op::AShr: op_str = ">>>"; break;
        case BinaryExpr::Op::LAnd: op_str = "&&"; break;
        case BinaryExpr::Op::LOr:  op_str = "||"; break;
        case BinaryExpr::Op::Eq:   op_str = "=="; break;
        case BinaryExpr::Op::Neq:  op_str = "!="; break;
        case BinaryExpr::Op::Lt:   op_str = "<"; break;
        case BinaryExpr::Op::Le:   op_str = "<="; break;
        case BinaryExpr::Op::Gt:   op_str = ">"; break;
        case BinaryExpr::Op::Ge:   op_str = ">="; break;
    }

    result_ = "(" + visitExpr(const_cast<Expr*>(node.getLeft())) + " " + op_str + " " +
              visitExpr(const_cast<Expr*>(node.getRight())) + ")";
}

void EmitVisitor::visit(UnaryExpr& node) {
    std::string op_str;
    switch (node.getOp()) {
        case UnaryExpr::Op::Neg:    op_str = "-"; break;
        case UnaryExpr::Op::Not:    op_str = "~"; break;
        case UnaryExpr::Op::LNot:   op_str = "!"; break;
        case UnaryExpr::Op::RedAnd: op_str = "&"; break;
        case UnaryExpr::Op::RedOr:  op_str = "|"; break;
        case UnaryExpr::Op::RedXor: op_str = "^"; break;
    }

    result_ = "(" + op_str + visitExpr(const_cast<Expr*>(node.getOperand())) + ")";
}

void EmitVisitor::visit(ConditionalExpr& node) {
    result_ = "(" + visitExpr(const_cast<Expr*>(node.getCond())) + " ? " +
              visitExpr(const_cast<Expr*>(node.getTrueExpr())) + " : " +
              visitExpr(const_cast<Expr*>(node.getFalseExpr())) + ")";
}

void EmitVisitor::visit(BitSelectExpr& node) {
    result_ = visitExpr(const_cast<Expr*>(node.getBase())) + "[" +
              visitExpr(const_cast<Expr*>(node.getIndex())) + "]";
}

void EmitVisitor::visit(RangeSelectExpr& node) {
    result_ = visitExpr(const_cast<Expr*>(node.getBase())) + "[" +
              visitExpr(const_cast<Expr*>(node.getMsb())) + ":" +
              visitExpr(const_cast<Expr*>(node.getLsb())) + "]";
}

void EmitVisitor::visit(ConcatExpr& node) {
    result_ = "{";
    const auto& exprs = node.getExprs();
    for (size_t i = 0; i < exprs.size(); ++i) {
        if (i > 0) result_ += ", ";
        result_ += visitExpr(exprs[i].get());
    }
    result_ += "}";
}

// EmitVisitor implementations - Statements
void EmitVisitor::visit(AssignStmt& node) {
    std::string op;
    switch (node.getKind()) {
        case AssignStmt::Kind::Blocking:    op = " = "; break;
        case AssignStmt::Kind::NonBlocking: op = " <= "; break;
        case AssignStmt::Kind::Continuous:
            result_ = "assign " + visitExpr(const_cast<Expr*>(node.getLhs())) + " = " +
                      visitExpr(const_cast<Expr*>(node.getRhs())) + ";";
            return;
    }
    result_ = visitExpr(const_cast<Expr*>(node.getLhs())) + op +
              visitExpr(const_cast<Expr*>(node.getRhs())) + ";";
}

void EmitVisitor::visit(BlockStmt& node) {
    result_ = "begin";
    if (!node.getName().empty()) {
        result_ += " : " + node.getName();
    }
    result_ += "\n";

    for (const auto& stmt : node.getStmts()) {
        result_ += "  " + visitStmt(stmt.get()) + "\n";
    }

    result_ += "end";
}

void EmitVisitor::visit(IfStmt& node) {
    result_ = "if (" + visitExpr(const_cast<Expr*>(node.getCond())) + ")\n";
    result_ += "  " + visitStmt(const_cast<Stmt*>(node.getThenStmt()));

    if (node.getElseStmt()) {
        result_ += "\nelse\n";
        result_ += "  " + visitStmt(const_cast<Stmt*>(node.getElseStmt()));
    }
}

void EmitVisitor::visit(CaseStmt& node) {
    std::string case_type;
    switch (node.getKind()) {
        case CaseStmt::Kind::Case:  case_type = "case"; break;
        case CaseStmt::Kind::Casex: case_type = "casex"; break;
        case CaseStmt::Kind::Casez: case_type = "casez"; break;
    }

    result_ = case_type + " (" + visitExpr(const_cast<Expr*>(node.getExpr())) + ")\n";

    for (const auto& item : node.getItems()) {
        result_ += "  ";
        for (size_t i = 0; i < item.conditions.size(); ++i) {
            if (i > 0) result_ += ", ";
            result_ += visitExpr(item.conditions[i].get());
        }
        result_ += ": " + visitStmt(item.stmt.get()) + "\n";
    }

    if (node.getDefaultStmt()) {
        result_ += "  default: " + visitStmt(const_cast<Stmt*>(node.getDefaultStmt())) + "\n";
    }

    result_ += "endcase";
}

void EmitVisitor::visit(ForStmt& node) {
    result_ = "for (";
    result_ += visitStmt(const_cast<Stmt*>(node.getInit()));
    result_ += " " + visitExpr(const_cast<Expr*>(node.getCond())) + "; ";
    result_ += visitStmt(const_cast<Stmt*>(node.getUpdate()));
    result_ += ")\n";
    result_ += "  " + visitStmt(const_cast<Stmt*>(node.getBody()));
}

void EmitVisitor::visit(WhileStmt& node) {
    result_ = "while (" + visitExpr(const_cast<Expr*>(node.getCond())) + ")\n";
    result_ += "  " + visitStmt(const_cast<Stmt*>(node.getBody()));
}

void EmitVisitor::visit(AlwaysStmt& node) {
    switch (node.getKind()) {
        case AlwaysStmt::Kind::Always:      result_ = "always"; break;
        case AlwaysStmt::Kind::AlwaysComb:  result_ = "always_comb"; break;
        case AlwaysStmt::Kind::AlwaysFF:    result_ = "always_ff"; break;
        case AlwaysStmt::Kind::AlwaysLatch: result_ = "always_latch"; break;
    }

    const auto& sensitivity = node.getSensitivity();
    if (!sensitivity.empty()) {
        result_ += " @(";
        for (size_t i = 0; i < sensitivity.size(); ++i) {
            if (i > 0) result_ += " or ";

            const auto& item = sensitivity[i];
            switch (item.edge) {
                case AlwaysStmt::SensitivityItem::Edge::Pos:
                    result_ += "posedge " + item.signal;
                    break;
                case AlwaysStmt::SensitivityItem::Edge::Neg:
                    result_ += "negedge " + item.signal;
                    break;
                case AlwaysStmt::SensitivityItem::Edge::None:
                    result_ += item.signal;
                    break;
            }
        }
        result_ += ")";
    }

    result_ += "\n  " + visitStmt(const_cast<Stmt*>(node.getBody()));
}

void EmitVisitor::visit(InitialStmt& node) {
    result_ = "initial\n";
    result_ += "  " + visitStmt(const_cast<Stmt*>(node.getBody()));
}

void EmitVisitor::visit(GenerateStmt& node) {
    std::string body;

    switch (node.getKind()) {
        case GenerateStmt::Kind::If:
            body += "if (" + visitExpr(const_cast<Expr*>(node.getCond())) + ") begin\n";
            body += indentString(visitStmtBody(const_cast<Stmt*>(node.getThenBlock())), "  ") + "\n";
            body += "end";
            if (node.getElseBlock()) {
                body += " else begin\n";
                body += indentString(visitStmtBody(const_cast<Stmt*>(node.getElseBlock())), "  ") + "\n";
                body += "end";
            }
            break;

        case GenerateStmt::Kind::For:
            body += "for (" + node.getGenvar()->getName() + " = " + visitExpr(const_cast<Expr*>(node.getInitExpr())) + "; ";
            body += visitExpr(const_cast<Expr*>(node.getCond())) + "; ";
            body += node.getGenvar()->getName() + " = " + visitExpr(const_cast<Expr*>(node.getUpdateExpr())) + ") begin\n";
            body += indentString(visitStmtBody(const_cast<Stmt*>(node.getThenBlock())), "  ") + "\n";
            body += "end";
            break;

        case GenerateStmt::Kind::Case:
            body += "case (" + visitExpr(const_cast<Expr*>(node.getCaseExpr())) + ")\n";

            for (const auto& item : node.getCaseItems()) {
                body += "  ";
                for (size_t i = 0; i < item.values.size(); ++i) {
                    body += visitExpr(item.values[i].get());
                    if (i < item.values.size() - 1) {
                        body += ", ";
                    }
                }
                body += ": begin\n";
                if (item.body) {
                    body += "    " + visitStmt(item.body.get()) + "\n";
                }
                body += "  end\n";
            }

            if (node.getCaseDefault()) {
                body += "  default: begin\n";
                body += "    " + visitStmt(const_cast<Stmt*>(node.getCaseDefault())) + "\n";
                body += "  end\n";
            }

            body += "endcase\n";
            break;
    }

    // If we're at the top level, wrap with generate/endgenerate
    if (emit_generate_wrapper_) {
        result_ = "generate\n" + indentString(body, "  ") + "\nendgenerate";
    } else {
        result_ = body;
    }
}

// EmitVisitor implementations - Declarations
void EmitVisitor::visit(VarDecl& node) {
    result_ = node.getType()->emit() + " " + node.getName();
    if (node.getInit()) {
        result_ += " = " + visitExpr(const_cast<Expr*>(node.getInit()));
    }
    result_ += ";";
}

void EmitVisitor::visit(PortDecl& node) {
    switch (node.getDirection()) {
        case PortDecl::Direction::Input:  result_ = "input "; break;
        case PortDecl::Direction::Output: result_ = "output "; break;
        case PortDecl::Direction::Inout:  result_ = "inout "; break;
    }
    result_ += node.getType()->emit() + " " + node.getName();
}

void EmitVisitor::visit(ParamDecl& node) {
    switch (node.getKind()) {
        case ParamDecl::Kind::Parameter:  result_ = "parameter "; break;
        case ParamDecl::Kind::LocalParam: result_ = "localparam "; break;
    }

    if (node.getType()) {
        result_ += node.getType()->emit() + " ";
    }

    result_ += node.getName() + " = " + visitExpr(const_cast<Expr*>(node.getValue())) + ";";
}

void EmitVisitor::visit(GenvarDecl& node) {
    result_ = "genvar " + node.getName() + ";";
}

// EmitVisitor implementations - Module
void EmitVisitor::visit(ModuleDefn& node) {
    result_ = "module " + node.getName();

    // Parameters
    const auto& params = node.getParams();
    if (!params.empty()) {
        result_ += " #(\n";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) result_ += ",\n";
            result_ += "  " + visitParamWithoutSemicolon(params[i].get());
        }
        result_ += "\n)";
    }

    // Ports
    const auto& ports = node.getPorts();
    result_ += " (\n";
    for (size_t i = 0; i < ports.size(); ++i) {
        if (i > 0) result_ += ",\n";
        result_ += "  " + visitPort(ports[i].get());
    }
    result_ += "\n);\n\n";

    // Declarations
    const auto& decls = node.getDecls();
    for (const auto& decl : decls) {
        result_ += "  " + visitDecl(decl.get()) + "\n";
    }
    if (!decls.empty()) result_ += "\n";

    // Functions - still use emit() for now
    const auto& functions = node.getFunctions();
    for (const auto& func : functions) {
        result_ += "  " + func->emit() + "\n\n";
    }

    // Tasks - still use emit() for now
    const auto& tasks = node.getTasks();
    for (const auto& task : tasks) {
        result_ += "  " + task->emit() + "\n\n";
    }

    // Statements (always blocks, assigns, generate blocks, etc.)
    const auto& stmts = node.getStmts();
    for (const auto& stmt : stmts) {
        result_ += indentString(visitStmt(stmt.get()), "  ") + "\n\n";
    }

    result_ += "endmodule";
}

// EmitVisitor helper methods
std::string EmitVisitor::visitStmtBody(Stmt* stmt) {
    // For GenerateStmt, we need special handling to avoid the wrapper
    if (auto* gen = dynamic_cast<GenerateStmt*>(stmt)) {
        EmitVisitor v;
        v.indent_level_ = indent_level_;
        v.emit_generate_wrapper_ = false;  // Don't emit the wrapper for nested generates
        gen->accept(v);
        return v.result_;
    }
    return visitStmt(stmt);
}

std::string EmitVisitor::visitParamWithoutSemicolon(ParamDecl* param) {
    std::string result;
    switch (param->getKind()) {
        case ParamDecl::Kind::Parameter:  result = "parameter "; break;
        case ParamDecl::Kind::LocalParam: result = "localparam "; break;
    }

    if (param->getType()) {
        result += param->getType()->emit() + " ";
    }

    result += param->getName() + " = " + visitExpr(const_cast<Expr*>(param->getValue()));
    return result;
}

std::string EmitVisitor::visitPort(PortDecl* port) {
    std::string result;
    switch (port->getDirection()) {
        case PortDecl::Direction::Input:  result = "input "; break;
        case PortDecl::Direction::Output: result = "output "; break;
        case PortDecl::Direction::Inout:  result = "inout "; break;
    }
    result += port->getType()->emit() + " " + port->getName();
    return result;
}

std::string EmitVisitor::visitDecl(Decl* decl) {
    EmitVisitor v;
    v.indent_level_ = indent_level_;
    v.emit_generate_wrapper_ = emit_generate_wrapper_;
    decl->accept(v);
    return v.result_;
}

} // namespace ast
