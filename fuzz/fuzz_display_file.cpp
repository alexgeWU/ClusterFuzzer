#include <stdint.h>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#include "filesystem.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    FuzzedDataProvider fdp(data, size);
    FileSystem fs;

    std::string filename = fdp.ConsumeRandomLengthString(64);
    std::string content  = fdp.ConsumeRandomLengthString(256);
    std::string realPw   = fdp.ConsumeRandomLengthString(32);
    std::string guessPw  = fdp.ConsumeRandomLengthString(32);

    fs.createFile(filename, content, realPw);
    fs.displayFile(filename, guessPw);  // fuzz the auth check
    return 0;
}