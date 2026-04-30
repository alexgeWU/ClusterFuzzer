#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sstream>
#include "filesystem.h"



extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Limit input size to avoid OOM from enormous command sequences
    if (size > 4096) return 0;

    std::string input(reinterpret_cast<const char*>(data), size);
    std::istringstream stream(input);

    FileSystem fs;

    // Suppress stdout during fuzzing to avoid I/O overhead masking crashes
    // runShell reads from stream and exits on EOF
    runShell(fs, stream);

    return 0;
}