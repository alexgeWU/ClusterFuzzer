#include "filesystem.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace {
struct NullBuf : std::streambuf { int overflow(int c) override { return c; } };
struct SilenceCout { NullBuf nb; std::streambuf* old = std::cout.rdbuf(&nb); ~SilenceCout(){ std::cout.rdbuf(old); } };
std::string cappedBytes(const uint8_t* data, size_t size, size_t max = 4096) {
    size = size > max ? max : size;
    return std::string(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + size);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    SilenceCout quiet;
    FileSystem fs;
    std::string content = cappedBytes(data, size);

    if (!fs.createTextFile("note.txt", content)) __builtin_trap();
    if (!fs.displayFile("note.txt")) __builtin_trap();
    if (!fs.editFile("note.txt", content + "\nedit")) __builtin_trap();
    if (!fs.deleteFile("note.txt")) __builtin_trap();
    if (fs.displayFile("note.txt")) __builtin_trap();

    if (!fs.createCsvFile("table.csv", content)) __builtin_trap();
    if (!fs.displayFile("table.csv")) __builtin_trap();
    if (!fs.editFile("table.csv", "h1,h2\n" + content)) __builtin_trap();
    if (!fs.deleteFile("table.csv")) __builtin_trap();
    if (fs.displayFile("table.csv")) __builtin_trap();

    return 0;
}
