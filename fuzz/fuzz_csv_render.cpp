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
    std::string csv = cappedBytes(data, size);

    // Exercise CSV parser/table renderer with arbitrary bytes, quotes, uneven rows, CRLFs, huge cells, etc.
    fs.createCsvFile("data.csv", csv);
    fs.displayFile("data.csv");

    // Also verify the same content as .txt does not enter CSV table rendering paths.
    fs.createTextFile("data.txt", csv);
    fs.displayFile("data.txt");
    return 0;
}
