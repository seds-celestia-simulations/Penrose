#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace viz::test {

struct TestCase {
    std::string name;
    std::function<bool()> fn;
};

inline int run_all(const std::vector<TestCase>& tests) {
    int failed = 0;
    for (const TestCase& test : tests) {
        const bool ok = test.fn();
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << test.name << "\n";
        if (!ok) {
            ++failed;
        }
    }
    std::cout << (failed == 0 ? "All tests passed.\n" : "Some tests failed.\n");
    return failed;
}

} // namespace viz::test
