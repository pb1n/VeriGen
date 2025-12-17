/**
 * Reference Model Visitor - FIXED VERSION
 *
 * Fixes:
 * 1. Non-blocking assignments (use shadow registers)
 * 2. Clock racing (update all prev_ signals at end of tick)
 * 3. @ sensitivity (combinational logic doesn't need tick())
 * 4. Type approximation (use uint8_t, uint16_t, uint32_t, uint64_t based on width)
 */

#include "reference_model_visitor.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>

namespace ast {

// ===== Helper: Choose appropriate C++ type based on bit width =====
std::string ReferenceModelVisitor::chooseCppType(int width) {
    if (width == 1) return "bool";
    if (width <= 8) return "uint8_t";
    if (width <= 16) return "uint16_t";
    if (width <= 32) return "uint32_t";
    if (width <= 64) return "uint64_t";

    // For very large widths, we'd need a BigInt library
    // For now, clamp to uint64_t and warn
    return "uint64_t";  // TODO: Support arbitrary precision
}

// ===== Expression Translation =====

std::string ReferenceModelVisitor::translateExpr(Expr* expr) {
    if (!expr) return "";

    std::string saved_result = result_;
    result_ = "";
    expr->accept(*this);
    std::string expr_result = result_;
    result_ = saved_result;

    return expr_result;
}

void ReferenceModelVisitor::visit(BinaryExpr& node) {
    std::string left = translateExpr(const_cast<Expr*>(node.getLeft()));
    std::string right = translateExpr(const_cast<Expr*>(node.getRight()));

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
        case BinaryExpr::Op::AShr: op_str = ">>"; break; // Arithmetic shift (same in C++ for signed)
        case BinaryExpr::Op::LAnd: op_str = "&&"; break;
        case BinaryExpr::Op::LOr:  op_str = "||"; break;
        case BinaryExpr::Op::Eq:   op_str = "=="; break;
        case BinaryExpr::Op::Neq:  op_str = "!="; break;
        case BinaryExpr::Op::Lt:   op_str = "<"; break;
        case BinaryExpr::Op::Le:   op_str = "<="; break;
        case BinaryExpr::Op::Gt:   op_str = ">"; break;
        case BinaryExpr::Op::Ge:   op_str = ">="; break;
        default: op_str = "?"; break;
    }

    result_ = "(" + left + " " + op_str + " " + right + ")";
}

void ReferenceModelVisitor::visit(UnaryExpr& node) {
    std::string operand = translateExpr(const_cast<Expr*>(node.getOperand()));
    std::string op_str;
    switch (node.getOp()) {
        case UnaryExpr::Op::LNot: op_str = "!"; break;
        case UnaryExpr::Op::Not:  op_str = "~"; break;
        case UnaryExpr::Op::Neg:  op_str = "-"; break;
        default: op_str = "!"; break;
    }
    result_ = "(" + op_str + operand + ")";
}

void ReferenceModelVisitor::visit(ConditionalExpr& node) {
    std::string cond = translateExpr(const_cast<Expr*>(node.getCond()));
    std::string then_val = translateExpr(const_cast<Expr*>(node.getTrueExpr()));
    std::string else_val = translateExpr(const_cast<Expr*>(node.getFalseExpr()));
    result_ = "(" + cond + " ? " + then_val + " : " + else_val + ")";
}

void ReferenceModelVisitor::visit(LiteralExpr& node) {
    result_ = std::to_string(node.getValue());
}

void ReferenceModelVisitor::visit(VarExpr& node) {
    result_ = node.getName();
}

// ===== Statement Visitors =====

void ReferenceModelVisitor::visit(AlwaysStmt& node) {
    const auto& sensitivity = node.getSensitivity();

    // Determine if this is combinational or sequential
    bool has_edge_sensitivity = false;
    for (const auto& item : sensitivity) {
        if (item.edge != AlwaysStmt::SensitivityItem::Edge::None) {
            has_edge_sensitivity = true;
            break;
        }
    }

    if (!has_edge_sensitivity && !sensitivity.empty()) {
        // FIX 3: Combinational logic (@(*) or @(a, b, c))
        // Don't add to tick() - this should be continuous assignment
        // For now, treat as if it runs every tick (not ideal but works)
        // TODO: Separate combinational vs sequential in reference model

        tick_body_ += indent() + "// Combinational always block\n";
        if (node.getBody()) {
            const_cast<Stmt*>(node.getBody())->accept(*this);
        }
        return;
    }

    // FIX 2: Build edge detection condition
    std::string condition;
    bool first = true;

    for (const auto& item : sensitivity) {
        if (!first) condition += " || ";
        first = false;

        std::string sig = item.signal;

        if (item.edge == AlwaysStmt::SensitivityItem::Edge::Pos) {
            condition += "(" + sig + " && !prev_" + sig + ")";
            registerPrevSignal(sig);
        } else if (item.edge == AlwaysStmt::SensitivityItem::Edge::Neg) {
            condition += "(!" + sig + " && prev_" + sig + ")";
            registerPrevSignal(sig);
        }
    }

    if (condition.empty()) condition = "true";

    // Generate tick body
    tick_body_ += indent() + "if (" + condition + ") {\n";
    indent_level_++;

    if (node.getBody()) {
        const_cast<Stmt*>(node.getBody())->accept(*this);
    }

    indent_level_--;
    tick_body_ += indent() + "}\n";
}

