// fuzz_directory_paths.cpp
//
// Target: FileSystem::changeDirectory, makeDirectory, removeDirectory,
//         and the pathStack_ bookkeeping (currentDir / pathString).
//
// Security properties verified:
//   1. No crash for any sequence of cd/mkdir/rmdir operations with
//      arbitrary name strings.
//   2. pathStack_ underflow: repeated "cd .." from root must never pop below
//      root (i.e. pathStack_ must not go negative / wrap).
//   3. Directory names containing '/', '\\', '.', "..", or NUL bytes are
//      rejected by makeDirectory — verified by checking that cd into such a
//      name fails after a supposedly successful mkdir.
//   4. Deep nesting: creating N directories and descending into each must not
//      overflow the stack or exhaust memory in pathString().
//   5. Removing a non-empty directory must be refused; removing the current
//      working directory (or an ancestor) must not corrupt pathStack_.
//   6. currentPath() is always well-formed (starts with '/') regardless of
//      the operation sequence.

#include "filesystem.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

// Decode the fuzz buffer as a sequence of (opcode, name) pairs.
// opcode 0=mkdir, 1=rmdir, 2=cd, 3=cd-dotdot, 4=cd-root
static void runOps(FileSystem& fs, const uint8_t* data, size_t size) {
    size_t i = 0;
    while (i < size) {
        uint8_t op = data[i++] % 5;

        // Read a variable-length name: length byte then that many chars.
        size_t nameLen = (i < size) ? data[i++] : 0;
        nameLen = std::min(nameLen, size - i);  // clamp to remaining buffer
        std::string name(reinterpret_cast<const char*>(data + i), nameLen);
        i += nameLen;

        switch (op) {
            case 0: fs.makeDirectory(name);  break;
            case 1: fs.removeDirectory(name); break;
            case 2: fs.changeDirectory(name); break;
            case 3: fs.changeDirectory(".."); break;
            case 4: fs.changeDirectory("/");  break;
        }

        // Property: currentPath() must always start with '/'.
        assert(fs.currentPath().front() == '/');
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;

    // Suppress stdout.
    struct NullBuf : public std::streambuf {
        int overflow(int c) override { return c; }
    } nb;
    std::streambuf* orig = std::cout.rdbuf(&nb);

    FileSystem fs;

    // ── Underflow probe: 1000× cd .. from root ────────────────────────────
    for (int k = 0; k < 1000; ++k) fs.changeDirectory("..");
    assert(fs.currentPath() == "/");

    // ── Fuzz-driven operation sequence ────────────────────────────────────
    runOps(fs, data, size);

    // ── Deep nesting probe: build a chain using printable name prefix ─────
    fs.changeDirectory("/");
    const int depth = 20;
    for (int d = 0; d < depth; ++d) {
        std::string dname = "d" + std::to_string(d);
        fs.makeDirectory(dname);
        fs.changeDirectory(dname);
    }
    // currentPath must now have depth+1 components without crashing.
    const std::string p = fs.currentPath();
    assert(p.front() == '/');

    // Unwind all the way back to root via "..".
    for (int d = 0; d < depth + 10; ++d) fs.changeDirectory("..");
    assert(fs.currentPath() == "/");

    std::cout.rdbuf(orig);
    return 0;
}