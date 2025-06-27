/**********************************************************************
 * hierarchy_generator.hpp
 * --------------------------------------------------------------------
 *    Generates one Verilog file that stresses hierarchical
 *    name-resolution (absolute $root paths, “..” upward refs^,
 *    defparam, alias^ … being optional), using the AST helpers defined
 *    in ast.hpp.
 *    ^ = (implementation in progress)
 *********************************************************************/
#pragma once
#include "ast.hpp"

#include <random>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace veri {

/* ────────────────────────────────────────────────────────────────
*  0.  Global knobs Configutation, overwritten by CLI args
* ────────────────────────────────────────────────────────────────*/
struct HierCfg {
    bool   rootPrefix  = false;
    bool   relativeUp  = false;
    bool   aliasStmt   = false;
    bool   defparam    = false;
    bool   enableBigGen = false;
    int    minChild    = 2;
    int    maxChild    = 4;
    int    depth       = 2;
    double bigGenProb  = 0.5;   // chance that a leaf is a “Generate” module
};

constexpr int  BIT_WIDTH       = 32;        // width of the result, can be changed. NOTE: ast.hpp uses 32-bit integers currently, with plans to change this in the future.
constexpr char ROOT_NAME[]     = "top";     // usually always "top", can be changed.

/* ────────────────────────────────────────────────────────────────
*  1.  Small helpers
* ────────────────────────────────────────────────────────────────*/
inline std::string hex32(uint32_t v)
{
    std::ostringstream s;
    s << "32'h" << std::hex << std::setw(8) << std::setfill('0') << v;
    return s.str();
}

// Forward decl.
struct Node;
inline void collect_leaf_paths(const std::string& prefix,
                            const Node& node,
                            std::vector<std::string>& out);

// Strip any $root./top./..  decoration so we can map the path to the Node hierarchy again when calculating the golden value.
inline std::string normalised(std::string p)
{
    if (p.rfind("$root.", 0) == 0)           // drop "$root."
        p.erase(0, 6);
    while (p.rfind(std::string(ROOT_NAME) + ".", 0) == 0)
        p.erase(0, std::string(ROOT_NAME).size() + 1);

    while (p.rfind("..", 0) == 0) {          // strip leading ".."
        auto firstDot = p.find('.', 2);
        p.erase(0, firstDot == std::string::npos ? p.size() : firstDot + 1);
    }
    return p;
}

// Walk the tree to fetch the literal value that belongs to a leaf path
inline uint32_t leaf_value(const Node& here, std::string_view dotted);

/* ────────────────────────────────────────────────────────────────
*  2.  Internal recursive node structure used while building
* ────────────────────────────────────────────────────────────────*/
struct Node {
    std::string                       name;
    std::vector<std::unique_ptr<Node>>children;
    uint32_t  const_val = 0;          // set for leaves (when !cfg_.defparam)
    bool                         bigGen   = false;
    std::shared_ptr<Module>      bigMod; 
};

/* ────────────────────────────────────────────────────────────────
*  3.  Main generator
* ────────────────────────────────────────────────────────────────*/
class HierarchyGen {
    std::mt19937 rng_;
    unsigned     seed_;
    HierCfg      cfg_;
    uint32_t     expected_ = 0;     // value of the *root* (expected result passed to Equivalency Checker in main.cpp)
    private:
        Generator                       bigGen_;      // per‐instance Generator
        std::unordered_set<std::string> dumpedMods_;  // per‐instance “have I emitted this submodule?”
        std::shared_ptr<Module> lastGenModule_;

    // tree construction
    std::unique_ptr<Node> build_tree(const std::string& name,
                                    int depth, int maxDepth)
    {
        auto n   = std::make_unique<Node>();
        n->name  = name;

        if (depth == maxDepth) {
            if (cfg_.enableBigGen && std::bernoulli_distribution(cfg_.bigGenProb)(rng_)) {
                auto [mod,val] = bigGen_.makeModule(name, cfg_.depth);   // bigGen_ : Generator
                n->bigGen  = true;
                n->bigMod  = std::move(mod);
                n->const_val = val;                          // for golden value
            } else {
                n->const_val = std::uniform_int_distribution<uint32_t>()(rng_);
            }
            return n;
        }
        int nc = std::uniform_int_distribution<>(cfg_.minChild, cfg_.maxChild)(rng_);
        for (int i = 0; i < nc; ++i)
            n->children.emplace_back(
                build_tree(name + "_c" + std::to_string(i),
                        depth + 1, maxDepth));
        return n;
    }

    // Verilog emission
    struct ModEmit {
        std::shared_ptr<Module> mod;
        std::string             text;   // Verilog (module + children)
        uint32_t                value;  // evaluated result
    };

