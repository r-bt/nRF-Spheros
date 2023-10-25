#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <cstring>
#include <vector>

template <typename T>
std::vector<uint8_t> int_to_bytes(T i, size_t size, bool big_endian = true)
{
    std::vector<uint8_t> bytes(size);

    if (big_endian) {
        for (size_t index = 0; index < size; ++index) {
            bytes[size - index - 1] = static_cast<uint8_t>(i >> (index * 8));
        }
    } else {
        for (size_t index = 0; index < size; ++index) {
            bytes[index] = static_cast<uint8_t>(i >> (index * 8));
        }
    }

    return bytes;
}

#endif // UTILS_H