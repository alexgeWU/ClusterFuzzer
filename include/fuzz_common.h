#pragma once

#include "filesystem.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class FuzzInput {
public:
    FuzzInput(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    bool empty() const { return off_ >= size_; }

    uint8_t byte(uint8_t fallback = 0) {
        if (off_ >= size_) return fallback;
        return data_[off_++];
    }

    bool coin() { return (byte() & 1U) != 0; }

    uint8_t small(uint8_t mod, uint8_t fallback = 0) {
        if (mod == 0) return fallback;
        return static_cast<uint8_t>(byte(fallback) % mod);
    }

    std::string bytes(size_t maxLen) {
        if (off_ >= size_) return {};
        size_t n = std::min(maxLen, size_ - off_);
        std::string s(reinterpret_cast<const char*>(data_ + off_), n);
        off_ += n;
        return s;
    }

    std::string token(size_t maxLen = 16) {
        static constexpr char alphabet[] =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789._-/\\: *[],;\"'\r\n\t";
        size_t n = small(static_cast<uint8_t>(maxLen + 1));
        std::string out;
        out.reserve(n);
        for (size_t i = 0; i < n && !empty(); ++i) {
            out.push_back(alphabet[byte() % (sizeof(alphabet) - 1)]);
        }
        return out;
    }

    std::string safeName(size_t maxLen = 10) {
        static constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789_-";
        size_t n = 1 + small(static_cast<uint8_t>(std::max<size_t>(1, maxLen)));
        std::string out;
        out.reserve(n);
        for (size_t i = 0; i < n && !empty(); ++i) {
            out.push_back(alphabet[byte() % (sizeof(alphabet) - 1)]);
        }
        if (out.empty()) out = "x";
        return out;
    }

    std::string fileName() {
        std::string base = safeName(12);
        switch (small(8)) {
            case 0: return base + ".txt";
            case 1: return base + ".csv";
            case 2: return base + ".TXT";
            case 3: return base + ".Csv";
            case 4: return base + ".png";
            case 5: return "." + base;
            case 6: return base + ".";
            default: return token(18);
        }
    }

    std::string content(size_t maxLen = 256) {
        return bytes(small(static_cast<uint8_t>(std::min<size_t>(maxLen, 255))));
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t off_ = 0;
};

class CoutSilencer {
public:
    CoutSilencer() : old_(std::cout.rdbuf(sink_.rdbuf())) {}
    ~CoutSilencer() { std::cout.rdbuf(old_); }
private:
    std::ostringstream sink_;
    std::streambuf* old_;
};

inline uint8_t permDigit(FuzzInput& in) {
    return static_cast<uint8_t>(in.byte() & 0x0F); // intentionally includes invalid 8..15
}

inline void seedUsers(FileSystem& fs) {
    fs.addUser("alice", "dev");
    fs.addUser("bob", "dev");
    fs.addUser("eve", "red");
}
