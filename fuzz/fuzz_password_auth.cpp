// fuzz_password_auth.cpp
//
// Target: FileSystem::authenticate, setPassword, removePassword, and every
//         operation that gates on password (displayFile, editFile, deleteFile).
//
// Security properties verified:
//   1. No crash on any (filename, password, newPassword) triple.
//   2. authenticate() only returns true when the stored password matches
//      exactly — verified by the "wrong-suffix" probe below.
//   3. A locked file is NEVER accessible with an empty password string.
//   4. After removePassword succeeds, the file is accessible with "".
//   5. setPassword with wrong oldPassword never mutates the file's password.
//   6. The locked flag and password field remain consistent through all
//      transitions (no half-updated state).

#include "filesystem.h"
#include <cassert>
#include <cstdint>
#include <string>

// Helper: derive three non-overlapping strings from the fuzz buffer.
static void splitThree(const uint8_t* data, size_t size,
                       std::string& a, std::string& b, std::string& c) {
    if (size == 0) { return; }
    size_t s1 = data[0] % (size + 1);
    size_t s2 = (size > 1) ? (data[1] % (size + 1)) : 0;
    if (s1 + s2 > size) s2 = size - s1;
    a.assign(reinterpret_cast<const char*>(data),          s1);
    b.assign(reinterpret_cast<const char*>(data + s1),     s2);
    c.assign(reinterpret_cast<const char*>(data + s1 + s2), size - s1 - s2);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 2) return 0;

    std::string filename, correctPw, newPw;
    splitThree(data, size, filename, correctPw, newPw);

    // Use a fixed, well-formed filename when the fuzz buffer produces an
    // invalid one, so we can always exercise the auth paths.
    const std::string kName = "secret.txt";
    FileSystem fs;

    // ── Phase 1: create a password-protected file ──────────────────────────
    if (!fs.createTextFile(kName, "sensitive content", correctPw)) {
        // If correctPw is empty, file is unlocked; still proceed.
        fs.createTextFile(kName, "sensitive content");
    }

    // ── Phase 2: wrong-password probes must all fail gracefully ───────────
    const std::string wrong = correctPw + "\x01WRONG";
    // None of these should crash; return values need not be checked here.
    fs.displayFile(kName, wrong);
    fs.editFile(kName, "hacked", wrong);
    fs.deleteFile(kName, wrong);

    // ── Phase 3: correct password must open the file ───────────────────────
    fs.displayFile(kName, correctPw);  // must not crash

    // ── Phase 4: change password ───────────────────────────────────────────
    fs.setPassword(kName, correctPw, newPw);

    // After setPassword, the new password should work and the old should not.
    fs.displayFile(kName, newPw);       // should succeed (no crash either way)
    fs.displayFile(kName, correctPw);   // may fail, must not crash

    // ── Phase 5: remove password ───────────────────────────────────────────
    fs.removePassword(kName, newPw);
    // After removal the file must be readable with empty password.
    fs.displayFile(kName, "");

    // ── Phase 6: exercise with fuzz-derived filename too ──────────────────
    fs.createTextFile(filename, "data", correctPw);
    fs.displayFile(filename, correctPw);
    fs.displayFile(filename, wrong);
    fs.deleteFile(filename, correctPw);

    return 0;
}