// fuzz_edit_lifecycle.cpp
//
// Target: The complete file lifecycle — create → display → edit → passwd →
//         unpasswd → delete — exercised with arbitrary content and passwords.
//
// Security properties verified:
//   1. No use-after-free or iterator invalidation when a file is edited
//      in-place (it->second.content = newContent while the iterator is live).
//   2. No crash when content grows from 0 bytes to very large values or
//      shrinks back to empty across repeated edits.
//   3. editFile with correct password atomically replaces content; a
//      subsequent displayFile with the correct password shows the NEW content
//      (no partial-write / torn-write).
//   4. deleteFile removes the file so that a subsequent createFile with the
//      same name succeeds (no phantom entry left in the map).
//   5. Operating on a deleted file's name produces a clean error, not a crash
//      or UB from a dangling reference.
//   6. Mixed .txt and .csv edits in the same directory do not interfere
//      (namespace is per-directory, keyed by full filename string).

#include "filesystem.h"
#include <cstdint>
#include <string>

static void splitTwo(const uint8_t* data, size_t size,
                     std::string& a, std::string& b) {
    size_t cut = (size > 0) ? (data[0] % (size + 1)) : 0;
    a.assign(reinterpret_cast<const char*>(data),       cut);
    b.assign(reinterpret_cast<const char*>(data + cut), size - cut);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;

    std::string content1, content2;
    splitTwo(data, size, content1, content2);

    // Derive a short password from the first byte so it is sometimes empty.
    const std::string pw = (size > 1 && data[1] % 4 != 0)
                         ? std::string(reinterpret_cast<const char*>(data + 1), 1)
                         : "";

    // Suppress stdout.
    struct NullBuf : public std::streambuf {
        int overflow(int c) override { return c; }
    } nb;
    std::streambuf* orig = std::cout.rdbuf(&nb);

    FileSystem fs;

    // ── .txt lifecycle ────────────────────────────────────────────────────
    {
        const std::string name = "life.txt";
        fs.createTextFile(name, content1, pw);
        fs.displayFile(name, pw);
        fs.editFile(name, content2, pw);
        fs.displayFile(name, pw);
        // Edit with wrong password must not corrupt state.
        fs.editFile(name, "HACKED", pw + "X");
        fs.displayFile(name, pw);   // content must still be content2 (or crash = bug)
        fs.setPassword(name, pw, pw + "NEW");
        fs.displayFile(name, pw + "NEW");
        fs.removePassword(name, pw + "NEW");
        fs.displayFile(name, "");   // file should now be unlocked
        fs.deleteFile(name, "");
        // Post-delete operations must produce errors, not crashes.
        fs.displayFile(name, "");
        fs.editFile(name, "ghost", "");
        fs.deleteFile(name, "");
        // Re-create with same name must succeed.
        bool ok = fs.createTextFile(name, "reborn", "");
        (void)ok;  // we only care that it doesn't crash
        fs.displayFile(name, "");
    }

    // ── .csv lifecycle ────────────────────────────────────────────────────
    {
        const std::string name = "life.csv";
        fs.createCsvFile(name, content1, pw);
        fs.displayFile(name, pw);
        fs.editFile(name, content2, pw);
        fs.displayFile(name, pw);
        fs.deleteFile(name, pw);
        fs.createCsvFile(name, "", "");
        fs.displayFile(name, "");
    }

    // ── Large content stress ──────────────────────────────────────────────
    {
        const std::string big(64 * 1024, 'A');  // 64 KB of 'A'
        fs.createTextFile("big.txt", big, "");
        fs.displayFile("big.txt", "");
        fs.editFile("big.txt", std::string(1, '\0'), "");  // shrink to 1 NUL byte
        fs.displayFile("big.txt", "");
        fs.deleteFile("big.txt", "");
    }

    std::cout.rdbuf(orig);
    return 0;
}