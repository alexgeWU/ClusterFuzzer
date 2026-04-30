#include "filesystem.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace {
struct NullBuf : std::streambuf { int overflow(int c) override { return c; } };
struct SilenceCout { NullBuf nb; std::streambuf* old = std::cout.rdbuf(&nb); ~SilenceCout(){ std::cout.rdbuf(old); } };
std::string slice(const uint8_t* data, size_t begin, size_t end) {
    if (begin > end) begin = end;
    return std::string(reinterpret_cast<const char*>(data + begin), reinterpret_cast<const char*>(data + end));
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    SilenceCout quiet;
    FileSystem fs;
    const std::string good = "correct-password";
    const std::string bad = "wrong-password";
    std::string content = slice(data, 0, size > 4096 ? 4096 : size);

    bool created = fs.createCsvFile("secret.csv", content, good);
    if (!created) __builtin_trap();

    if (fs.displayFile("secret.csv", bad)) __builtin_trap();
    if (fs.editFile("secret.csv", "tampered", bad)) __builtin_trap();
    if (fs.deleteFile("secret.csv", bad)) __builtin_trap();

    if (!fs.displayFile("secret.csv", good)) __builtin_trap();
    if (!fs.editFile("secret.csv", content + "\nextra,row", good)) __builtin_trap();
    if (!fs.setPassword("secret.csv", good, "new-password")) __builtin_trap();
    if (fs.displayFile("secret.csv", good)) __builtin_trap();
    if (!fs.displayFile("secret.csv", "new-password")) __builtin_trap();
    if (!fs.removePassword("secret.csv", "new-password")) __builtin_trap();
    if (!fs.displayFile("secret.csv")) __builtin_trap();

    return 0;
}
