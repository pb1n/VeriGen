#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace ast {

// Forward declarations
class Visitor;

// Base class for all AST nodes
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Accept visitor for traversal
    virtual void accept(Visitor& visitor) = 0;

    // TODO: REMOVE emit() - This violates separation of concerns!
    // Emit to string representation (Verilog code)
    // This should be handled by EmitVisitor, not baked into AST nodes.
    // Keep for now during transition, remove in future refactor.
    virtual std::string emit() const = 0;
};

// Base class for all expressions
class Expr : public ASTNode {
public:
    virtual ~Expr() = default;

    // Evaluate expression with given variable values
    // Returns nullopt if evaluation fails (e.g., division by zero)
    virtual std::optional<uint32_t> eval(const std::vector<uint32_t>& values) const = 0;

    void accept(Visitor& visitor) override;
};

// Base class for all statements
class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;

    void accept(Visitor& visitor) override;
};

// Base class for all declarations
class Decl : public ASTNode {
public:
    virtual ~Decl() = default;

    void accept(Visitor& visitor) override;

    // Get the name of the declared entity
    virtual std::string getName() const = 0;
};

// Base class for all type representations
class Type : public ASTNode {
public:
    virtual ~Type() = default;

    void accept(Visitor& visitor) override;

    // Get bit width of the type
    virtual uint32_t getWidth() const = 0;

    // Check if type is signed
    virtual bool isSigned() const = 0;
};

// Base class for module-level constructs
class Module : public ASTNode {
public:
    virtual ~Module() = default;

    void accept(Visitor& visitor) override;

    // Get module name
    virtual std::string getName() const = 0;
};

// Helper type aliases for ownership
template<typename T>
using Ptr = std::unique_ptr<T>;

template<typename T, typename... Args>
Ptr<T> make(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace ast
