#pragma once
/*───────────────────────────────────────────────────────────────*
 * Compact hierarchical-test-source generator for Verilog       *
 * – outer generate/endgenerate wrapper                         *
 * – ADD / XOR only                                             *
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
#include <algorithm>

namespace veri
{

    /*───────────────────────────────────────────────────────────────*/
    /* 1.  Expression-level AST                                      */
    /*───────────────────────────────────────────────────────────────*/
    struct Expr
    {
        virtual ~Expr() = default;
        virtual std::string emit() const = 0;
        virtual uint32_t eval(const std::vector<uint32_t>& values) const = 0;
    };

    /* constant literal or aliased symbol */
    struct Const final : Expr
    {
        uint32_t value;
        std::string sym;
        Const(uint32_t v, std::string s = {}) : value(v), sym(std::move(s)) {}
        std::string emit() const override { return sym; }
        uint32_t eval(const std::vector<uint32_t>& /*values*/) const override { return value; }
    };

    /* reference to an existing net */
    struct WireRef final : Expr
    {
        std::string name;
        int index; // Store index for evaluation
        explicit WireRef(std::string n, int idx = -1) : name(std::move(n)), index(idx) {}
        std::string emit() const override { return name; }
        uint32_t eval(const std::vector<uint32_t>& values) const override {
            if (index < 0 || static_cast<size_t>(index) >= values.size()) {
                 throw std::out_of_range("WireRef evaluation index out of range.");
            }
            return values[index];
        }
    };

    /* binary expression ─ (op restricted later) */
    enum class BinOp
    {
        Add,
        Xor
    };

    inline const char *tok(BinOp o)
    {
        switch (o)
        {
        case BinOp::Add:
            return "+";
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

        uint32_t eval(const std::vector<uint32_t>& values) const override
        {
            if (ops.empty()) return 0;
            uint32_t acc = ops.front()->eval(values);
            for (std::size_t i = 1; i < ops.size(); ++i)
            {
                uint32_t r = ops[i]->eval(values);
                switch (op)
                {
                case BinOp::Add:
                    acc += r;
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
                for(size_t k = 0; k < params.size(); ++k) {
                    if (k > 0) os << ", ";
                    os << params[k];
                }
                os << ")";
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
        int start;
        std::string condition, update_expr;
        std::vector<std::shared_ptr<Stmt>> body;

        GenerateFor(std::string v, std::string l, int s, std::string c, std::string u,
                    std::vector<std::shared_ptr<Stmt>> b)
            : var(std::move(v)), label(std::move(l)), start(s),
              condition(std::move(c)), update_expr(std::move(u)), body(std::move(b)) {}

        std::string emit(int i) override
        {
            std::ostringstream os;
            os << ind(i) << "genvar " << var << ";\n";
            os << ind(i) << "for(" << var << '=' << start << "; " << condition << "; " << update_expr << ") begin : " << label << "\n";
            for (auto &s : body)
                os << s->emit(i + 4) << "\n";
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
                os << ind(i+2) << kv.first->emit() << ": ";
                if(kv.second.size() == 1) {
                     os << kv.second[0]->emit(0) << "\n";
                } else {
                    os << "begin\n";
                    for (auto &s : kv.second)
                        os << s->emit(i + 4) << "\n";
                    os << ind(i+2) << "end\n";
                }
            }
            if (!def.empty())
            {
                os << ind(i+2) << "default: begin\n";
                for (auto &s : def)
                    os << s->emit(i + 4) << "\n";
                os << ind(i+2) << "end\n";
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
        static inline const std::vector<BinOp> OPS{BinOp::Add, BinOp::Xor};
        std::vector<uint32_t> const_data;
        std::vector<std::vector<std::shared_ptr<Expr>>> logic_trees;
        std::vector<int> N_per_level;
        std::shared_ptr<Expr> final_logic_tree;
        
        int min_start, max_start;
        int min_iter, max_iter;
        bool random_update;

        static std::shared_ptr<Stmt> constInst(const std::string &w_param, const std::string &tgt)
        {
            return std::make_shared<Instance>("const_block", "inst",
                                              std::vector<std::string>{".VALUE(" + w_param + ")"},
                                              std::vector<std::pair<std::string, std::string>>{{"w", tgt}});
        }

        std::shared_ptr<Stmt> buildNested(int level, int maxDepth, const std::string &out_base_name) {
            
            std::string var = "g" + std::to_string(level);
            std::string lbl = "lvl" + std::to_string(level);
            std::vector<std::shared_ptr<Stmt>> loop_body;

            std::uniform_int_distribution<> start_dist(min_start, max_start);
            int start_val = start_dist(rng);

            std::uniform_int_distribution<> iter_dist(min_iter, max_iter);
            int num_iterations = iter_dist(rng);
            if(static_cast<size_t>(level) < N_per_level.size()) {
                N_per_level[level] = num_iterations;
            }

            std::string update_expr, cond_expr, index;
            bool increment = random_update ? (rng() % 2 == 0) : true;
            
            if (increment) {
                update_expr = var + " = " + var + " + 1";
                cond_expr = var + " < " + std::to_string(start_val + num_iterations);
                index = var + " - " + std::to_string(start_val);
            } else { 
                update_expr = var + " = " + var + " - 1";
                cond_expr = var + " > " + std::to_string(start_val - num_iterations);
                index = std::to_string(start_val) + " - " + var;
            }


            if (level >= maxDepth) {
                std::string const_param = "CONSTS0[(" + index + ")*32 +: 32]";
                loop_body.push_back(constInst(const_param, out_base_name + "[(" + index + ")]"));
            } else {
                std::string next_level_arr = "t" + std::to_string(level + 1);
                
                // Recursively build the inner loop first to get its size
                auto inner_loop = buildNested(level + 1, maxDepth, next_level_arr);
                int next_level_iters = N_per_level[level+1];
                
                // Declare the wire array with the correct size
                loop_body.push_back(std::make_shared<CustomStmt>(
                    [next_level_arr, next_level_iters](int i){
                        return ind(i) + "wire [31:0] " + next_level_arr + " [0:" + std::to_string(next_level_iters-1) + "];";
                    }
                ));
                
                loop_body.push_back(inner_loop);

                std::vector<std::pair<std::shared_ptr<Expr>, std::vector<std::shared_ptr<Stmt>>>> case_items;
                std::vector<std::shared_ptr<Expr>> level_logic;
                for (int k = 0; k < num_iterations; ++k) {
                    std::vector<std::shared_ptr<Stmt>> assign_body;
                    
                    std::uniform_int_distribution<> op_dist(0, OPS.size() - 1);
                    std::shared_ptr<Expr> reduction_acc = std::make_shared<WireRef>(next_level_arr + "[0]", 0);
                    for (int op_idx = 1; op_idx < next_level_iters; ++op_idx) {
                        BinOp random_op = OPS[op_dist(rng)];
                        reduction_acc = std::make_shared<BinExpr>(random_op, 
                            std::vector<std::shared_ptr<Expr>>{
                                reduction_acc, 
                                std::make_shared<WireRef>(next_level_arr + "[" + std::to_string(op_idx) + "]", op_idx)
                            });
                    }
                    level_logic.push_back(reduction_acc);
                    
                    std::string assign_lhs = out_base_name + "[" + std::to_string(k) + "]";
                    assign_body.push_back(std::make_shared<AssignStmt>(assign_lhs, reduction_acc));
                    
                    case_items.push_back({std::make_shared<Const>(start_val + (increment ? k : -k), std::to_string(start_val + (increment ? k : -k))), std::move(assign_body)});
                }
                logic_trees.push_back(level_logic);
                
                auto sel = std::make_shared<WireRef>(var);
                loop_body.push_back(std::make_shared<GenerateCase>(sel, std::move(case_items)));
            }
            return std::make_shared<GenerateFor>(var, lbl, start_val, cond_expr, update_expr, std::move(loop_body));
        }

       uint32_t calculateExpectedResult() {
            std::vector<uint32_t> current_level_values = const_data;
            
            for (size_t i = 0; i < logic_trees.size(); ++i) {
                std::vector<uint32_t> next_level_values;
                const auto& logic_for_this_level = logic_trees[i];

                for (size_t k = 0; k < logic_for_this_level.size(); ++k) { 
                    uint32_t val = logic_for_this_level[k]->eval(current_level_values);
                    next_level_values.push_back(val);
                }
                current_level_values = next_level_values;
            }
            
            return final_logic_tree->eval(current_level_values);
        }

    public:
        // Constructor that matches the main.cpp call
        Generator(unsigned seed, int min_s=0, int max_s=0, int min_i=2, int max_i=16, bool rand_up=true) 
            : rng(seed), min_start(min_s), max_start(max_s), min_iter(min_i), max_iter(max_i), random_update(rand_up) {}

        std::pair<std::filesystem::path, uint32_t>
        make(const std::string &topName, int idx, int depth = 2)
        {
            Module top;
            top.name = topName;
            top.ports = {"output [31:0] result"};
            
            const_data.clear();
            logic_trees.clear();
            final_logic_tree = nullptr;
            N_per_level.assign(depth + 1, 0);

            int maxDepth = (depth > 0) ? depth - 1 : 0;
            
            auto outer_loop_stmts = buildNested(0, maxDepth, "t0");

            int top_N = N_per_level[0];
            int base_N = N_per_level[maxDepth];

            for (int j = 0; j < base_N; ++j) {
                std::uniform_int_distribution<uint32_t> val_dist;
                const_data.push_back(val_dist(rng));
            }
            
            top.body.push_back(std::make_shared<CustomStmt>(
                [base_N, this](int indent){
                    std::ostringstream os;
                    os << ind(indent) << "localparam [" << base_N*32-1 << ":0] CONSTS0 = {";
                    for(int j=base_N-1; j>=0; --j) { 
                        os << "32'h" << std::hex << std::setw(8) << std::setfill('0') << this->const_data[j];
                        if (j > 0) os << ", ";
                    }
                    os << "};";
                    return os.str();
                }
            ));

            top.body.push_back(std::make_shared<CustomStmt>(
                [top_N](int i) {
                    return ind(i) + "wire [31:0] t0 [0:" + std::to_string(top_N - 1) + "];";
                }));

            std::reverse(logic_trees.begin(), logic_trees.end());

            top.body.push_back(std::make_shared<CustomStmt>(
                [outer_loop_stmts](int i) {
                    std::ostringstream os;
                    os << ind(i) << "generate\n"
                       << outer_loop_stmts->emit(i + 2) << "\n"
                       << ind(i) << "endgenerate";
                    return os.str();
                }));
            
            if (top_N > 0) {
                std::uniform_int_distribution<> op_dist(0, OPS.size() - 1);
                final_logic_tree = std::make_shared<WireRef>("t0[0]", 0);
                for (int k = 1; k < top_N; ++k) {
                     BinOp random_op = OPS[op_dist(rng)];
                     final_logic_tree = std::make_shared<BinExpr>(random_op, 
                        std::vector<std::shared_ptr<Expr>>{
                            final_logic_tree, 
                            std::make_shared<WireRef>("t0[" + std::to_string(k) + "]", k)
                        });
                }
                top.body.push_back(std::make_shared<AssignStmt>("result", final_logic_tree));
            }

            uint32_t expected_result = calculateExpectedResult();

            /* dump Verilog file */
            std::filesystem::path fn = "gen_" + std::to_string(idx) + ".v";
            std::ofstream f(fn);
            if (!f)
                throw std::runtime_error("open " + fn.string());
            f << "// generated by veri::Generator\n`timescale 1ns/1ps\n\n";
            f << "module const_block #(parameter VALUE=32'h0)(output [31:0] w);\n"
              << "  assign w = VALUE;\nendmodule\n\n";
            f << top.emit();

            return {fn, expected_result};
        }
    };

} // namespace veri
