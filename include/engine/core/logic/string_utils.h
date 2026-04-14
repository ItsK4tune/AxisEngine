#pragma once

#include <string>
#include <vector>
#include <cstdint>

class StringUtils {
public:
    static std::vector<uint32_t> GetCodepoints(const std::string& utf8) {
        std::vector<uint32_t> codepoints;
        for (size_t i = 0; i < utf8.length();) {
            unsigned char cp = static_cast<unsigned char>(utf8[i]);
            uint32_t res = 0;
            size_t next = 0;

            if (cp <= 0x7F) {
                res = cp;
                next = 1;
            } else if ((cp & 0xE0) == 0xC0) {
                res = cp & 0x1F;
                next = 2;
            } else if ((cp & 0xF0) == 0xE0) {
                res = cp & 0x0F;
                next = 3;
            } else if ((cp & 0xF8) == 0xF0) {
                res = cp & 0x07;
                next = 4;
            } else {
                // Invalid UTF-8 byte
                i++;
                continue;
            }

            if (i + next > utf8.length()) {
                break;
            }

            for (size_t j = 1; j < next; j++) {
                res = (res << 6) | (static_cast<unsigned char>(utf8[i + j]) & 0x3F);
            }

            codepoints.push_back(res);
            i += next;
        }
        return codepoints;
    }
};
