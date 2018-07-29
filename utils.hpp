#ifndef TPKTOOL_UTILS_HPP
#define TPKTOOL_UTILS_HPP

#include <fstream>
#include <ostream>
#include <string>
#include <boost/variant.hpp>

#include "DDS.hpp"

#define PACK __attribute__((__packed__))

typedef unsigned char BYTE;
typedef boost::variant<int, std::string, BYTE, std::vector<BYTE>> TEXTURE_PARAMETER;

enum Game
{
    World = 0
};

typedef struct PACK
{
    unsigned int Magic; // 0x55441122
    unsigned int OutSize;
    unsigned int BlockSize;
    unsigned int Unknown2;
    unsigned int Unknown3;
    unsigned int Unknown4;
} CompressBlockHead;

size_t getLength(std::ifstream &stream);

template<typename T>
T readGeneric(std::ifstream &stream, size_t size = sizeof(T))
{
//    printf("[debug] read %s (%zu bytes)\n", typeid(T).name(), size);
    T result;
    stream.read((char *) &result, size);

    return result;
}

template <typename T>
void writeGeneric(std::ofstream &stream, T data, size_t size = sizeof(T))
{
//    printf("[debug] write %s (%zu bytes)\n", typeid(T).name(), size);
    stream.write((const char*) &data, size);
}

void printStreamPosition(std::ifstream &stream);

template<class Elem, class Traits>
inline void
hex_dump(const void *aData, std::size_t aLength, std::basic_ostream<Elem, Traits> &aStream, std::size_t aWidth = 16)
{
    const auto *const start = static_cast<const char *>(aData);
    const char *const end = start + aLength;
    const char *line = start;
    while (line != end)
    {
        aStream.width(4);
        aStream.fill('0');
        aStream << std::hex << line - start << " : ";
        std::size_t lineLength = std::min(aWidth, static_cast<std::size_t>(end - line));
        for (std::size_t pass = 1; pass <= 2; ++pass)
        {
            for (const char *next = line; next != end && next != line + aWidth; ++next)
            {
                char ch = *next;
                switch (pass)
                {
                    case 1:
                        aStream << (ch < 32 ? '.' : ch);
                        break;
                    case 2:
                        if (next != line)
                        {
                            aStream << " ";
                        }
                        aStream.width(2);
                        aStream.fill('0');
                        aStream << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(ch));
                        break;
                    default:
                        break;
                }
            }
            if (pass == 1 && lineLength != aWidth)
            {
                aStream << std::string(aWidth - lineLength, ' ');
            }
            aStream << " ";
        }
        aStream << std::endl;
        line = line + lineLength;
    }
}

#endif //TPKTOOL_UTILS_HPP
