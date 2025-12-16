#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace testing {

/**
 * Represents expected values at specific time steps for simulation verification
 */
struct GoldenValue {
    int time_step;        // Simulation time step
    uint64_t expected;    // Expected value at this time step
    std::string signal;   // Signal name (e.g., "count", "result")
};

/**
 * Container for golden values for a specific test case
 */
struct GoldenValueSet {
    std::string test_name;
    std::vector<GoldenValue> values;

    GoldenValueSet(const std::string& name) : test_name(name) {}

    void addValue(int time_step, uint64_t expected, const std::string& signal = "count") {
        values.push_back({time_step, expected, signal});
    }
};

/**
 * Golden value generator for basic counter (with reset)
 * TODO: Fill in expected values based on counter behavior
 */
inline GoldenValueSet getBasicCounterGoldenValues() {
    GoldenValueSet golden("basic_counter");

    // TODO: Add golden values for each time step
    // Example format:
    // golden.addValue(time_step, expected_value, "count");

    // Time 0: Reset active, count = 0
    // golden.addValue(0, 0, "count");

    // Time 1: After first clock, count = 1
    // golden.addValue(1, 1, "count");

    return golden;
}

/**
 * Golden value generator for simple counter (no reset)
 * TODO: Fill in expected values
 */
inline GoldenValueSet getSimpleCounterGoldenValues() {
    GoldenValueSet golden("simple_counter");

    // TODO: Add golden values

    return golden;
}

/**
 * Golden value generator for loadable counter (with reset and load)
 * TODO: Fill in expected values
 */
inline GoldenValueSet getLoadableCounterGoldenValues() {
    GoldenValueSet golden("loadable_counter");

    // TODO: Add golden values
    // Consider load_enable signal behavior

    return golden;
}

/**
 * Golden value generator for loadable counter (no reset, with load)
 * TODO: Fill in expected values
 */
inline GoldenValueSet getLoadableNoResetGoldenValues() {
    GoldenValueSet golden("loadable_no_reset");

    // TODO: Add golden values

    return golden;
}

/**
 * Golden value generator for down counter (with reset and load)
 * TODO: Fill in expected values
 */
inline GoldenValueSet getDownCounterGoldenValues() {
    GoldenValueSet golden("down_counter");

    // TODO: Add golden values
    // Remember: this counts down instead of up

    return golden;
}

/**
 * Golden value generator for simple down counter (no reset, no load)
 * TODO: Fill in expected values
 */
inline GoldenValueSet getSimpleDownGoldenValues() {
    GoldenValueSet golden("simple_down");

    // TODO: Add golden values

    return golden;
}

/**
 * Verify simulation results against golden values
 * TODO: Implement comparison logic
 *
 * @param actual Actual simulation output
 * @param golden Expected golden values
 * @return true if all values match, false otherwise
 */
inline bool verifyGoldenValues(const std::string& actual, const GoldenValueSet& golden) {
    // TODO: Parse actual output and compare against golden values
    // Parse VCD or log format from simulator
    // Compare each time step
    // Return true if all match, false if any mismatch

    // Skeleton implementation:
    bool all_match = true;

    // TODO: Parse actual output
    // TODO: For each golden value, check if actual matches
    // TODO: Print mismatches for debugging

    return all_match;
}

} // namespace testing
