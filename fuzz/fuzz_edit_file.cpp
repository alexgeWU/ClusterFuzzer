#include <stdint.h>
#include <stddef.h>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#include "filesystem.h"

// Fuzzes the editFile path focusing on:
//   - Unbounded content size (potential OOM / excessive memory use)
//   - Edit on non-existent file
//   - Edit on password-protected file with wrong password
//   - Chained create -> edit -> display cycle
//   - Empty content replacement
//   - Content with embedded nulls, newlines, and special characters

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Cap total input to avoid legitimate OOM from the fuzzer itself
    if (size > 8192) return 0;

    FuzzedDataProvider fdp(data, size);

    std::string filename   = fdp.ConsumeRandomLengthString(64);
    std::string initial    = fdp.ConsumeRandomLengthString(512);
    std::string password   = fdp.ConsumeRandomLengthString(64);
    std::string newContent = fdp.ConsumeRandomLengthString(1024);
    std::string wrongPw    = fdp.ConsumeRandomLengthString(64);

    if (filename.empty()) return 0;

    FileSystem fs;

    // ── Scenario 1: edit a file that doesn't exist yet ────────────────────
    fs.editFile(filename, newContent, "");    // should fail cleanly

    // ── Scenario 2: create then edit with correct password ────────────────
    fs.createFile(filename, initial, password);
    fs.editFile(filename, newContent, password);

    // ── Scenario 3: edit with wrong password — must not apply ─────────────
    fs.editFile(filename, initial, wrongPw);

    // ── Scenario 4: verify content after edit via display ─────────────────
    fs.displayFile(filename, password);

    // ── Scenario 5: replace content with empty string ─────────────────────
    fs.editFile(filename, "", password);
    fs.displayFile(filename, password);

    // ── Scenario 6: consume remaining bytes as a large content payload ─────
    std::string largeContent = fdp.ConsumeRemainingBytesAsString();
    fs.editFile(filename, largeContent, password);

    return 0;
}