// fuzz_csv_render.cpp
//
// Target: parseCsvRow → parseCsv → displayCsvTable (called indirectly via
//         FileSystem::displayFile on a .csv file).
//
// Security properties verified:
//   1. No crash / heap-overflow / stack-overflow for any CSV byte sequence.
//   2. The column-width accumulator (std::max loop) never produces a value
//      that causes std::string(w+2, '-') to allocate an unreasonable buffer —
//      checked implicitly because the process would OOM/crash.
//   3. Unterminated quoted fields (inQuotes never reset) do not cause the
//      parser to read past the end of the string.
//   4. Embedded NUL bytes, lone CR, CRLF, and LF-only line endings all
//      survive without memory errors.
//   5. Ragged rows (varying column counts) are handled without out-of-bounds
//      access in the rendering loop.

#include "filesystem.h"
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;

    std::string csvContent(reinterpret_cast<const char*>(data), size);

    FileSystem fs;

    // Create the CSV file (no password so displayFile always proceeds to
    // the rendering path rather than bailing on auth).
    bool ok = fs.createCsvFile("fuzz_data.csv", csvContent);
    if (!ok) return 0;  // filename rejected — nothing to exercise

    // This triggers parseCsv → displayCsvTable with our raw fuzz content.
    fs.displayFile("fuzz_data.csv");

    // Also exercise edit → re-display to ensure re-parsing is stable.
    fs.editFile("fuzz_data.csv", csvContent + "\nextra,row,appended\n");
    fs.displayFile("fuzz_data.csv");

    return 0;
}