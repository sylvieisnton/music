#pragma once

#include <cstdint>

inline std::uint32_t checksum(std::uint32_t crc, unsigned char byte)
{
    crc ^= byte;

    for (int i = 0; i < 8; ++i)
    {
        if (crc & 1)
        {
            crc = (crc >> 1) ^ 0xEDB88320u;
        }
        else
        {
            crc >>= 1;
        }
    }

    return crc;
}