    static void set_leaf(Node& here, std::string_view dotted, uint32_t nv)
    {
        auto dot = dotted.find('.');
        if (dot == std::string_view::npos) {                 // reached leaf
            for (auto& k : here.children)
                if (k->name == dotted) { k->const_val = nv; return; }
            return;
        }
        std::string_view head = dotted.substr(0, dot);
        for (auto& k : here.children)
            if (k->name == head) {
                set_leaf(*k, dotted.substr(dot + 1), nv);
                return;
            }
    }

    /* ------------------------------------------------------------------ */
    /*  emit_module – recursively emit Verilog + compute golden value     */
    /* ------------------------------------------------------------------ */
    ModEmit emit_module(const Node& n, int depth)
    {
        // choose port name
        const bool   isRoot = (depth == 0);        // only the very top root uses result others retain out
        const char*  PORT   = "out";

        // create Module AST node
        auto m   = std::make_shared<Module>();
        m->name  = n.name;
        m->ports = { "output [" + std::to_string(BIT_WIDTH-1) + ":0] " + PORT };

        std::ostringstream vtxt;                  // textual output

        // leaf
        if (n.children.empty()) {
            if (n.bigGen) { 
                if (dumpedMods_.insert("const_block").second) {
                    vtxt << "module const_block #(parameter VALUE=32'h0)(output [31:0] w);\n"
                        << "  assign w = VALUE;\nendmodule\n\n";
                }

                // emit the module text once
                if (dumpedMods_.insert(n.bigMod->name).second)
                    vtxt << n.bigMod->emit();

                // instantiate it 
                m->body.push_back(std::make_shared<Instance>(
                    n.bigMod->name,               // module name
                    n.name + "_inst",             // unique instance
                    std::vector<std::string>{},   // no parameters
                    std::vector<std::pair<std::string,std::string>>{
                        {"result", PORT}          // connect its 'result' to our PORT
                    }));

                return {m, vtxt.str(), n.const_val};
            }            
            if (cfg_.defparam) {              // parameterised leaf
                vtxt << "module " << n.name
                     << " #(parameter VALUE = " << hex32(n.const_val) << ")"
                     << " (output [" << BIT_WIDTH-1 << ":0] " << PORT << ");\n"
                     << "  assign " << PORT << " = VALUE;\nendmodule\n\n";
            } else {
                m->body.push_back(
                    std::make_shared<AssignStmt>(
                        PORT,
                        std::make_shared<Const>(
                            n.const_val,
                            std::to_string(BIT_WIDTH) + "'d" +
                            std::to_string(n.const_val))));
                vtxt << m->emit();
            }
            return {m, vtxt.str(), n.const_val};
        }

        // recurse & instantiate children
        std::vector<std::string> childTxt;
        for (const auto& k : n.children) {
            auto ce = emit_module(*k, depth + 1);
            childTxt.push_back(ce.text);

            // empty port-list (hierarchical access only)
            m->body.push_back(
                std::make_shared<Instance>(k->name, k->name,
                    std::vector<std::string>{},
                    std::vector<std::pair<std::string,std::string>>{}));
        }

        // build reduction over leaf paths
        std::vector<std::string> leaves;
        collect_leaf_paths("", n, leaves);
        std::shuffle(leaves.begin(), leaves.end(), rng_);
        
        if (isRoot && cfg_.defparam && !leaves.empty())
        {
            std::string tgt = leaves.front();
            uint32_t    nv  = std::uniform_int_distribution<uint32_t>()(rng_);
        
            std::string inst_path = tgt.substr(0, tgt.rfind('.')); // strip ".out"
            std::string norm_path = normalised(inst_path);

            set_leaf(const_cast<Node&>(n), norm_path, nv);
        
            m->body.push_back(std::make_shared<CustomStmt>(
                [inst_path, nv](int i){
                    std::ostringstream s;
                    s << ind(i) << "defparam "
                    << inst_path << ".VALUE = " << hex32(nv) << ";";
                    return s.str();
                }));
        }         

        int nOps = std::uniform_int_distribution<>(2,
                    static_cast<int>(leaves.size()))(rng_);

        auto qualify = [this,depth](const std::string& p)->std::string {
            if (cfg_.rootPrefix && std::uniform_real_distribution<>(0.0,1.0)(rng_) < 0.33)
            {
                std::string s = p;
                if (s.rfind(std::string(ROOT_NAME)+".",0)==0)
                    s.erase(0, std::string(ROOT_NAME).size()+1);
                return std::string("$root.") + "tb." + ROOT_NAME + "." + s;
            }
            if (cfg_.relativeUp && depth>=1 &&
                std::uniform_real_distribution<>(0.0,1.0)(rng_) < 0.5)
            {
                return std::string("..") + p.substr(p.find('.')+1);
            }
            return p;
        };

        std::vector<std::string> opsTxt;
        std::vector<uint32_t>    opsVal;
        for (int i=0;i<nOps;++i){
            opsTxt .push_back( qualify(leaves[i]) );
            opsVal .push_back( leaf_value(n, normalised(leaves[i])) );
        }
        if (std::uniform_int_distribution<>(0,1)(rng_)){
            uint32_t lit = std::uniform_int_distribution<uint32_t>()(rng_);
            opsTxt.push_back(std::to_string(BIT_WIDTH)+"'d"+std::to_string(lit));
            opsVal.push_back(lit);
        }

        const char OPS[] = { '+','|','&','^' };
        char opSym = OPS[ std::uniform_int_distribution<>(0,3)(rng_) ];

        std::string rhs = opsTxt.front();
        for(size_t i=1;i<opsTxt.size();++i){
            rhs += ' '; rhs += opSym; rhs += ' ';
            rhs += opsTxt[i];
        }

        // save Verilog line
        m->body.push_back(std::make_shared<CustomStmt>(
            [rhs,PORT](int ind){
            return std::string(ind,' ') + "assign " + PORT + " = " + rhs + ";";
            }));

        // evaluate expected value
        uint32_t acc = opsVal.front();
        for(size_t i=1;i<opsVal.size();++i){
            switch(opSym){
                case '+': acc += opsVal[i]; break;
                case '|': acc |= opsVal[i]; break;
                case '&': acc &= opsVal[i]; break;
                case '^': acc ^= opsVal[i]; break;
            }
        }

        // emit accumulated text
        vtxt << m->emit();
        for(auto& t : childTxt) vtxt << t;

        return {m, vtxt.str(), acc};
    }

