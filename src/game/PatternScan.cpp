#include "PatternScan.hpp"

#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace PatternScan {
namespace {
    struct PatternByte {
        uint8_t value;
        bool wildcard;
    };

    std::vector<PatternByte> Parse(const char* pattern) {
        std::vector<PatternByte> bytes;
        const char* p = pattern;
        while (*p) {
            while (*p == ' ')
                ++p;
            if (!*p)
                break;

            if (*p == '?') {
                bytes.push_back({0, true});
                while (*p == '?')
                    ++p;
            } else {
                bytes.push_back({static_cast<uint8_t>(strtoul(p, nullptr, 16)), false});
                while (*p && *p != ' ')
                    ++p;
            }
        }
        return bytes;
    }
} // namespace

void* Find(const char* moduleName, const char* pattern) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) {
        return nullptr;
    }

    auto dosHeader   = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    auto ntHeaders   = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(hModule)
                                                         + dosHeader->e_lfanew);
    size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;

    auto bytes = Parse(pattern);
    if (bytes.empty()) {
        return nullptr;
    }

    auto base = reinterpret_cast<uint8_t*>(hModule);
    for (size_t i = 0; i + bytes.size() <= imageSize; ++i) {
        bool match = true;
        for (size_t j = 0; j < bytes.size(); ++j) {
            if (!bytes[j].wildcard && base[i + j] != bytes[j].value) {
                match = false;
                break;
            }
        }
        if (match) {
            return base + i;
        }
    }

    return nullptr;
}
} // namespace PatternScan
