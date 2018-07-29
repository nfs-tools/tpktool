#ifndef TPKTOOL_TEXTUREPACK_HPP
#define TPKTOOL_TEXTUREPACK_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <boost/variant.hpp>

#include "utils.hpp"

enum CompressionType
{
    DXT1 = 0x31545844,
    DXT3 = 0x33545844,
    DXT5 = 0x35545844,
    ATI2 = 0x32495441,
    A8R8G8B8 = 0x41,
    P8 = 0x15,
    Unknown = 0x00
};

/**
 * Holds texture information
 */
class Texture
{
public:
    std::string name;
    int32_t texHash = -1;
    int32_t typeHash = -1;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t dataSize = 0;
    uint32_t dataOffset = 0;
    uint32_t mipMap = 0;

    uint32_t padding = 0;

    CompressionType compressionType = Unknown;

    std::map<std::string, TEXTURE_PARAMETER> properties{};
    std::vector<BYTE> data{};

    Texture() = default;

    virtual ~Texture() = default;
};

/**
 * Holds TPK information
 */
class TexturePack
{
public:
    TexturePack()
    = default;

    virtual ~TexturePack()
    = default;

    std::string name;
    std::string path;
    int32_t hash;

    std::vector<int> textureHashes;
    std::vector<std::shared_ptr<Texture>> textures;
};


#endif //TPKTOOL_TEXTUREPACK_HPP
