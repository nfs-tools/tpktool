#include <functional>
#include <iostream>
#include <numeric>
#include <vector>

#include "WorldWriter.hpp"
#include "WorldReader.hpp"

void WorldWriter::Write(std::ofstream &stream, const std::shared_ptr<TexturePack> &texturePack)
{
    auto textureDataSize = std::accumulate(
            texturePack->textures.begin(),
            texturePack->textures.end(), 0x78,
            [](int ds, std::shared_ptr<Texture> t)
            {
                return ds + t->dataSize;
            });

//    for (auto i = 0; i < texturePack->textures.size(); ++i)
//    {
//        auto texture = texturePack->textures[i];
//
//        if (i != texturePack->textures.size() - 1 && texture->dataSize % 32 != 0)
//        {
//
////            texture->padding = (32 - (texture->dataSize % 32)) * 3;
//
////            texture->padding = (texture->dataSize % 32) * 3;
////            printf("Padding for %s: %d\n", texture->name.c_str(), texture->padding);
//        }
//
//        textureDataSize += texture->padding;
//    }

    auto dataChunkSize = 0u;

    // 01 00 33 33 - empty
    dataChunkSize += 8 + 0x04;
    // 01 00 32 33 - contains TPK hash
    dataChunkSize += 8 + 0x50;
    // null
    dataChunkSize += 8 + 0x0C;

    auto ddsInfoSize = (unsigned int) (32 * texturePack->textures.size());
    auto textureInfoSize = 0u;
    auto hashTableSize = (unsigned int) (0x8 * texturePack->textures.size());

    auto lastTextureOffset = 0u;

    for (auto i = 0; i < texturePack->textures.size(); i++)
//    for (const auto &texture : texturePack->textures)
    {
        auto texture = texturePack->textures[i];

        textureInfoSize += 144;
        textureInfoSize += 1;
        textureInfoSize += texture->name.length();

        auto namePad = 4 - textureInfoSize % 4;
        textureInfoSize += namePad;

        texture->padding = (texture->dataOffset - lastTextureOffset);

        if (texture->padding == 0 && i != 0)
        {
            texture->padding = (texture->dataSize % 32) * (namePad == 1 ? 5 : namePad + 1);
        }

        printf("Padding for %s = %d\n", texture->name.c_str(), texture->padding);

        lastTextureOffset = texture->dataOffset + texture->dataSize;

        if (i == 1)
        {
            texturePack->textures[0]->padding = texture->padding;
        }
//        texture->padding = (texture->dataSize % 32) * 3;
//        {
//            texture->padding = (texture->dataSize % 32) * (namePad + 1);
//        }
    }

    dataChunkSize += 8 + textureDataSize;
    auto tpkChunkSize = 0x7c + 8 + hashTableSize + 8 + textureInfoSize + 8 + ddsInfoSize + 8;
    auto capsuleSize = tpkChunkSize + 0x8 + 0x38 + 0x8 + ((tpkChunkSize % 32) * 3) + 0x8 + dataChunkSize + 0x8 + 0x78;

    writeGeneric<int>(stream, 0xB3300000);
    writeGeneric<int>(stream, capsuleSize);

    // first null
    writeGeneric<int>(stream, 0x00000000);
    writeGeneric<int>(stream, 0x30);

    for (auto i = 0; i < 0x30; ++i)
    {
        writeGeneric<BYTE>(stream, 0x00);
    }

    // TPK chunk
    writeGeneric<int>(stream, 0xB3310000);
    writeGeneric<int>(stream, tpkChunkSize);

    // TPK info
    writeGeneric<int>(stream, 0x33310001);
    writeGeneric<int>(stream, 0x7C);

    writeGeneric<int>(stream, 0x9);

    char name[0x1C];
    memset(&name, '\0', 0x1C);

    char path[0x40];
    memset(&path, '\0', 0x40);

    strcpy(name, texturePack->name.data());
    strcpy(path, texturePack->path.data());

    stream.write(name, 0x1C);
    stream.write(path, 0x40);

    writeGeneric<int>(stream, texturePack->hash);

    for (auto i = 0; i < 24; ++i)
    {
        writeGeneric<BYTE>(stream, 0x00);
    }

    // Hash table
    writeGeneric<int>(stream, 0x33310002);
    writeGeneric<int>(stream, hashTableSize);

    for (auto &texture : texturePack->textures)
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
    writeGeneric<int>(stream, ddsInfoSize);

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
    writeGeneric<int>(stream, (tpkChunkSize % 32) * 3);

    for (auto i = 0; i < (tpkChunkSize % 32) * 3; ++i)
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