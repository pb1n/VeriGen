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
)

target_include_directories(ast_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Example executable
add_executable(ast_example
    src/ast/example.cpp
)

# Generator test executable
add_executable(test_counter_gen
    src/generators/test_counter_gen.cpp
)

target_link_libraries(test_counter_gen PRIVATE ast_lib)

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
