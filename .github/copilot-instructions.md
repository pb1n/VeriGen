# Copilot Instructions for reVeriGen

- **Project scope (current snapshot)**: Minimal C++ skeleton for expression handling in [src/ast.hpp](../src/ast.hpp). Defines abstract `Expr` with `emit()` (string emission) and `eval(values)` (32-bit evaluation).
- **Core contract**: Every expression node must implement both `emit()` and `eval(const std::vector<uint32_t>&)`. Keep implementations deterministic and side-effect free; `eval` should not mutate inputs.
- **Data types**: Use `uint32_t` for evaluation results and operands; stay consistent with the existing interface to avoid implicit sign/width issues.
- **String emission**: `emit()` likely returns a serialized representation (e.g., source-like or IR-like); ensure emitted forms stay reversible/parsable if new nodes are added.
- **Evaluation semantics**: When adding nodes, define clear behavior for overflow and division-by-zero; prefer explicit handling over relying on UB.
- **Memory model**: Base class has virtual destructor; use `std::unique_ptr` for ownership of `Expr` implementations to avoid leaks when extending the AST.
- **Header hygiene**: Keep `#pragma once` and lightweight includes; prefer forward declarations where possible to minimize compile times.
- **Extending the AST**: Add new node structs/classes deriving from `Expr`; override with `override` keyword; keep interfaces in headers and heavy logic in `.cpp` files to reduce recompiles.
- **Const-correctness**: Preserve `const` on `emit()`/`eval`; avoid caching mutable state unless guarded.
- **Error handling**: If evaluation can fail, surface it explicitly (e.g., optional/expected) rather than hidden behavior; document any non-standard semantics in comments near implementations.
- **Testing guidance**: Add focused unit tests around `eval()` edge cases (overflow, zero operands) and `emit()` formatting; keep golden outputs small and deterministic.
- **Build assumptions**: C++17 or later is typical; check with project owner for exact toolchain/flags before adding code. Prefer standard library over third-party deps unless justified.
- **Style**: Favor simple structs/classes with minimal inheritance. Keep includes ordered: std headers first, then project headers.
- **Adding files**: Place new AST nodes under `src/` with matching `.hpp/.cpp` pairs. Keep public interfaces small; hide helpers in unnamed namespaces inside `.cpp` files.
- **Performance considerations**: Avoid unnecessary copies of `std::vector<uint32_t>`; pass by const reference as in the interface. Consider `constexpr` for trivial nodes when possible.
- **Documentation**: When introducing new semantics, add brief comments near the implementation to clarify any non-obvious rules.
- **Open questions**: No build/test tooling is present in the repo snapshot. Ask the maintainer for the canonical build command and CI expectations before adding dependencies or changing interfaces.
