#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include <filesystem>
#include <fstream>
#include <utility>
#include <iomanip>
#include <stdexcept>

namespace veri {

/* ==========================  Core AST  ============================== */

struct Expr {
    virtual ~Expr() = default;
    virtual std::string emit() const = 0;   // Verilog string
    virtual uint32_t    eval() const = 0;   // 32‑bit constant evaluation
};

// leaf: constant (optional wire alias)
struct Const final : Expr {
    uint32_t    value;
    std::string sym;        // if non‑empty -> treat as wire name in emit()

    Const(uint32_t v, std::string s = {}) : value(v), sym(std::move(s)) {}
    std::string emit()  const override {
        return sym.empty() ? ("32'd"+std::to_string(value)) : sym;
    }
    uint32_t    eval()  const override { return value; }
};

// leaf: reference to another net (no constant value)
struct WireRef final : Expr {
    std::string name;
    explicit WireRef(std::string n) : name(std::move(n)) {}
    std::string emit() const override { return name; }
    uint32_t    eval() const override {
        throw std::logic_error("WireRef::eval() not constant‑foldable");
    }
};

// binary operators
enum class BinOp { Add, Or, And, Xor, Sub };
inline const char* tok(BinOp op){
    switch(op){
        case BinOp::Add: return "+"; case BinOp::Sub: return "-";
        case BinOp::Or : return "|"; case BinOp::And: return "&";
        case BinOp::Xor: return "^";
    }
    return "?";
}

struct BinExpr final : Expr {
    BinOp op;
    std::vector<std::shared_ptr<Expr>> ops;
    BinExpr(BinOp o, std::vector<std::shared_ptr<Expr>> v)
        : op(o), ops(std::move(v)) {}
    std::string emit() const override {
        std::ostringstream os;
        os << '(';
        for(std::size_t i=0;i<ops.size();++i){
            if(i) os << ' ' << tok(op) << ' ';
            os << ops[i]->emit();
        }
        os << ')';
        return os.str();
    }
    uint32_t eval() const override {
        uint32_t acc = ops.front()->eval();
        for(std::size_t i=1;i<ops.size();++i){
            uint32_t rhs = ops[i]->eval();
            switch(op){
                case BinOp::Add: acc += rhs; break;
                case BinOp::Sub: acc -= rhs; break;
                case BinOp::Or : acc |= rhs; break;
                case BinOp::And: acc &= rhs; break;
                case BinOp::Xor: acc ^= rhs; break;
            }
        }
        return acc;
    }
};

// Module
struct Assign { std::string lhs; std::shared_ptr<Expr> rhs;
    std::string emit() const { return "assign " + lhs + " = " + rhs->emit() + ';'; } };

struct Module {
    std::string                          name;
    std::vector<Assign>                  assigns;
    std::vector<std::unique_ptr<Module>> children;
    std::vector<std::string>             childNets;  // wires to children
    bool                                 isTop = false;
};

// Hierarchy fuzz
class HierarchyGenerator {
public:
    explicit HierarchyGenerator(unsigned seed = std::random_device{}()) : rng(seed) {}
    std::unique_ptr<Module> make(const std::string& topName, int depth){
        return rec(topName, 0, depth);
    }
private:
    std::mt19937 rng;
    std::uniform_int_distribution<int> leafVal{0,15}, fanout{2,4}, opPick{0,4};

    static BinOp opFromIdx(int i){
        switch(i){ case 0: return BinOp::Add; case 1: return BinOp::Sub;
                   case 2: return BinOp::Xor; case 3: return BinOp::And;
                   default: return BinOp::Or; }
    }

    std::unique_ptr<Module> rec(const std::string& name,int d,int maxD){
        auto m = std::make_unique<Module>();
        m->name = name; m->isTop = (d==0);
        if(d==maxD){
            m->assigns.push_back({"out",
                                  std::make_shared<Const>(leafVal(rng))});
            return m;
        }
        int n = fanout(rng);
        std::vector<std::shared_ptr<Expr>> refs;
        for(int i=0;i<n;++i){
            auto child = rec(name+"_c"+std::to_string(i), d+1, maxD);
            std::string net = child->name + "_out";
            m->childNets.push_back(net);
            refs.push_back(std::make_shared<WireRef>(net));
            child->assigns.front().lhs = "out";
            m->children.push_back(std::move(child));
        }
        BinOp op = opFromIdx(opPick(rng));
        m->assigns.push_back({"out", std::make_shared<BinExpr>(op,refs)});
        return m;
    }
};

// Verilog emit
inline void emitModule(const Module& m, std::ostream& os, int ind = 0){
    std::string s(ind, ' ');
    os << s << "module " << m.name << " (\n";
    os << s << "    output [31:0] out\n" << s << ");\n";

    for(const auto& w : m.childNets) os << s << "wire [31:0] " << w << ";\n";

    for(const auto& cptr : m.children){
        const Module& c = *cptr;
        os << s << c.name << ' ' << c.name << "_inst ( .out(" << c.name << "_out) );\n";
    }
    for(const auto& a : m.assigns) os << s << a.emit() << "\n";
    os << s << "endmodule\n\n";

    for(const auto& cptr : m.children) emitModule(*cptr, os, ind);
}

inline std::string emitVerilog(const Module& top){
    std::ostringstream os;
    emitModule(top, os);
    return os.str();
}

/* ===================================================================
   'Legacy' generator wrappers (from old verigen_generate.hpp &
   verigen_generate_for.hpp).
   =================================================================== */
namespace legacy {

// operator support
inline BinOp binOpFromChar(char c){
    switch(c){
        case '+': return BinOp::Add;
        case '-': return BinOp::Sub;
        case '^': return BinOp::Xor;
        case '&': return BinOp::And;
        case '|': return BinOp::Or;
        default : throw std::invalid_argument("invalid op");
    }
}

/* ================================================================
   1.  VerilogGenerator      – random constants, generate‑block inst
   ================================================================ */
class VerilogGenerator {
    std::mt19937 rng;
    static inline const std::vector<char> OPS{'+','^'/*,'&','|','-',*/};
public:
    explicit VerilogGenerator(unsigned seed = std::random_device{}())
        : rng(seed) {}

