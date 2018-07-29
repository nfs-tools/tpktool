#include "BitConverter.h"

short BitConverter::ToInt16(const BYTE *bytes, int offset)
{
    auto result = (int16_t) ((int) bytes[offset] & 0xff);
    result |= ((int) bytes[offset + 1] & 0xff) << 8;
    return (int16_t) (result & 0xffff);
}

uint16_t BitConverter::ToUInt16(const BYTE *bytes, int offset)
{
    auto result = (int) bytes[offset + 1] & 0xff;
    result |= ((int) bytes[offset] & 0xff) << 8;
    return (uint16_t) (result & 0xffff);
}

int32_t BitConverter::ToInt32(const BYTE *bytes, int offset)
{
    auto result = (int) bytes[offset] & 0xff;
    result |= ((int) bytes[offset + 1] & 0xff) << 8;
    result |= ((int) bytes[offset + 2] & 0xff) << 16;
    result |= ((int) bytes[offset + 3] & 0xff) << 24;
    return result;
}

uint32_t BitConverter::ToUInt32(const BYTE *bytes, int offset)
{
    auto result = static_cast<uint32_t>((int) bytes[offset] & 0xff);
    result |= ((int) bytes[offset + 1] & 0xff) << 8;
    result |= ((int) bytes[offset + 2] & 0xff) << 16;
    result |= ((int) bytes[offset + 3] & 0xff) << 24;
    return result & 0xFFFFFFFF;
}

uint64_t BitConverter::ToUInt64(const BYTE *bytes, int offset)
{
    uint64_t result = 0;

    for (int i = 0; i <= 56; i += 8)
    {
        result |= ((int) bytes[offset++] & 0xff) << i;
    }

    return result;
}

std::vector<BYTE> BitConverter::GetBytes(int value)
{
    std::vector<BYTE> bytes;
    bytes.resize(4);
    bytes[0] = (BYTE) (value >> 24);
    bytes[1] = (BYTE) (value >> 16);
    bytes[2] = (BYTE) (value >> 8);
    bytes[3] = (BYTE) (value);
    return bytes;
}

std::vector<BYTE> BitConverter::GetBytes(long value)
{
    std::vector<BYTE> bytes;
    bytes.resize(4);
    bytes[0] = (BYTE) (value >> 24);
    bytes[1] = (BYTE) (value >> 16);
    bytes[2] = (BYTE) (value >> 8);
    bytes[3] = (BYTE) (value);
    return bytes;
}