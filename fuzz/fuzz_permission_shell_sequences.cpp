#include "fuzz_common.h"

static std::string pickCommand(FuzzInput& in) {
    static const char* cmds[] = {
        "useradd alice dev", "useradd bob dev", "su alice", "su bob", "su root",
        "touch a.txt", "touch b.csv", "mkdir d", "chmod 000 a.txt", "chmod 777 a.txt",
        "chmod 640 a.txt", "chown alice:dev a.txt", "cat a.txt", "edit a.txt\nEND",
        "rm a.txt", "chmodd 000 d", "chmodd 777 d", "chownd alice:dev d", "cd d", "cd ..", "ls"
    };
    if (in.coin()) return cmds[in.small(sizeof(cmds) / sizeof(cmds[0]))];
    return in.token(32);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    CoutSilencer silence;
    FuzzInput in(data, size);
    FileSystem fs;

    std::string script;
    for (int i = 0; i < 40 && !in.empty(); ++i) {
        script += pickCommand(in);
        script += '\n';
        if (in.coin()) {
            script += in.content(64);
            script += "\nEND\n";
        }
    }
    script += "exit\n";
    std::istringstream input(script);
    runShell(fs, input);
    return 0;
}
