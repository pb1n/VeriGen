#include "reference_model_visitor.hpp"
#include <cctype>
#include <sstream>

namespace ast {

// ===== Expression Translation Helpers =====

std::string ReferenceModelVisitor::translateExpr(Expr* expr) {
    if (!expr) return "";
    
    // Save current result to support recursive calls
    std::string saved_result = result_;
    result_ = "";
    
    // Dispatch visitor
    expr->accept(*this);
    
    // Capture result and restore state
    std::string expr_result = result_;
    result_ = saved_result;
    
    return expr_result;
}

std::string ReferenceModelVisitor::translateType(const std::string& verilog_type) {
    // TODO: DEPRECATED - Remove this method once all callers use getWidth() directly
    // This is a legacy helper that parses emitted Verilog type strings
    // If it has brackets [N:0], it's a multi-bit integer (uint32_t)
    // Otherwise it's a single bit (bool)
    if (verilog_type.find('[') != std::string::npos) {
        return "uint32_t";
    }
    return "bool";
}

// ===== Expression Visitors =====

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
        // Bitwise
        case BinaryExpr::Op::And:  op_str = "&"; break;
        case BinaryExpr::Op::Or:   op_str = "|"; break;
        case BinaryExpr::Op::Xor:  op_str = "^"; break;
        case BinaryExpr::Op::Shl:  op_str = "<<"; break;
        case BinaryExpr::Op::Shr:  op_str = ">>"; break; // what about if signed? what happens in C++?
        case BinaryExpr::Op::AShr: op_str = ">>"; break; // >> is signed in C++
        // Logical
        case BinaryExpr::Op::LAnd: op_str = "&&"; break;
        case BinaryExpr::Op::LOr:  op_str = "||"; break;
        // Comparison
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
        case UnaryExpr::Op::LNot:   op_str = "!"; break;
        case UnaryExpr::Op::Not:    op_str = "~"; break;
        case UnaryExpr::Op::Neg:    op_str = "-"; break;
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
// 1. Extract sensitivity list and build condition
    std::string condition;
    std::vector<std::string> prev_updates;
    
    const auto& sensitivity = node.getSensitivity();
    bool first = true;
    
    for (const auto& item : sensitivity) {
        if (!first) condition += " || ";
        first = false;
        
        std::string sig = item.signal;
        
        if (item.edge == AlwaysStmt::SensitivityItem::Edge::Pos) {
            // posedge clk -> (clk && !prev_clk)
            condition += "(" + sig + " && !prev_" + sig + ")";
            
            // Track that we need a prev_ variable
            // Check if already added to avoid duplicates
            std::string decl = "bool prev_" + sig + ";";
            bool exists = false;
            for(const auto& v : state_vars_) if(v == decl) exists = true;
            if(!exists) state_vars_.push_back(decl);
            
            prev_updates.push_back("prev_" + sig + " = " + sig + ";");
            
        } else if (item.edge == AlwaysStmt::SensitivityItem::Edge::Neg) {
            // negedge rst -> (!rst && prev_rst)
            condition += "(!" + sig + " && prev_" + sig + ")";
            
            std::string decl = "bool prev_" + sig + ";";
            bool exists = false;
            for(const auto& v : state_vars_) if(v == decl) exists = true;
            if(!exists) state_vars_.push_back(decl);
            
            prev_updates.push_back("prev_" + sig + " = " + sig + ";");
        } else {
            // Level sensitive (always @(*)) or just signal name
            // For reference model tick(), we usually just execute
            condition += "true"; 
        }
    }
    
    if (condition.empty()) condition = "true";

    // 2. Generate tick body
    tick_body_ += indent() + "if (" + condition + ") {\n";
    indent_level_++;
    
    // Translate body - calls visit(AssignStmt) or others via accept
    if (node.getBody()) {
        const_cast<Stmt*>(node.getBody())->accept(*this);
    }
    
    indent_level_--;
    tick_body_ += indent() + "}\n";
    
    // 3. Add edge updates
    // Note: In a robust simulation, these should happen at the very end of tick()
    // to avoid race conditions between multiple always blocks. 
    // For this skeleton, we append them here.
    for (const auto& update : prev_updates) {
        tick_body_ += indent() + update + "\n";
    }
}

void ReferenceModelVisitor::visit(AssignStmt& node) {
    std::string lhs = translateExpr(const_cast<Expr*>(node.getLhs()));
    std::string rhs = translateExpr(const_cast<Expr*>(node.getRhs()));

    tick_body_ += indent() + lhs + " = " + rhs + ";\n";
}

void ReferenceModelVisitor::visit(VarDecl& node) {
    // Extract type information directly from Type interface
    const Type* type_ptr = node.getType();

    // Determine C++ type based on width
    // TODO: Once emit() is removed from AST, this is the correct approach
    std::string cpp_type = (type_ptr->getWidth() > 1) ? "uint32_t" : "bool";
    std::string name = node.getName();

    // Logic: Treat both Reg and Wire as class members.
    // This allows wires to be used easily in the tick() logic without scope issues.
    state_vars_.push_back(cpp_type + " " + name + ";");
}

