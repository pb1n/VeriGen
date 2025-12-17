cmake_minimum_required(VERSION 3.15)
project(reVeriGen VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# AST library
add_library(ast_lib
    src/ast/ast_visitor.cpp
    src/ast/reference_model_visitor.cpp
    src/generators/random_expr_gen.cpp
)

target_include_directories(ast_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tools library (simulators, parsers, etc.)
add_library(tools_lib
    src/tools/parsers/monitor_parser.cpp
    src/tools/icarus/icarus_tool.cpp
)

target_include_directories(tools_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tools library depends on ast_lib for common types
target_link_libraries(tools_lib PUBLIC ast_lib)

# Example executable
add_executable(ast_example
    src/ast/example.cpp
)

# Generator test executables
add_executable(test_generate_constructs
    src/generators/tests/test_generate_constructs.cpp
    src/generators/case_gen.cpp
)

add_executable(test_refmodel_gen
    src/generators/tests/test_refmodel_gen.cpp
)

target_link_libraries(test_generate_constructs PRIVATE ast_lib tools_lib)
target_link_libraries(test_refmodel_gen PRIVATE ast_lib tools_lib)

target_link_libraries(ast_example PRIVATE ast_lib)

# Enable testing
enable_testing()

# Test executable
add_executable(run_tests
    tests/test_main.cpp
    tests/test_expressions.cpp
    tests/test_statements.cpp
    tests/test_declarations.cpp
    tests/test_emission.cpp
    tests/test_visitor.cpp
)

target_include_directories(run_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(run_tests PRIVATE ast_lib)

# Add test to CTest
add_test(NAME ast_tests COMMAND run_tests)

# Installation
install(TARGETS ast_lib ast_example
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY src/ast/
    DESTINATION include/ast
    FILES_MATCHING PATTERN "*.hpp"
)
add_executable(test_refmodel_complex src/generators/tests/test_refmodel_complex.cpp)
target_link_libraries(test_refmodel_complex PRIVATE ast_lib tools_lib)
add_executable(test_refmodel_verification src/generators/tests/test_refmodel_verification.cpp)
target_link_libraries(test_refmodel_verification PRIVATE ast_lib tools_lib)
add_executable(test_random_fuzzer src/generators/tests/test_random_fuzzer.cpp)
target_link_libraries(test_random_fuzzer PRIVATE ast_lib tools_lib)

# Tool framework tests
add_executable(test_icarus_tool src/tools/icarus/test_icarus_tool.cpp)
target_link_libraries(test_icarus_tool PRIVATE ast_lib tools_lib)
