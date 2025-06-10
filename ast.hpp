#pragma once
/*───────────────────────────────────────────────────────────────*
 *  Compact hierarchical-test-source generator for Verilog       *
 *  – outer generate/endgenerate wrapper                         *
 *  – ADD / XOR only                                             *
 *───────────────────────────────────────────────────────────────*/

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
#include <functional>

namespace veri
{

    /*───────────────────────────────────────────────────────────────*/
    /* 1.  Expression-level AST                                      */
    /*───────────────────────────────────────────────────────────────*/
    struct Expr
    {
        virtual ~Expr() = default;
        virtual std::string emit() const = 0;
        virtual uint32_t eval() const = 0;
    };

    /* constant literal or aliased symbol */
    struct Const final : Expr
    {
        uint32_t value;
        std::string sym;
        Const(uint32_t v, std::string s = {}) : value(v), sym(std::move(s)) {}
        std::string emit() const override { return sym.empty() ? "32'd" + std::to_string(value) : sym; }
        uint32_t eval() const override { return value; }
    };

    /* reference to an existing net */
    struct WireRef final : Expr
    {
        std::string name;
        explicit WireRef(std::string n) : name(std::move(n)) {}
        std::string emit() const override { return name; }
        uint32_t eval() const override { throw std::logic_error("WireRef not const-foldable"); }
    };

    /* binary expression ─ (op restricted later) */
    enum class BinOp
    {
        Add,
        Sub,
        And,
        Or,
        Xor
    };

    inline const char *tok(BinOp o)
    {
        switch (o)
        {
        case BinOp::Add:
            return "+";
        case BinOp::Sub:
            return "-";
        case BinOp::And:
            return "&";
        case BinOp::Or:
            return "|";
        case BinOp::Xor:
            return "^";
        }
        return "?";
    }

    struct BinExpr final : Expr
    {
        BinOp op;
        std::vector<std::shared_ptr<Expr>> ops;
        BinExpr(BinOp o, std::vector<std::shared_ptr<Expr>> v)
            : op(o), ops(std::move(v)) {}

        std::string emit() const override
        {
            std::ostringstream os;
            os << '(';
            for (std::size_t i = 0; i < ops.size(); ++i)
            {
                if (i)
                    os << ' ' << tok(op) << ' ';
                os << ops[i]->emit();
            }
            os << ')';
            return os.str();
        }

        uint32_t eval() const override
        {
            uint32_t acc = ops.front()->eval();
            for (std::size_t i = 1; i < ops.size(); ++i)
            {
                uint32_t r = ops[i]->eval();
                switch (op)
                {
                case BinOp::Add:
                    acc += r;
                    break;
                case BinOp::Sub:
                    acc -= r;
                    break;
                case BinOp::And:
                    acc &= r;
                    break;
                case BinOp::Or:
                    acc |= r;
                    break;
                case BinOp::Xor:
                    acc ^= r;
                    break;
                }
            }
            return acc;
        }
    };

    /*───────────────────────────────────────────────────────────────*/
    /* 2.  Statement-level AST                                       */
    /*───────────────────────────────────────────────────────────────*/
    inline std::string ind(int n) { return std::string(n, ' '); }

    struct Stmt
    {
        virtual ~Stmt() = default;
        virtual std::string emit(int) = 0;
    };

    /* continuous assignment */
    struct AssignStmt final : Stmt
    {
        std::string lhs;
        std::shared_ptr<Expr> rhs;
        AssignStmt(std::string l, std::shared_ptr<Expr> r) : lhs(std::move(l)), rhs(std::move(r)) {}
        std::string emit(int i) override { return ind(i) + "assign " + lhs + " = " + rhs->emit() + ";"; }
    };

    /* simple module instance */
    struct Instance final : Stmt
    {
        std::string mod, inst;
        std::vector<std::string> params;
        std::vector<std::pair<std::string, std::string>> conns;
        Instance(std::string m, std::string n,
                 std::vector<std::string> p,
                 std::vector<std::pair<std::string, std::string>> c)
            : mod(std::move(m)), inst(std::move(n)), params(std::move(p)), conns(std::move(c)) {}
        std::string emit(int i) override
        {
            std::ostringstream os;
            os << ind(i) << mod;
            if (!params.empty())
            {
                os << " #(";
                for (std::size_t k = 0; k < params.size(); ++k)
                {
                    if (k)
                        os << ", ";
                    os << params[k];
                }
                os << ')';
            }
            os << ' ' << inst << " (";
            for (std::size_t k = 0; k < conns.size(); ++k)
            {
                if (k)
                    os << ", ";
                os << '.' << conns[k].first << "(" << conns[k].second << ')';
            }
            os << ");";
            return os.str();
        }
    };

