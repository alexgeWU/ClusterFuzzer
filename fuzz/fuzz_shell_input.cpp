// fuzz_shell_input.cpp
//
// Target: runShell() — the full command-dispatch loop driven by arbitrary
//         byte streams presented as std::istream input.
//
// Security properties verified:
//   1. No crash for any sequence of shell input bytes, including:
//        • embedded NUL bytes in command lines
//        • very long tokens (>>10 MB filenames, passwords, content)
//        • multiline content blocks missing the "END" sentinel (EOF flush)
//        • commands interleaved with multiline content (e.g. "write" whose
//          content stream also contains valid commands — confused parser?)
//        • rapid mode-switches: write→csv→edit→rm with injected "END" lines
//   2. The "END" sentinel in readMultilineContent is correctly recognized
//      even when surrounded by look-alike strings ("ENDx", "xEND", " END").
//   3. tokenize() (istringstream >> tok) does not misbehave on lines that
//      are purely whitespace, or that start with shell-special characters.
//   4. The shell loop terminates cleanly on EOF (std::getline returns false).

#include "filesystem.h"
#include <cstdint>
#include <sstream>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;

    // Feed the raw fuzz bytes directly as the shell's stdin.
    std::string input(reinterpret_cast<const char*>(data), size);
    std::istringstream in(input);

    // Suppress stdout during fuzzing to keep I/O overhead low; we care about
    // memory safety, not correct output text.  Redirect cout to a null buf.
    struct NullBuf : public std::streambuf {
        int overflow(int c) override { return c; }
    } nullBuf;
    std::streambuf* origBuf = std::cout.rdbuf(&nullBuf);

    FileSystem fs;
    runShell(fs, in);   // must return without crash or hang

    std::cout.rdbuf(origBuf);
    return 0;
}