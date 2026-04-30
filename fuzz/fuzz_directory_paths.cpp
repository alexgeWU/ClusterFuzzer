#include "filesystem.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace {
struct NullBuf : std::streambuf { int overflow(int c) override { return c; } };
struct SilenceCout { NullBuf nb; std::streambuf* old = std::cout.rdbuf(&nb); ~SilenceCout(){ std::cout.rdbuf(old); } };
std::string safeDir(const uint8_t* data, size_t size) {
    std::string s;
    for (size_t i = 0; i < size && s.size() < 40; ++i) {
        char c = static_cast<char>(data[i]);
        if (c == '\0' || c == '\n' || c == '\r') continue;
        s.push_back(c);
    }
    if (s.empty()) s = "dir";
    return s;
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    SilenceCout quiet;
    FileSystem fs;
    std::string d = safeDir(data, size);

    fs.makeDirectory(d);
    fs.changeDirectory(d);
    fs.createFile("inside.txt", "hello");
    fs.createCsvFile("inside.csv", "a,b\n1,2");
    fs.listDirectory();
    fs.changeDirectory("..");

    // Directories should not accept traversal/path separators/extensions.
    if (fs.makeDirectory("..")) __builtin_trap();
    if (fs.makeDirectory(".")) __builtin_trap();
    if (fs.makeDirectory("a/b")) __builtin_trap();
    if (fs.makeDirectory("a\\b")) __builtin_trap();
    if (fs.makeDirectory("name.txt")) __builtin_trap();

    return 0;
}
