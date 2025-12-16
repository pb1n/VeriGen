#ifndef VERIGEN_GENERATORS_CASE_GEN_HPP
#define VERIGEN_GENERATORS_CASE_GEN_HPP

#include "ast/ast_module.hpp"

namespace generators {

    /**
     * Generates a module "case_test_module" that implements a simple ALU
     * using a Verilog generate-case statement.
     * * Logic:
     * parameter MODE = 0;
     * generate case (MODE)
     * 0: y = a + b;
     * 1, 2: y = a - b;
     * default: y = 0;
     * endcase endgenerate
     * * @param width The bitwidth for the input/output ports (default 32)
     */
    ast::Ptr<ast::ModuleDefn> generateCaseTest(int width = 32);

}

#endif // VERIGEN_GENERATORS_CASE_GEN_HPP