    std::pair<std::filesystem::path,uint32_t>
    make(unsigned idx,int Nconst = 5)
    {
        // build constants
        std::vector<std::shared_ptr<Const>> C;
        std::uniform_int_distribution<uint32_t> dist(0,0xffffffffu);
        for(int i=0;i<Nconst;++i){
            uint32_t val = dist(rng);
            C.push_back(std::make_shared<Const>(val,"c"+std::to_string(i)));
        }

        // random expression tree
        std::uniform_int_distribution<int> opSel(0, (int)OPS.size() - 1);
        std::shared_ptr<Expr> expr = C[0];
        for(int i=1;i<Nconst;++i){
            char opCh = OPS[opSel(rng)];
            expr = std::make_shared<BinExpr>(binOpFromChar(opCh),
                                             std::vector<std::shared_ptr<Expr>>{expr, C[i]});
        }
        uint32_t expected = expr->eval();

        /* ---- write Verilog file -------------------------------- */
        std::filesystem::path fname = "fuzz_" + std::to_string(idx) + ".v";
        std::ofstream f(fname);
        if(!f) throw std::runtime_error("cannot open "+fname.string());

        f << "// auto‑generated by VerilogGenerator\n`timescale 1ns/1ps\n\n"
          << "module constant_block #(parameter VALUE=32'h0)(out);\n"
          << "  output [31:0] out;\n  assign out = VALUE;\nendmodule\n\n"
          << "module top(result);\n  output [31:0] result;\n";

        for(auto& c : C)
            f << "  wire [31:0] " << c->sym << ";\n";

        f << "  generate\n";
        for(auto& c : C){
            f << "    constant_block #(32'h" << std::hex << std::setw(8) << std::setfill('0')
              << c->value << std::dec << ") inst_" << c->sym
              << " (.out(" << c->sym << "));\n";
        }
        f << "  endgenerate\n"
          << "  assign result = " << expr->emit() << ";\nendmodule\n";

        return {fname, expected};
    }
};

/* ================================================================
   2.  VerilogGeneratorFor   – deterministic constants, for‑loop gen
   ================================================================ */
class VerilogGeneratorFor {
    std::mt19937 rng;
    static inline const std::vector<char> OPS{'+','^'/*,'&','|','-',*/};
    // deterministic formula – identical in C++ & Verilog
    static constexpr uint32_t K1 = 0x9E37'79B9;
    static constexpr uint32_t K2 = 0xBA55'ED5A;

    static constexpr uint32_t const_val(unsigned i,unsigned seed){
        return ((i+1)*K1) ^ (seed*K2);
    }
public:
    explicit VerilogGeneratorFor(unsigned seed = std::random_device{}())
        : rng(seed) {}

    std::pair<std::filesystem::path,uint32_t>
    make(unsigned idx,int Nconst = 5)
    {
        // deterministic constants
        std::vector<std::shared_ptr<Const>> C;
        for(int i=0;i<Nconst;++i){
            uint32_t val = const_val(i, idx);
            C.push_back(std::make_shared<Const>(val,"g["+std::to_string(i)+"]"));
        }

        // random expression over those constants
        std::uniform_int_distribution<int> opSel(0, (int)OPS.size() - 1);
        std::shared_ptr<Expr> expr = C[0];
        for(int i=1;i<Nconst;++i){
            char opCh = OPS[opSel(rng)];
            expr = std::make_shared<BinExpr>(binOpFromChar(opCh),
                                             std::vector<std::shared_ptr<Expr>>{expr, C[i]});
        }
        uint32_t expected = expr->eval();

        // write Verilog
        std::filesystem::path fname = "fuzz_for_" + std::to_string(idx) + ".v";
        std::ofstream f(fname);
        if(!f) throw std::runtime_error("cannot open "+fname.string());

        f << "// auto‑generated by VerilogGeneratorFor\n`timescale 1ns/1ps\n\n"
          << "module const_block #(parameter VALUE=32'h0)(output [31:0] w);\n"
          << "  assign w = VALUE;\nendmodule\n\n"
          << "module top(result);\n  output [31:0] result;\n"
          << "  wire [31:0] g[" << Nconst << "];\n"
          << "  genvar gi;\n  generate\n"
          << "    for(gi=0; gi<" << Nconst << "; gi=gi+1) begin : g_blk\n"
          << "      const_block #( ((gi + 1) * 32'h" << std::hex << K1 << std::dec
          << ") ^ (32'd" << idx << " * 32'h" << std::hex << K2 << std::dec
          << ") ) inst ( .w(g[gi]) );\n"
          << "    end\n"
          << "  endgenerate\n\n"
          << "  assign result = " << expr->emit() << ";\nendmodule\n";

        //std::cout << "EXPECTED: " << expected << "\n";
        return {fname, expected};
    }
};

} // namespace legacy

} // namespace veri