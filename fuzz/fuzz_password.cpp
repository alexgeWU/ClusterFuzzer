#include <stdint.h>
#include <stddef.h>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#include "filesystem.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    FuzzedDataProvider fdp(data, size);

    std::string filename = fdp.ConsumeRandomLengthString(64);
    std::string content  = fdp.ConsumeRandomLengthString(256);
    std::string pw1      = fdp.ConsumeRandomLengthString(64);
    std::string pw2      = fdp.ConsumeRandomLengthString(64);
    std::string pw3      = fdp.ConsumeRandomLengthString(64);
    std::string wrongPw  = fdp.ConsumeRandomLengthString(64);

    if (filename.empty()) return 0;

    FileSystem fs;

    // ── Scenario 1: set password on non-existent file ─────────────────────
    fs.setPassword(filename, pw1, pw2);       // should fail gracefully

    // ── Scenario 2: create file with initial password ─────────────────────
    fs.createFile(filename, content, pw1);

    // ── Scenario 3: wrong old password — must fail, not crash ─────────────
    fs.setPassword(filename, wrongPw, pw2);

    // ── Scenario 4: correct old password — should succeed ─────────────────
    fs.setPassword(filename, pw1, pw2);

    // ── Scenario 5: chain another password change ─────────────────────────
    fs.setPassword(filename, pw2, pw3);

    // ── Scenario 6: empty new password (equivalent to removePassword) ─────
    fs.setPassword(filename, pw3, "");

    // ── Scenario 7: removePassword on an unlocked file ───────────────────
    fs.removePassword(filename, "");

    // ── Scenario 8: removePassword with wrong password on re-locked file ──
    fs.setPassword(filename, "", pw1);
    fs.removePassword(filename, wrongPw);     // should fail, not crash

    return 0;
}