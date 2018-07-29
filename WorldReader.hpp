#ifndef TPKTOOL_WORLDREADER_HPP
#define TPKTOOL_WORLDREADER_HPP

#include <vector>

#include "TexturePackReader.hpp"
#include <boost/filesystem.hpp>
#include <cassert>
#include <iostream>

#include "JDLZ.h"
#include "utils.hpp"

typedef struct PACK
{
    int marker; // e.g. 09 00 00 00
    char name[28];
    char path[64];
    int hash;
} World_PackInfo;

typedef struct PACK
{
    int textureHash;
    int offset;
    int size;

    int unknown[6];
} World_CompressionInfo;

typedef struct PACK
{
    BYTE blank[12];

    int textureHash;
    int typeHash;
    int unknown1;
    unsigned int dataSize;
    int unknown2;
    unsigned int width;
    unsigned int height;
    unsigned int mipMap;
    int unknown3;
    int unknown4;
    BYTE unknown5[24];
    int unknown6;
    unsigned int dataOffset;
    BYTE unknown7[60];
    BYTE nameLength;
} World_TextureInfo;

class WorldReader : public TexturePackReader
{
public:
    WorldReader();

    ~WorldReader();

    std::shared_ptr<TexturePack> Read(std::ifstream &stream, int chunkSize) override;

private:
    void ReadChunks(std::ifstream &stream, int size) override;

    std::vector<World_CompressionInfo> m_compressionBlocks;
    std::vector<World_TextureInfo> m_infoBlocks;

    void dumpTextureInfo(const World_TextureInfo &textureInfo, const std::string& name) const;
    void generateDDS(const std::shared_ptr<Texture>& texture) const;

    std::shared_ptr<Texture> pushTexture(const World_TextureInfo &textureInfo, const std::string &name) const;
};


#endif //TPKTOOL_WORLDREADER_HPP