    // collect leaves (helper declared earlier)
    friend void collect_leaf_paths(const std::string&, const Node&, std::vector<std::string>&);

public:
    explicit HierarchyGen(unsigned seed, const HierCfg& c)
    : rng_(seed), seed_(seed), cfg_(c), bigGen_(seed) {}

    // public API
    std::pair<std::filesystem::path,uint32_t>
    write(const std::string& fileName = "hier_test.v")
    {   
        dumpedMods_.clear();
        auto root    = build_tree(ROOT_NAME, 0, cfg_.depth);
        auto emitted = emit_module(*root, 0);           // includes value
        expected_    = emitted.value;   // 0 when cfg_.defparam may alter it

        std::ostringstream hdr;
        hdr << "// auto-generated by VeriGen-Hierarchy \n// seed: " << seed_ << "\n"
            << "`timescale 1ns/1ps\n\n";
        std::string text = hdr.str() + emitted.text;

        // alias - not yet functional
        if (cfg_.aliasStmt) {
            std::vector<std::string> leaves;
            collect_leaf_paths("", *root, leaves);
            if (leaves.size() >= 2) {
                hdr.str(""); hdr.clear();
                hdr << "\n// ---------- cross-hierarchy extras ----------\n";
                if (cfg_.aliasStmt) {
                    std::uniform_int_distribution<> d(0, leaves.size()-1);
                    std::string a = leaves[d(rng_)], b = leaves[d(rng_)];
                    while (b == a) b = leaves[d(rng_)];
                    hdr << "alias " << a << " = " << b << ";\n";
                }
                text += hdr.str();
            }
        }

        lastGenModule_ = emitted.mod;
        std::ofstream f(fileName);
        f << text;
        return { std::filesystem::absolute(fileName), expected_ };    
    }

    // expose expected result
    uint32_t expected() const { return expected_; }

    // expose the last generated module
    std::shared_ptr<Module> lastModule() const {
        return lastGenModule_;
    }
};

/* ===== helper implementation ===================================*/
inline void collect_leaf_paths(const std::string& prefix,
                            const Node& node,
                            std::vector<std::string>& out)
{
    if (node.children.empty()) {
        out.push_back(prefix + node.name + ".out");
        return;
    }
    for (const auto& k : node.children)
        collect_leaf_paths(prefix + node.name + ".", *k, out);
}

inline uint32_t leaf_value(const Node& here, std::string_view dotted)
{
   
    if (dotted.empty() || dotted == "out")          // reached the pin -> literal found
        return here.const_val;


    auto dot  = dotted.find('.');
    auto head = dotted.substr(0, dot);

    for (const auto& k : here.children)
        if (k->name == head) {
            if (dot == std::string_view::npos)      // no more segments
                return k->const_val;                // (should not happen)
            return leaf_value(*k, dotted.substr(dot + 1));
        }
    return 0;                                       // unreachable
}

} // namespace veri