    /* arbitrary text */
    struct CustomStmt final : Stmt
    {
        std::function<std::string(int)> fn;
        explicit CustomStmt(std::function<std::string(int)> f) : fn(std::move(f)) {}
        std::string emit(int i) override { return fn(i); }
    };

    /* for-generate loop (assumes already inside generate) */
    struct GenerateFor final : Stmt
    {
        std::string var, label;
        int start, end;
        std::vector<std::shared_ptr<Stmt>> body;
        GenerateFor(std::string v, std::string l, int s, int e,
                    std::vector<std::shared_ptr<Stmt>> b)
            : var(std::move(v)), label(std::move(l)), start(s), end(e), body(std::move(b)) {}
        std::string emit(int i) override
        {
            std::ostringstream os;
            os << ind(i) << "genvar " << var << ";\n";
            os << ind(i) << "for(" << var << '=' << start << "; " << var << '<' << end << "; "
               << var << '=' << var << "+1) begin : " << label << "\n";
            for (auto &s : body)
                os << s->emit(i + 4) << "\n";
            os << ind(i) << "end";
            return os.str();
        }
    };

    /* if-generate */
    struct GenerateIf final : Stmt
    {
        std::shared_ptr<Expr> cond;
        std::vector<std::shared_ptr<Stmt>> t, e;
        GenerateIf(std::shared_ptr<Expr> c,
                   std::vector<std::shared_ptr<Stmt>> t_,
                   std::vector<std::shared_ptr<Stmt>> e_ = {})
            : cond(std::move(c)), t(std::move(t_)), e(std::move(e_)) {}
        std::string emit(int i) override
        {
            std::ostringstream os;
            os << ind(i) << "if(" << cond->emit() << ") begin\n";
            for (auto &s : t)
                os << s->emit(i + 2) << "\n";
            if (!e.empty())
            {
                os << ind(i) << "end else begin\n";
                for (auto &s : e)
                    os << s->emit(i + 2) << "\n";
            }
            os << ind(i) << "end";
            return os.str();
        }
    };

    /* case-generate */
    struct GenerateCase final : Stmt
    {
        std::shared_ptr<Expr> sel;
        std::vector<std::pair<std::shared_ptr<Expr>,
                              std::vector<std::shared_ptr<Stmt>>>>
            cases;
        std::vector<std::shared_ptr<Stmt>> def;
        GenerateCase(std::shared_ptr<Expr> s,
                     decltype(cases) c,
                     std::vector<std::shared_ptr<Stmt>> d = {})
            : sel(std::move(s)), cases(std::move(c)), def(std::move(d)) {}
        std::string emit(int i) override
        {
            std::ostringstream os;
            os << ind(i) << "case(" << sel->emit() << ")\n";
            for (auto &kv : cases)
            {
                os << ind(i) << "  " << kv.first->emit() << ": begin\n";
                for (auto &s : kv.second)
                    os << s->emit(i + 4) << "\n";
                os << ind(i) << "  end\n";
            }
            if (!def.empty())
            {
                os << ind(i) << "  default: begin\n";
                for (auto &s : def)
                    os << s->emit(i + 4) << "\n";
                os << ind(i) << "  end\n";
            }
            os << ind(i) << "endcase";
            return os.str();
        }
    };

    /*───────────────────────────────────────────────────────────────*/
    /* 3.  Module container                                          */
    /*───────────────────────────────────────────────────────────────*/
    struct Module
    {
        std::string name;
        std::vector<std::shared_ptr<Stmt>> body;
        std::vector<std::string> ports;
        std::string emit() const
        {
            std::ostringstream os;
            os << "module " << name << "(\n";
            for (std::size_t i = 0; i < ports.size(); ++i)
                os << "    " << ports[i] << (i + 1 < ports.size() ? ",\n" : "\n");
            os << ");\n";
            for (auto &s : body)
                os << s->emit(2) << "\n";
            os << "endmodule\n";
            return os.str();
        }
    };

    /*───────────────────────────────────────────────────────────────*/
    /* 4.  Hierarchical random generator                             */
    /*───────────────────────────────────────────────────────────────*/
    class Generator
    {
        std::mt19937 rng;

        /* ONLY '+' and '^' are allowed now */
        static inline const std::vector<char> OPS{'+', '^'};
        static BinOp opFromChar(char c) { return (c == '+') ? BinOp::Add : BinOp::Xor; }

        /* shorthand for const_block instantiation */
        static std::shared_ptr<Stmt> constInst(const std::string &w, const std::string &tgt)
        {
            return std::make_shared<Instance>("const_block", "inst",
                                              std::vector<std::string>{w},
                                              std::vector<std::pair<std::string, std::string>>{{"w", tgt}});
        }

