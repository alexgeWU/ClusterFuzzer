#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    CoutSilencer silence;
    FuzzInput in(data, size);
    FileSystem fs;
    seedUsers(fs);

    fs.createTextFile("target.txt", "content");
    fs.chown("target.txt", "alice", "dev");

    const char* users[] = {"root", "alice", "bob", "eve"};
    fs.switchUser(users[in.small(4)]);

    bool isOwnerOrRoot = fs.currentUser().isRoot || fs.currentUserName() == "alice";
    bool chmodResult = fs.chmod("target.txt", permDigit(in), permDigit(in), permDigit(in));
    bool chownResult = fs.chown("target.txt", users[in.small(4)], in.coin() ? "dev" : "red");

    // Only owner/root should chmod; only root should chown.
    if (!isOwnerOrRoot && chmodResult) __builtin_trap();
    if (!fs.currentUser().isRoot && chownResult) __builtin_trap();

    fs.displayFile("target.txt", "");
    fs.editFile("target.txt", in.content(80), "");
    return 0;
}
