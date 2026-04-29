#include <stdint.h>
#include <stddef.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Call your code with the fuzzer-provided input
    if (size > 0 && data[0] == 'A') {
        __builtin_trap();
    }
    return 0;
}