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

} // namespace ast
