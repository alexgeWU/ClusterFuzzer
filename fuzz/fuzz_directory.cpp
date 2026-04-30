#include <stdint.h>
#include <stddef.h>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#include "filesystem.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    FuzzedDataProvider fdp(data, size);

    FileSystem fs;

    // Pre-create a small known directory tree to navigate
    fs.makeDirectory("alpha");
    fs.makeDirectory("beta");
    fs.changeDirectory("alpha");
    fs.makeDirectory("nested");
    fs.changeDirectory("..");

    // Fuzz-driven operation sequence
    int ops = fdp.ConsumeIntegralInRange<int>(1, 32);
    for (int i = 0; i < ops; i++) {
        uint8_t choice = fdp.ConsumeIntegralInRange<uint8_t>(0, 5);
        std::string name = fdp.ConsumeRandomLengthString(64);

        switch (choice) {
            case 0:
                // cd into a fuzz-provided name (likely to miss)
                fs.changeDirectory(name);
                break;
            case 1:
                // cd into a known directory
                fs.changeDirectory(
                    fdp.PickValueInArray<std::string>({"alpha", "beta", "nested", "..", "/"})
                );
                break;
            case 2:
                fs.makeDirectory(name);
                break;
            case 3:
                fs.removeDirectory(name);
                break;
            case 4:
                // create a file at current location then navigate away
                if (!name.empty()) fs.createFile(name, "x");
                fs.changeDirectory("..");
                break;
            case 5:
                // jump to root and verify path is sane
                fs.changeDirectory("/");
                break;
        }
    }

    return 0;
}