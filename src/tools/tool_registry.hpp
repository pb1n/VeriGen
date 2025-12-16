/**
 * Tool Registry - Plugin System for User-Defined Tools
 *
 * Allows users to register custom tools and parsers at runtime.
 */

#pragma once

#include "tool_base.hpp"
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <stdexcept>

namespace tools {

/**
 * Factory function type for creating tools
 */
using ToolFactory = std::function<std::unique_ptr<Tool>()>;

/**
 * Global registry for tools and parsers
 *
 * Usage:
 *   // Register a tool
 *   ToolRegistry::instance().registerTool("icarus", []() {
 *       return std::make_unique<IcarusTool>();
 *   });
 *
 *   // Get a tool
 *   auto tool = ToolRegistry::instance().getTool("icarus");
 *
 *   // Register a parser
 *   ToolRegistry::instance().registerParser("vcd",
 *       std::make_unique<VCDParser>());
 */
class ToolRegistry {
public:
    // Singleton access
    static ToolRegistry& instance() {
        static ToolRegistry registry;
        return registry;
    }

    /**
     * Register a tool factory
     */
    void registerTool(const std::string& name, ToolFactory factory) {
        tool_factories_[name] = std::move(factory);
    }

    /**
     * Create a tool instance by name
     */
    std::unique_ptr<Tool> getTool(const std::string& name) const {
        auto it = tool_factories_.find(name);
        if (it == tool_factories_.end()) {
            throw std::runtime_error("Tool not found: " + name);
        }
        return it->second();
    }

    /**
     * Check if a tool is registered
     */
    bool hasTool(const std::string& name) const {
        return tool_factories_.count(name) > 0;
    }

    /**
     * Get all registered tool names
     */
    std::vector<std::string> getToolNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : tool_factories_) {
            names.push_back(name);
        }
        return names;
    }

    /**
     * Register an output parser
     */
    void registerParser(const std::string& format,
                        std::unique_ptr<OutputParser> parser) {
        parsers_[format] = std::move(parser);
    }

    /**
     * Get a parser by format name
     */
    OutputParser* getParser(const std::string& format) const {
        auto it = parsers_.find(format);
        if (it == parsers_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    /**
     * Find a parser that can handle the given file
     */
    OutputParser* findParser(const fs::path& file) const {
        for (const auto& [_, parser] : parsers_) {
            if (parser->canParse(file)) {
                return parser.get();
            }
        }
        return nullptr;
    }

    /**
     * Register a testbench generator
     */
    void registerTestbench(const std::string& name,
                           std::unique_ptr<TestbenchGenerator> gen) {
        testbench_gens_[name] = std::move(gen);
    }

    /**
     * Get a testbench generator by name
     */
    TestbenchGenerator* getTestbench(const std::string& name) const {
        auto it = testbench_gens_.find(name);
        if (it == testbench_gens_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

private:
    ToolRegistry() = default;
    ToolRegistry(const ToolRegistry&) = delete;
    ToolRegistry& operator=(const ToolRegistry&) = delete;

    std::map<std::string, ToolFactory> tool_factories_;
    std::map<std::string, std::unique_ptr<OutputParser>> parsers_;
    std::map<std::string, std::unique_ptr<TestbenchGenerator>> testbench_gens_;
};

/**
 * Helper macro for auto-registration of tools
 *
 * Usage in your custom tool .cpp file:
 *   REGISTER_TOOL("mytool", MyCustomTool)
 */
#define REGISTER_TOOL(name, ToolClass) \
    namespace { \
        struct ToolClass##Registrar { \
            ToolClass##Registrar() { \
                tools::ToolRegistry::instance().registerTool(name, []() { \
                    return std::make_unique<ToolClass>(); \
                }); \
            } \
        }; \
        static ToolClass##Registrar ToolClass##_registrar; \
    }

} // namespace tools
