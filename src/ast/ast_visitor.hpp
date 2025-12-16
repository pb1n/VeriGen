#pragma once

#include "ast_base.hpp"

namespace ast {

// Forward declarations of all AST node types
class LiteralExpr;
class VarExpr;
class BinaryExpr;
class UnaryExpr;
class ConditionalExpr;
class BitSelectExpr;
class RangeSelectExpr;
class ConcatExpr;

class AssignStmt;
class BlockStmt;
class IfStmt;
class CaseStmt;
class ForStmt;
class WhileStmt;
class AlwaysStmt;
class InitialStmt;
class ModuleInstStmt;
class GenerateStmt;

class VarDecl;
class PortDecl;
class ParamDecl;
class FunctionDecl;
class TaskDecl;
class GenvarDecl;

class IntType;
class ArrayType;
class PackedArrayType;

class ModuleDefn;

// Visitor interface using the visitor pattern
class Visitor {
public:
    virtual ~Visitor() = default;

    // Expression visitors
    virtual void visit(LiteralExpr& node) = 0;
    virtual void visit(VarExpr& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(ConditionalExpr& node) = 0;
    virtual void visit(BitSelectExpr& node) = 0;
    virtual void visit(RangeSelectExpr& node) = 0;
    virtual void visit(ConcatExpr& node) = 0;

    // Statement visitors
    virtual void visit(AssignStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(CaseStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(AlwaysStmt& node) = 0;
    virtual void visit(InitialStmt& node) = 0;
    virtual void visit(ModuleInstStmt& node) = 0;
    virtual void visit(GenerateStmt& node) = 0;

    // Declaration visitors
    virtual void visit(VarDecl& node) = 0;
    virtual void visit(PortDecl& node) = 0;
    virtual void visit(ParamDecl& node) = 0;
    virtual void visit(FunctionDecl& node) = 0;
    virtual void visit(TaskDecl& node) = 0;
    virtual void visit(GenvarDecl& node) = 0;

    // Type visitors
    virtual void visit(IntType& node) = 0;
    virtual void visit(ArrayType& node) = 0;
    virtual void visit(PackedArrayType& node) = 0;

    // Module visitors
    virtual void visit(ModuleDefn& node) = 0;
};

// Base visitor with default no-op implementations
class BaseVisitor : public Visitor {
public:
    // Expression visitors
    void visit(LiteralExpr& node) override { (void)node; }
    void visit(VarExpr& node) override { (void)node; }
    void visit(BinaryExpr& node) override { (void)node; }
    void visit(UnaryExpr& node) override { (void)node; }
    void visit(ConditionalExpr& node) override { (void)node; }
    void visit(BitSelectExpr& node) override { (void)node; }
    void visit(RangeSelectExpr& node) override { (void)node; }
    void visit(ConcatExpr& node) override { (void)node; }

    // Statement visitors
    void visit(AssignStmt& node) override { (void)node; }
    void visit(BlockStmt& node) override { (void)node; }
    void visit(IfStmt& node) override { (void)node; }
    void visit(CaseStmt& node) override { (void)node; }
    void visit(ForStmt& node) override { (void)node; }
    void visit(WhileStmt& node) override { (void)node; }
    void visit(AlwaysStmt& node) override { (void)node; }
    void visit(InitialStmt& node) override { (void)node; }
    void visit(ModuleInstStmt& node) override { (void)node; }
    void visit(GenerateStmt& node) override { (void)node; }

    // Declaration visitors
    void visit(VarDecl& node) override { (void)node; }
    void visit(PortDecl& node) override { (void)node; }
    void visit(ParamDecl& node) override { (void)node; }
    void visit(FunctionDecl& node) override { (void)node; }
    void visit(TaskDecl& node) override { (void)node; }
    void visit(GenvarDecl& node) override { (void)node; }

    // Type visitors
    void visit(IntType& node) override { (void)node; }
    void visit(ArrayType& node) override { (void)node; }
    void visit(PackedArrayType& node) override { (void)node; }

    // Module visitors
    void visit(ModuleDefn& node) override { (void)node; }
};

// Recursive visitor that traverses the entire AST
class RecursiveVisitor : public BaseVisitor {
public:
    // Expressions
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(ConditionalExpr& node) override;
    void visit(BitSelectExpr& node) override;
    void visit(RangeSelectExpr& node) override;
    void visit(ConcatExpr& node) override;

    // Statements
    void visit(AssignStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(CaseStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(AlwaysStmt& node) override;
    void visit(InitialStmt& node) override;
    void visit(GenerateStmt& node) override;

    // Module
    void visit(ModuleDefn& node) override;
};

// Visitor that emits Verilog code
class EmitVisitor : public BaseVisitor {
public:
    EmitVisitor() : indent_level_(0), emit_generate_wrapper_(true) {}

    std::string getResult() const { return result_; }

    // Expression visitors
    void visit(LiteralExpr& node) override;
    void visit(VarExpr& node) override;
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(ConditionalExpr& node) override;
    void visit(BitSelectExpr& node) override;
    void visit(RangeSelectExpr& node) override;
    void visit(ConcatExpr& node) override;

    // Statement visitors
    void visit(AssignStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(CaseStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(AlwaysStmt& node) override;
    void visit(InitialStmt& node) override;
    void visit(GenerateStmt& node) override;

    // Declaration visitors
    void visit(VarDecl& node) override;
    void visit(PortDecl& node) override;
    void visit(ParamDecl& node) override;
    void visit(GenvarDecl& node) override;

    // Module visitor
    void visit(ModuleDefn& node) override;

private:
    std::string result_;
    int indent_level_;
    bool emit_generate_wrapper_;  // Whether to emit generate/endgenerate wrapper

    void indent() { 
        for (int i = 0; i < indent_level_; ++i) {
            result_ += "  ";
        }
    }

    std::string visitExpr(Expr* expr) {
        EmitVisitor v;
        v.indent_level_ = indent_level_;
        v.emit_generate_wrapper_ = emit_generate_wrapper_;
        expr->accept(v);
        return v.result_;
    }

    std::string visitStmt(Stmt* stmt) {
        EmitVisitor v;
        v.indent_level_ = indent_level_;
        v.emit_generate_wrapper_ = emit_generate_wrapper_;
        stmt->accept(v);
        return v.result_;
    }

    // Helper to indent a multi-line string
    static std::string indentString(const std::string& str, const std::string& prefix) {
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

    // Helper methods (defined in .cpp since they need complete type information)
    std::string visitStmtBody(Stmt* stmt);
    std::string visitParamWithoutSemicolon(ParamDecl* param);
    std::string visitPort(PortDecl* port);
    std::string visitDecl(Decl* decl);
};

} // namespace ast