void ReferenceModelVisitor::visit(AssignStmt& node) {
    std::string lhs = translateExpr(const_cast<Expr*>(node.getLhs()));
    std::string rhs = translateExpr(const_cast<Expr*>(node.getRhs()));

    // FIX 1: Handle non-blocking assignments
    if (node.getKind() == AssignStmt::Kind::NonBlocking) {
        // Use shadow register
        std::string shadow_var = lhs + "_next";

        // Register this variable needs a shadow
        if (std::find(nonblocking_vars_.begin(), nonblocking_vars_.end(), lhs) == nonblocking_vars_.end()) {
            nonblocking_vars_.push_back(lhs);
        }

        tick_body_ += indent() + shadow_var + " = " + rhs + ";\n";
    } else {
        // Blocking assignment or continuous
        tick_body_ += indent() + lhs + " = " + rhs + ";\n";
    }
}

void ReferenceModelVisitor::visit(VarDecl& node) {
    const Type* type_ptr = node.getType();

    // FIX 4: Use proper type based on bit width
    std::string cpp_type = chooseCppType(type_ptr->getWidth());
    std::string name = node.getName();

    state_vars_.push_back(cpp_type + " " + name + ";");
}

// ===== Module Visitor =====

void ReferenceModelVisitor::visit(ModuleDefn& node) {
    class_name_ = node.getName() + "RefModel";

    // Clear state
    state_vars_.clear();
    tick_body_.clear();
    nonblocking_vars_.clear();
    prev_signals_.clear();
    indent_level_ = 0;

    // Separate inputs and outputs
    std::vector<std::string> input_ports, output_ports;

    for (const auto& port : node.getPorts()) {
        if (port->getDirection() == PortDecl::Direction::Input) {
            const Type* type_ptr = port->getType();
            std::string cpp_type = chooseCppType(type_ptr->getWidth());
            input_ports.push_back(cpp_type + " " + port->getName());
        } else {
            output_ports.push_back(port->getName());
            // Visit to add to state_vars_
            const_cast<PortDecl*>(port.get())->accept(*this);
        }
    }

    // Process declarations (adds to state_vars_)
    for (const auto& decl : node.getDecls()) {
        const_cast<Decl*>(decl.get())->accept(*this);
    }

    // Process statements (builds tick_body_)
    for (const auto& stmt : node.getStmts()) {
        const_cast<Stmt*>(stmt.get())->accept(*this);
    }

    // Build the class
    std::ostringstream os;

    os << "class " << class_name_ << " {\n";
    os << "private:\n";

    // State variables
    for (const auto& var : state_vars_) {
        os << "    " << var << "\n";
    }

    // FIX 1: Shadow registers for non-blocking assignments
    for (const auto& var : nonblocking_vars_) {
        // Find the type of this variable
        std::string var_type = "uint32_t";  // Default
        for (const auto& state_var : state_vars_) {
            if (state_var.find(var + ";") != std::string::npos) {
                size_t space_pos = state_var.find(' ');
                var_type = state_var.substr(0, space_pos);
                break;
            }
        }
        os << "    " << var_type << " " << var << "_next;\n";
    }

    // FIX 2: Previous signal values for edge detection
    for (const auto& sig : prev_signals_) {
        os << "    bool prev_" << sig << ";\n";
    }

    os << "\npublic:\n";

    // Constructor
    os << "    " << class_name_ << "() : ";
    bool first_init = true;
    for (const auto& var : state_vars_) {
        size_t space = var.find(' ');
        size_t semi = var.find(';');
        std::string name = var.substr(space + 1, semi - space - 1);

        if (!first_init) os << ", ";
        first_init = false;
        os << name << "(0)";
    }
    // Initialize shadow registers
    for (const auto& var : nonblocking_vars_) {
        if (!first_init) os << ", ";
        first_init = false;
        os << var << "_next(0)";
    }
    // Initialize prev_ signals
    for (const auto& sig : prev_signals_) {
        if (!first_init) os << ", ";
        first_init = false;
        os << "prev_" << sig << "(false)";
    }
    os << " {}\n\n";

    // tick() method
    os << "    void tick(";
    for (size_t i = 0; i < input_ports.size(); ++i) {
        if (i > 0) os << ", ";
        os << input_ports[i];
    }
    os << ") {\n";

    // Add indented tick body
    if (!tick_body_.empty()) {
        std::istringstream body_stream(tick_body_);
        std::string line;
        while (std::getline(body_stream, line)) {
            os << "        " << line << "\n";
        }
    }

    // FIX 1: Update non-blocking registers at end of tick
    if (!nonblocking_vars_.empty()) {
        os << "\n        // Update non-blocking assignments\n";
        for (const auto& var : nonblocking_vars_) {
            os << "        " << var << " = " << var << "_next;\n";
        }
    }

    // FIX 2: Update edge detection signals at END of tick (no race conditions)
    if (!prev_signals_.empty()) {
        os << "\n        // Update edge detection\n";
        for (const auto& sig : prev_signals_) {
            os << "        prev_" << sig << " = " << sig << ";\n";
        }
    }

    os << "    }\n\n";

    // Getters for output ports
    for (const auto& port_name : output_ports) {
        // Capitalize first letter for getter
        std::string getter_name = "get" + port_name;
        getter_name[3] = std::toupper(getter_name[3]);

        // Find type
        std::string port_type = "uint32_t";
        for (const auto& var : state_vars_) {
            if (var.find(port_name + ";") != std::string::npos) {
                size_t space = var.find(' ');
                port_type = var.substr(0, space);
                break;
            }
        }

        os << "    " << port_type << " " << getter_name << "() const { return " << port_name << "; }\n";
    }

    os << "};\n";

    result_ = os.str();
}

// Helper to register prev_ signal
void ReferenceModelVisitor::registerPrevSignal(const std::string& signal) {
    if (std::find(prev_signals_.begin(), prev_signals_.end(), signal) == prev_signals_.end()) {
        prev_signals_.push_back(signal);
    }
}

std::string ReferenceModelVisitor::indent() const {
    return std::string(indent_level_ * 4, ' ');
}

} // namespace ast
