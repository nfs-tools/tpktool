#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <vector>

#include "WorldWriter.hpp"
#include "WorldReader.hpp"

void WorldWriter::Write(std::ofstream &stream, const std::shared_ptr<TexturePack> &texturePack)
{
    World_PackInfo packInfo{};
    memset(&packInfo, '\0', sizeof(World_PackInfo));

    packInfo.hash = texturePack->hash;
    packInfo.marker = 0x09;

    strcpy(packInfo.name, texturePack->name.c_str());
    strcpy(packInfo.path, texturePack->path.c_str());

    auto textureDataSize = std::accumulate(
            texturePack->textures.begin(),
            texturePack->textures.end(), 0x78,
            [](int ds, std::shared_ptr<Texture> t)
            {
                return ds + t->dataSize;
            });

    auto tpkCapsuleSize = 0;
    auto tpkChunkSize = 0;
    auto dataChunkSize = 0u;

    // 01 00 33 33 - empty
    dataChunkSize += 8 + 0x04;
    // 01 00 32 33 - contains TPK hash
    dataChunkSize += 8 + 0x50;
    // null
    dataChunkSize += 8 + 0x0C;

    auto lastOffset = 0u;

    for (auto i = 0; i < texturePack->textures.size(); i++)
    {
        auto texture = texturePack->textures[i];

        printf("texture %s - offset=%d, size=%d, offDiff=%d, sizeMod=%d, sizeMod2=%d, sizeMod3=%d\n",
               texture->name.c_str(),
               texture->dataOffset,
               texture->dataSize,
               texture->dataOffset - lastOffset,
               32 - (texture->dataSize % 32),
               64 - (texture->dataSize % 64),
               128 - (texture->dataSize % 128));
        lastOffset = texture->dataOffset + texture->dataSize;

        if (i != texturePack->textures.size() - 1 && texture->dataSize % 128 != 0)
        {
            texture->padding = 128 - (texture->dataSize % 128);

            if (texture->padding < 48)
            {
                texture->padding = 48;
            }

            textureDataSize += texture->padding;
        }
    }

    auto textureInfoSize = 0;
    auto hashTableSize = (unsigned int) (texturePack->textures.size() * 8);
    auto ddsTableSize = (unsigned int) (texturePack->textures.size() * 32);

    for (const auto &texture : texturePack->textures)
    {
        textureInfoSize += 144;
        textureInfoSize += 1;
        textureInfoSize += texture->name.length();

        auto namePad = 4 - textureInfoSize % 4;
        textureInfoSize += namePad;
    }

    dataChunkSize += 8 + textureDataSize;

    // info
    tpkChunkSize += 0x8 + 0x7C;
    // hash table
    tpkChunkSize += 0x8 + hashTableSize;
    // DDS info table
    tpkChunkSize += 0x8 + ddsTableSize;
    // info table
    tpkChunkSize += 0x8 + textureInfoSize;

    auto alignSize = 0x78;
//    auto alignSize = (tpkChunkSize % 32) * 3;

    printf("alignSize = %d\n", alignSize);

    tpkCapsuleSize = (0x8 + tpkChunkSize) + 0x38 + (0x8 + dataChunkSize) + (0x8 + alignSize);
//    tpkCapsuleSize = (0x8 + tpkChunkSize) + 0x38 + (0x8 + alignSize) + 0x8 + dataChunkSize + 0x8 + 0x78;

    writeGeneric<int>(stream, 0xB3300000);
    writeGeneric<int>(stream, tpkCapsuleSize);

    writeGeneric<int>(stream, 0x00);
    writeGeneric<int>(stream, 0x30);

    fill<BYTE>(stream, 0x00, 0x30);

    // TPK chunk
    writeGeneric<int>(stream, 0xB3310000);
    writeGeneric<int>(stream, tpkChunkSize);

    // info chunk
    writeGeneric<int>(stream, 0x33310001);
    writeGeneric<int>(stream, 0x7C);

    writeGeneric<World_PackInfo>(stream, packInfo);
    fill<BYTE>(stream, 0x00, 24);

    // hashes chunk
    writeGeneric<int>(stream, 0x33310002);
    writeGeneric<int>(stream, hashTableSize);

    // Write hashes
    for (const auto &texture : texturePack->textures)
    {
        writeGeneric<int>(stream, texture->texHash);
        writeGeneric<int>(stream, 0);
    }

    // Texture info
    writeGeneric<int>(stream, 0x33310004);
    writeGeneric<int>(stream, textureInfoSize);

    auto dataOffset = 0u;
    auto textureInfoBytes = 0u;

    for (auto &texture : texturePack->textures)
    {
        printf("* Write texture: %s (0x%08x)\n", texture->name.c_str(), texture->texHash);
        printf("\tData offset: %d (0x%08x) vs %d (0x%08x)\n", dataOffset, dataOffset, texture->dataOffset,
               texture->dataOffset);

        World_TextureInfo textureInfo{};

        memset(&textureInfo, '\0', sizeof(World_TextureInfo));

        textureInfo.textureHash = texture->texHash;
        textureInfo.typeHash = texture->typeHash;
        textureInfo.unknown1 = boost::get<int>(texture->properties.find("unknown1")->second);
        textureInfo.unknown2 = boost::get<int>(texture->properties.find("unknown2")->second);
        textureInfo.unknown3 = boost::get<int>(texture->properties.find("unknown3")->second);
        textureInfo.unknown4 = boost::get<int>(texture->properties.find("unknown4")->second);
        memcpy(textureInfo.unknown5,
               boost::get<std::vector<BYTE >>(texture->properties.find("unknown5")->second).data(),
               24);
        textureInfo.unknown6 = boost::get<int>(texture->properties.find("unknown6")->second);
        memcpy(textureInfo.unknown7,
               boost::get<std::vector<BYTE >>(texture->properties.find("unknown7")->second).data(),
               60);

        textureInfo.dataOffset = dataOffset;
        textureInfo.dataSize = static_cast<unsigned int>(texture->data.size());
        textureInfo.width = texture->width;
        textureInfo.height = texture->height;
        textureInfo.mipMap = texture->mipMap;
        textureInfoBytes += 144;
        textureInfoBytes += 1 + texture->name.length();

        auto namePad = 4 - textureInfoBytes % 4;
        textureInfoBytes += namePad;

        printf("\tName padding: %d byte(s)\n", namePad);

        hex_dump(&textureInfo, sizeof(World_TextureInfo), std::cout);

        writeGeneric<World_TextureInfo>(stream, textureInfo, sizeof(World_TextureInfo) - 1);
        writeGeneric<BYTE>(stream, static_cast<BYTE>(texture->name.length() + namePad));

        stream.write(texture->name.c_str(), texture->name.length());

        for (auto j = 0; j < namePad; ++j)
        {
            writeGeneric<BYTE>(stream, 0x00);
        }

        dataOffset += texture->data.size();
        dataOffset += texture->padding;
    }

    // DDS type info
    writeGeneric<int>(stream, 0x33310005);
    writeGeneric<int>(stream, ddsTableSize);

    for (auto &texture : texturePack->textures)
    {
        for (auto i = 0; i < 12; ++i)
        {
            writeGeneric<BYTE>(stream, 0x00);
        }
        writeGeneric<int>(stream, (int) texture->compressionType);
        for (auto i = 0; i < 16; ++i)
        {
            writeGeneric<BYTE>(stream, 0x00);
        }
    }

    // pre-data null
    writeGeneric<int>(stream, 0);
    writeGeneric<int>(stream, alignSize);

    for (auto i = 0; i < alignSize; ++i)
    {
        writeGeneric<BYTE>(stream, 0x00);
    }

    // write data chunk
    writeGeneric<int>(stream, 0xB3320000);
    writeGeneric<int>(stream, dataChunkSize);

    // write child 1
    writeGeneric<int>(stream, 0x33330001);
    writeGeneric<int>(stream, 0x4);

    for (auto i = 0; i < 4; ++i)
    {
        writeGeneric<BYTE>(stream, 0x00);
    }

    // write child 2
    writeGeneric<int>(stream, 0x33320001);
    writeGeneric<int>(stream, 0x50);
    writeGeneric<int>(stream, 0x00000000);
    writeGeneric<int>(stream, 0x00000000);
    writeGeneric<int>(stream, 0x2);
    writeGeneric<int>(stream, texturePack->hash);
    writeGeneric<int>(stream, 0x00000000);
    writeGeneric<int>(stream, 0x00000000);
    for (auto i = 0; i < 56; ++i)
    {
        writeGeneric<BYTE>(stream, 0x00);
    }

    writeGeneric<int>(stream, 0);
    writeGeneric<int>(stream, 0x0C);

    for (auto i = 0; i < 0x0C; ++i)
    {
        writeGeneric<BYTE>(stream, 0x00);
    }

    // write data
    writeGeneric<int>(stream, 0x33320002);
    writeGeneric<int>(stream, textureDataSize);

    for (auto i = 0; i < 0x78; ++i)
    {
        writeGeneric<BYTE>(stream, 0x11);
    }

    for (auto i = 0; i < texturePack->textures.size(); ++i)
    {
        auto texture = texturePack->textures[i];

        stream.write((const char *) texture->data.data(), texture->data.size());

        if (i != texturePack->textures.size() - 1)
        {
            for (auto j = 0; j < texture->padding; ++j)
            {
                writeGeneric<BYTE>(stream, 0x00);
            }
        }
    }
}