// ===== Module Visitor (Main Entry Point) =====

void ReferenceModelVisitor::visit(ModuleDefn& node) {
    // Extract module name and initialise
    class_name_ = node.getName() + "RefModel";

    // Clear any previous state
    state_vars_.clear();
    input_vars_.clear();
    tick_body_.clear();
    result_.clear();

    // Visit all ports to separate inputs and outputs
    std::vector<const PortDecl*> input_ports;
    std::vector<const PortDecl*> output_ports;

    for (const auto& port : node.getPorts()) {
        if (port->getDirection() == PortDecl::Direction::Input) {
            input_ports.push_back(port.get());
        } else if (port->getDirection() == PortDecl::Direction::Output) {
            output_ports.push_back(port.get());
        }
    }

    // Visit all declarations to track state variables
    for (const auto& decl : node.getDecls()) {
        const_cast<Decl*>(decl.get())->accept(*this);
    }
        // Step 4: Visit all statements (always blocks, assigns) to build tick_body_
    for (const auto& stmt : node.getStmts()) {
        const_cast<Stmt*>(stmt.get())->accept(*this);
    }

    // Generate complete C++ class structure

    // Class declaration
    result_ += "class " + class_name_ + " {\n";
    result_ += "private:\n";

    // Private member variables (state variables from declarations)
    for (const auto& var : state_vars_) {
        result_ += "    " + var + "\n";
    }

    // Add output port variables as state (if not already added)
    for (const auto* port : output_ports) {
        std::string cpp_type = (port->getType()->getWidth() > 1) ? "uint32_t" : "bool";
        std::string var_decl = cpp_type + " " + port->getName() + ";";

        // Check if not already in state_vars_
        bool already_exists = false;
        for (const auto& var : state_vars_) {
            if (var.find(port->getName() + ";") != std::string::npos) {
                already_exists = true;
                break;
            }
        }

        if (!already_exists) {
            result_ += "    " + var_decl + "\n";
        }
    }

    result_ += "\n";
    result_ += "public:\n";

    // Constructor - initialize all state to 0
    result_ += "    " + class_name_ + "() : ";

    // Initialize state variables
    std::vector<std::string> inits;

    // Add state vars initialization
    for (const auto& var : state_vars_) {
        // Extract variable name from declaration (e.g., "uint32_t count;" -> "count")
        size_t space_pos = var.find_last_of(' ');
        size_t semi_pos = var.find(';');
        if (space_pos != std::string::npos && semi_pos != std::string::npos) {
            std::string var_name = var.substr(space_pos + 1, semi_pos - space_pos - 1);
            inits.push_back(var_name + "(0)");
        }
    }

    // Add output port initialization
    for (const auto* port : output_ports) {
        std::string var_name = port->getName();

        // Check if not already initialized
        bool already_init = false;
        for (const auto& init : inits) {
            if (init.find(var_name + "(") == 0) {
                already_init = true;
                break;
            }
        }

        if (!already_init) {
            inits.push_back(var_name + "(0)");
        }
    }

    // Write initializer list
    for (size_t i = 0; i < inits.size(); ++i) {
        result_ += inits[i];
        if (i < inits.size() - 1) {
            result_ += ", ";
        }
    }

    result_ += " {}\n\n";

    // tick() method with input ports as parameters
    result_ += "    void tick(";

    // Add input ports as parameters
    for (size_t i = 0; i < input_ports.size(); ++i) {
        const auto* port = input_ports[i];
        std::string cpp_type = (port->getType()->getWidth() > 1) ? "uint32_t" : "bool";
        result_ += cpp_type + " " + port->getName();

        if (i < input_ports.size() - 1) {
            result_ += ", ";
        }
    }

    result_ += ") {\n";

    // tick() body (generated from always blocks)
    if (!tick_body_.empty()) {
        // Add proper indentation to tick_body_ (indent each line by 8 spaces = 2 levels)
        std::istringstream body_stream(tick_body_);
        std::string line;
        while (std::getline(body_stream, line)) {
            result_ += "        " + line + "\n";  // Add 8 spaces (2 indent levels)
        }
    }

    result_ += "    }\n\n";

    // Getter methods for each output port
    for (const auto* port : output_ports) {
        std::string cpp_type = (port->getType()->getWidth() > 1) ? "uint32_t" : "bool";
        std::string getter_name = "get" + port->getName();

        // Capitalize first letter
        if (!port->getName().empty()) {
            getter_name = "get";
            getter_name += static_cast<char>(std::toupper(port->getName()[0]));
            getter_name += port->getName().substr(1);
        }

        result_ += "    " + cpp_type + " " + getter_name + "() const { return " + port->getName() + "; }\n";
    }

    result_ += "};\n";
}

} // namespace ast
