#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    CoutSilencer silence;
    FuzzInput in(data, size);
    FileSystem fs;
    seedUsers(fs);

    fs.makeDirectory("box");
    fs.chownDir("box", "alice", "dev");
    fs.chmodDir("box", permDigit(in), permDigit(in), permDigit(in));

    const char* users[] = {"root", "alice", "bob", "eve"};
    fs.switchUser(users[in.small(4)]);

    uint8_t dirBits = fs.effectiveDirBitsQuery("box");
    bool canTraverse = (dirBits & PERM_DELETE) != 0;
    bool cdResult = fs.changeDirectory("box");

    if (!fs.currentUser().isRoot && !canTraverse && cdResult) __builtin_trap();

    if (cdResult) {
        bool canWriteInside = (fs.effectiveDirBitsQuery(".") & PERM_WRITE) != 0; // returns 0 for non-child, mainly mutation coverage
        (void)canWriteInside;
        fs.createTextFile("inside.txt", in.content(64));
        fs.listDirectory();
        fs.changeDirectory("..");
    }

    bool chmoddResult = fs.chmodDir("box", permDigit(in), permDigit(in), permDigit(in));
    bool isDirOwnerOrRoot = fs.currentUser().isRoot || fs.currentUserName() == "alice";
    if (!isDirOwnerOrRoot && chmoddResult) __builtin_trap();
    return 0;
}
