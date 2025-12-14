# reVeriGen

A C++ library for representing and generating Verilog/SystemVerilog code through an Abstract Syntax Tree (AST).

## Overview

reVeriGen provides a comprehensive AST framework for programmatically constructing Verilog modules and expressions. It supports code emission (generating Verilog text) and expression evaluation for verification and testing.

## Features

- **Complete AST Representation**: Expressions, statements, declarations, types, and modules
- **Code Generation**: Emit valid Verilog/SystemVerilog code from AST nodes
- **Expression Evaluation**: Evaluate constant expressions with 32-bit arithmetic
- **Visitor Pattern**: Traverse and transform AST structures
- **Type-Safe**: Leverages C++17 features for memory safety with `std::unique_ptr`
- **Deterministic**: All operations are side-effect free and reproducible

## Project Structure

```
reVeriGen/
├── src/
│   └── ast/
│       ├── ast_base.hpp       # Base classes for all AST nodes
│       ├── ast_types.hpp      # Type system (wire, reg, logic, arrays)
│       ├── ast_expr.hpp       # Expression nodes (binary, unary, literals, etc.)
│       ├── ast_stmt.hpp       # Statement nodes (assign, always, if, case, etc.)
│       ├── ast_decl.hpp       # Declaration nodes (variables, ports, parameters)
│       ├── ast_module.hpp     # Module definitions and instantiations
│       ├── ast_visitor.hpp    # Visitor pattern interface
│       ├── ast_visitor.cpp    # Visitor implementation
│       └── example.cpp        # Usage examples
├── tests/                     # Unit tests (TBD)
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

## Building

### Requirements

- C++17 or later compiler
- CMake 3.15 or later

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Run example
./ast_example
```

## Usage Examples

### Creating a Simple Counter Module

```cpp
#include "ast_base.hpp"
#include "ast_types.hpp"
#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_decl.hpp"
#include "ast_module.hpp"

using namespace ast;

// Create ports
std::vector<Ptr<PortDecl>> ports;
ports.push_back(make<PortDecl>(
    PortDecl::Direction::Input,
    "clk",
    makeWire(1)
));
ports.push_back(make<PortDecl>(
    PortDecl::Direction::Output,
    "count",
    makeReg(8)
));

// Create module
auto module = make<ModuleDefn>("counter", std::move(ports));

// Create always block
auto always_block = make<AlwaysStmt>(
    AlwaysStmt::Kind::Always,
    std::vector<AlwaysStmt::SensitivityItem>{
        {AlwaysStmt::SensitivityItem::Edge::Pos, "clk"}
    },
    make<AssignStmt>(
        AssignStmt::Kind::NonBlocking,
        make<VarExpr>("count"),
        make<BinaryExpr>(
            BinaryExpr::Op::Add,
            make<VarExpr>("count"),
            make<LiteralExpr>(1, 8)
        )
    )
);

module->addStmt(std::move(always_block));

// Emit Verilog code
std::cout << module->emit() << std::endl;
```

### Expression Evaluation

```cpp
// Create expression: (5 + 3) * 2
auto expr = make<BinaryExpr>(
    BinaryExpr::Op::Mul,
    make<BinaryExpr>(
        BinaryExpr::Op::Add,
        make<LiteralExpr>(5),
        make<LiteralExpr>(3)
    ),
    make<LiteralExpr>(2)
);

// Evaluate
auto result = expr->eval({});
if (result) {
    std::cout << "Result: " << *result << std::endl; // Output: 16
}
```

### Using Variables in Expressions

```cpp
// Create expression: a & b | c
auto expr = make<BinaryExpr>(
    BinaryExpr::Op::Or,
    make<BinaryExpr>(
        BinaryExpr::Op::And,
        make<VarExpr>("a", 0),  // Index 0 in value vector
        make<VarExpr>("b", 1)   // Index 1 in value vector
    ),
    make<VarExpr>("c", 2)       // Index 2 in value vector
);

// Evaluate with specific values
std::vector<uint32_t> values = {0xF0, 0x0F, 0x55};
auto result = expr->eval(values);
```

## AST Node Types

### Expressions (`ast_expr.hpp`)
- `LiteralExpr` - Constant values
- `VarExpr` - Variable references
- `BinaryExpr` - Binary operations (+, -, *, /, &, |, ^, ==, !=, etc.)
- `UnaryExpr` - Unary operations (-, ~, !, reduction operators)
- `ConditionalExpr` - Ternary operator (cond ? true : false)
- `BitSelectExpr` - Single bit selection (signal[index])
- `RangeSelectExpr` - Range selection (signal[msb:lsb])
- `ConcatExpr` - Concatenation ({a, b, c})

### Statements (`ast_stmt.hpp`)
- `AssignStmt` - Blocking (=), non-blocking (<=), continuous (assign)
- `BlockStmt` - Begin/end blocks
- `IfStmt` - Conditional statements
- `CaseStmt` - Case statements (case, casex, casez)
- `ForStmt` - For loops
- `WhileStmt` - While loops
- `AlwaysStmt` - Always blocks (always, always_comb, always_ff, always_latch)
- `InitialStmt` - Initial blocks

### Declarations (`ast_decl.hpp`)
- `VarDecl` - Variable declarations (wire, reg, logic)
- `PortDecl` - Port declarations (input, output, inout)
- `ParamDecl` - Parameters and localparams
- `FunctionDecl` - Function definitions
- `TaskDecl` - Task definitions
- `GenvarDecl` - Generate variables

### Types (`ast_types.hpp`)
- `IntType` - Integer types (wire, reg, logic) with width and signedness
- `ArrayType` - Unpacked arrays
- `PackedArrayType` - Packed arrays

### Modules (`ast_module.hpp`)
- `ModuleDefn` - Module definitions
- `ModuleInstStmt` - Module instantiations
- `GenerateStmt` - Generate blocks

## Core Contracts

### Expression Evaluation
- All expressions implement `eval(const std::vector<uint32_t>& values)`
- Returns `std::optional<uint32_t>` (nullopt on error, e.g., division by zero)
- Evaluation is deterministic and side-effect free
- Variables are indexed into the values vector by their index field

### Code Emission
- All nodes implement `emit()` returning a string representation
- Emitted code follows Verilog/SystemVerilog syntax
- Formatting includes proper indentation and newlines

### Memory Management
- All AST nodes use `std::unique_ptr` for ownership
- Use `ast::make<T>(args...)` helper for creating nodes
- Virtual destructors ensure proper cleanup

## Design Principles

1. **Const-Correctness**: `emit()` and `eval()` are const methods
2. **Error Handling**: Evaluation failures return `std::nullopt` rather than throwing
3. **No Implicit Conversions**: Explicit about bit widths and signedness
4. **Minimal Dependencies**: Uses only C++17 standard library
5. **Header Hygiene**: `#pragma once` and minimal includes

## Future Work

- [ ] Unit test framework
- [ ] More comprehensive error messages
- [ ] Support for SystemVerilog interfaces and classes
- [ ] Advanced type checking
- [ ] Parser to build AST from Verilog source
- [ ] Optimization passes

## Contributing

This is a university project. Contributions should follow the guidelines in [.github/copilot-instructions.md](.github/copilot-instructions.md).

## License

TBD
