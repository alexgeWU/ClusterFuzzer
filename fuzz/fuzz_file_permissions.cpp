#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    CoutSilencer silence;
    FuzzInput in(data, size);
    FileSystem fs;
    seedUsers(fs);

    std::string pw = in.token(16);
    fs.createTextFile("owned.txt", "root-owned content", pw);
    fs.chmod("owned.txt", permDigit(in), permDigit(in), permDigit(in));
    fs.chown("owned.txt", in.coin() ? "alice" : "bob", in.coin() ? "dev" : "red");

    const char* users[] = {"root", "alice", "bob", "eve", "missing"};
    fs.switchUser(users[in.small(5)]);

    uint8_t bits = fs.effectiveFileBits("owned.txt");
    bool canR = (bits & PERM_READ) != 0;
    bool canW = (bits & PERM_WRITE) != 0;
    bool canD = (bits & PERM_DELETE) != 0;

    std::string attemptPw = in.coin() ? pw : in.token(16);
    bool readResult = fs.displayFile("owned.txt", attemptPw);
    bool editResult = fs.editFile("owned.txt", in.content(64), attemptPw);
    bool deleteResult = fs.deleteFile("owned.txt", attemptPw);

    // Permission invariant: a non-root user must not succeed without the required bit.
    if (!fs.currentUser().isRoot) {
        if (!canR && readResult) __builtin_trap();
        if (!canW && editResult) __builtin_trap();
        if (!canD && deleteResult) __builtin_trap();
    }
    return 0;
}

//hello