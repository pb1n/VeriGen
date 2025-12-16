#include "../counter_gen.hpp"
#include <iostream>

int main() {
    std::cout << "Test starting..." << std::endl;

    auto counter1 = generators::generateCounter("test_counter", 8, true);

    std::cout << "Counter generated successfully!" << std::endl;
    std::cout << counter1->emit() << std::endl;

    return 0;
}