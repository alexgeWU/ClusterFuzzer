#include <stdint.h>
#include <stddef.h>
#include <string>
#include "lib.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    std::string input(reinterpret_cast<const char*>(data), size);

    // Call the functions from lib.h you want fuzzed
    printText(input);

    return 0;
}