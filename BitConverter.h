#ifndef CARPARTS_BITCONVERTER_H
#define CARPARTS_BITCONVERTER_H

typedef unsigned char BYTE;

#include <cstdint>
#include <memory>
#include <vector>

class BitConverter
{
public:
    static int16_t ToInt16(const BYTE bytes[], int offset);

    static uint16_t ToUInt16(const BYTE bytes[], int offset);

    static int32_t ToInt32(const BYTE bytes[], int offset);

    static uint32_t ToUInt32(const BYTE bytes[], int offset);

    static uint64_t ToUInt64(const BYTE bytes[], int offset);

    static std::vector<BYTE> GetBytes(int value);

    static std::vector<BYTE> GetBytes(long value);
};

#endif
