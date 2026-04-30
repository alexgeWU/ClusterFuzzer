#include "filesystem.h"
#include <cstdint>
#include <cstring>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;

    // Split the fuzz buffer into (filename | content | password) using the
    // first two bytes as split offsets so the fuzzer can explore all ratios.
    const size_t split1 = data[0] % (size);
    const size_t split2 = (size > 1) ? (data[1] % (size)) : split1;

    const size_t lo = std::min(split1, split2);
    const size_t hi = std::max(split1, split2);

    std::string filename(reinterpret_cast<const char*>(data),          lo);
    std::string content (reinterpret_cast<const char*>(data + lo),     hi - lo);
    std::string password(reinterpret_cast<const char*>(data + hi),     size - hi);

    FileSystem fs;
    bool created = fs.createFile(filename, content, password);

    // Property: if creation succeeded the filesystem must be able to list
    // without crashing, and the extension must be supported.
    if (created) {
        fs.listDirectory();

        // Verify the file is retrievable via displayFile (no crash).
        fs.displayFile(filename, password);

        // Verify locked/password consistency: displayFile with wrong password
        // must not crash regardless of outcome.
        fs.displayFile(filename, password + "WRONG");

        // Attempting to create the same file again must not crash.
        fs.createFile(filename, content, password);
    }

    return 0;
}