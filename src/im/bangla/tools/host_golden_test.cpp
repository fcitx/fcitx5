/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../avro_parser.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct GoldenCase {
    const char *input;
    const char *expected;
};

int runGoldenTests() {
    const std::vector<GoldenCase> cases = {
        {"ami banglay gan gai", "আমি বাংলায় গান গাই"},
        {"amader valObasa", "আমাদের ভালোবাসা"},
        {"bhalo", "ভাল"},
        {"bhalO", "ভালো"},
        {"mukh", "মুখ"},
        {"OI", "ঐ"},
        {"OU", "ঔ"},
        {"rry", "রর‍্য"},
        {"t``", "ৎ"},
        {",,", "্‌"},
        {"x", "এক্স"},
        {"kw", "ক্ব"},
        {"ky", "ক্য"},
        {"123", "১২৩"},
    };

    fcitx::AvroParser parser;
    int failures = 0;
    for (const auto &testCase : cases) {
        const std::string input(testCase.input);
        const std::string expected(testCase.expected);
        const std::string actual = parser.parse(input);
        if (actual != expected) {
            ++failures;
            std::cerr << "FAIL: \"" << input << "\"\n"
                      << "  expected: " << expected << "\n"
                      << "  actual:   " << actual << "\n";
        } else {
            std::cout << "PASS: \"" << input << "\" -> " << actual << "\n";
        }
    }
    return failures;
}

} // namespace

int main() {
    const int failures = runGoldenTests();
    if (failures == 0) {
        std::cout << "All golden tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cerr << failures << " golden test(s) failed.\n";
    return EXIT_FAILURE;
}
