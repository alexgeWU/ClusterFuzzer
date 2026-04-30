#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sstream>
#include "filesystem.h"



extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Limit input size to avoid enormous command sequences
    if (size > 4096) return 0;

    std::string input(reinterpret_cast<const char*>(data), size);
    std::istringstream stream(input);

    FileSystem fs;

    runShell(fs, stream);

    return 0;
}