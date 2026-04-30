#include "filesystem.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct NullBuf : std::streambuf { int overflow(int c) override { return c; } };
struct SilenceCout {
    NullBuf nb;
    std::streambuf* old = std::cout.rdbuf(&nb);
    ~SilenceCout() { std::cout.rdbuf(old); }
};

std::string bytesToSafeName(const uint8_t* data, size_t size) {
    std::string s;
    s.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = data[i];
        if (c == '\0' || c == '\n' || c == '\r') continue;
        s.push_back(static_cast<char>(c));
    }
    if (s.empty()) s = "file";
    if (s.size() > 80) s.resize(80);
    return s;
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    SilenceCout quiet;
    std::string base = bytesToSafeName(data, size);
    FileSystem fs;

    const std::vector<std::string> allowed = {base + ".txt", base + ".TXT", base + ".csv", base + ".CSV"};
    for (const auto& name : allowed) {
        FileSystem local;
        bool ok = local.createFile(name, "a,b\n1,2\n");
        if (!ok) __builtin_trap(); // allowed extensions should be accepted when filename is otherwise valid
    }

    const std::vector<std::string> blocked = {
        base,
        base + ".",
        ".hidden",
        base + ".json",
        base + ".png",
        base + ".html",
        base + ".csv.exe",
        base + ".txt.bak",
        "../" + base + ".txt",
        base + "/nested.txt",
        base + "\\nested.txt"
    };

    for (const auto& name : blocked) {
        FileSystem local;
        bool ok = local.createFile(name, "content");
        if (ok) __builtin_trap(); // unsupported, path-like, or extensionless names must not be accepted
    }

    return 0;
}
