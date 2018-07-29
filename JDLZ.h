#ifndef CARPARTS_JDLZ_H
#define CARPARTS_JDLZ_H

#include <cstdlib>
#include <vector>

typedef unsigned char BYTE;

struct JDLZInfo
{

};

class JDLZ
{
#define HEADER_SIZE 16
public:
    static std::vector<BYTE> decompress(std::vector<BYTE> input);
};

#endif //CARPARTS_JDLZ_H