    public:
        bool doHierarchy, doFor, doIf, doCase;
        Generator(bool h, bool f, bool i, bool c, unsigned seed)
            : rng(seed), doHierarchy(h), doFor(f), doIf(i), doCase(c) {}

        /* recursive builder */
        std::shared_ptr<Stmt>
        buildNested(int level, int maxDepth, int N,
                    const std::string &out,
                    std::vector<std::shared_ptr<Stmt>> &decls)
        {
            /* base case -> const block */
            if (level == maxDepth)
            {
                std::ostringstream p;
                p << "(32'h"
                  << std::hex << std::setw(8) << std::setfill('0')
                  << (0xDEADBEEF + level) << ')';
                return constInst(p.str(), out);
            }

            /* choose construct */
            std::vector<int> choices;
            if (doFor)
                choices.push_back(0);
            if (doIf)
                choices.push_back(1);
            if (doCase)
                choices.push_back(2);
            if (choices.empty())
                choices.push_back(0);

            int pick = choices[rng() % choices.size()];

            /* ---------- FOR ---------- */
            if (pick == 0)
            {
                std::string var = "g" + std::to_string(level);
                std::string lbl = "lvl" + std::to_string(level);
                std::string arr = "t" + std::to_string(level);

                decls.push_back(std::make_shared<CustomStmt>(
                    [arr, N](int i)
                    { std::ostringstream os;
                                os<<ind(i)<<"wire [31:0] "<<arr<<" [0:"<<N-1<<"];";
                                return os.str(); }));

                std::vector<std::shared_ptr<Stmt>> body;
                body.push_back(buildNested(level + 1, maxDepth, N,
                                           arr + "[" + var + "]", decls));

                std::ostringstream red;
                red << arr << "[0]";
                for (int k = 1; k < N; ++k)
                    red << " ^ " << arr << "[" << k << "]";

                body.push_back(std::make_shared<AssignStmt>(
                    out, std::make_shared<WireRef>(red.str())));

                return std::make_shared<GenerateFor>(var, lbl, 0, N, std::move(body));
            }

            /* ---------- IF ---------- */
            if (pick == 1)
            {
                auto cond = std::make_shared<Const>(1);
                std::vector<std::shared_ptr<Stmt>> thenB{
                    buildNested(level + 1, maxDepth, N, out, decls)};
                return std::make_shared<GenerateIf>(cond, thenB);
            }

            /* ---------- CASE ---------- */
            auto inner = buildNested(level + 1, maxDepth, N, out, decls);
            auto sel = std::make_shared<WireRef>("sel");
            return std::make_shared<GenerateCase>(sel,
                                                  std::vector<std::pair<std::shared_ptr<Expr>,
                                                                        std::vector<std::shared_ptr<Stmt>>>>{
                                                      {std::make_shared<Const>(0), {inner}}});
        }

        /* ---------------------------------------------------------- */
        std::pair<std::filesystem::path, uint32_t>
        make(const std::string &topName, int idx, int depth = 2, int N = 4)
        {
            Module top;
            top.name = topName;
            top.ports = {"output [31:0] result"};

            /* declare top-level array */
            top.body.push_back(std::make_shared<CustomStmt>(
                [N](int i)
                { std::ostringstream os;
                        os<<ind(i)<<"wire [31:0] g [0:"<<N-1<<"];";
                        return os.str(); }));

            std::vector<std::shared_ptr<Stmt>> decls;
            auto outer = buildNested(0, depth, N, "g[g0]", decls);

            /* one generate/endgenerate wrapper */
            top.body.insert(top.body.end(), decls.begin(), decls.end());
            top.body.push_back(std::make_shared<CustomStmt>(
                [outer](int i)
                {
                    std::ostringstream os;
                    os << ind(i) << "generate\n"
                       << outer->emit(i + 2) << "\n"
                       << ind(i) << "endgenerate";
                    return os.str();
                }));

            /* XOR reduction */
            std::shared_ptr<Expr> acc = std::make_shared<WireRef>("g[0]");
            for (int k = 1; k < N; ++k)
                acc = std::make_shared<BinExpr>(BinOp::Xor,
                                                std::vector<std::shared_ptr<Expr>>{
                                                    acc, std::make_shared<WireRef>("g[" + std::to_string(k) + "]")});
            top.body.push_back(std::make_shared<AssignStmt>("result", acc));

            /* dump Verilog file */
            std::filesystem::path fn = "gen_" + std::to_string(idx) + ".v";
            std::ofstream f(fn);
            if (!f)
                throw std::runtime_error("open " + fn.string());
            f << "// generated by veri::Generator\ntimescale 1ns/1ps\n\n";
            f << "module const_block #(parameter VALUE=32'h0)(output [31:0] w);\n"
              << "  assign w = VALUE;\nendmodule\n\n";
            f << top.emit();

            return {fn, 0};
        }
    };

} // namespace veri
