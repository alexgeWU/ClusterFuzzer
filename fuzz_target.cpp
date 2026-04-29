#include <stdint.h>
#include <stddef.h>
#include "main.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Call your code with the fuzzer-provided input
    std::string input(reinterpret_cast<const char*>(data), size);

    printText(input);
    return 0;
}
