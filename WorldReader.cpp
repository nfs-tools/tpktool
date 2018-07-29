#include "WorldReader.hpp"

constexpr int TPK_INFO_ID = 0x33310001;
constexpr int TPK_HASHES_ID = 0x33310002;
constexpr int TPK_COMPRESSION_ID = 0x33310003;
constexpr int TPK_TEXTURES_ID = 0x33310004;
constexpr int TPK_DXT_ID = 0x33310005;

constexpr int TPK_DATA_ID = 0x33320002;

std::shared_ptr <TexturePack> WorldReader::Read(std::ifstream &stream, int chunkSize)
{
    this->m_texturePack = std::make_shared<TexturePack>();

    this->ReadChunks(stream, chunkSize);

    return m_texturePack;
}

void WorldReader::ReadChunks(std::ifstream &stream, int size)
{
    auto runTo = ((long) stream.tellg()) + size;

    for (auto i = 0; i < 0xFFFF && stream.tellg() < runTo; ++i)
    {
        auto chunkId = readGeneric<unsigned int>(stream);
        auto chunkSize = readGeneric<unsigned int>(stream);
        auto chunkEnd = ((long) stream.tellg()) + chunkSize;

        printf("chunk 0x%08x [%d bytes]\n", chunkId, chunkSize);

        if ((chunkId & 0x80000000) != 0u)
        {
            printf("container\n");
            this->ReadChunks(stream, chunkSize);
        }

        switch (chunkId)
        {
            case TPK_INFO_ID:
            {
                auto info = readGeneric<World_PackInfo>(stream);

                printf("\tinfo:\n");
                printf("\t\tmarker = 0x%08x\n", info.marker);
                printf("\t\tname   = %s\n", info.name);
                printf("\t\tpath   = %s\n", info.path);
                printf("\t\thash   = 0x%08x\n", info.hash);

                m_texturePack->name = std::string(info.name);
                m_texturePack->path = std::string(info.path);
                m_texturePack->hash = info.hash;

                break;
            }
            case TPK_HASHES_ID:
            {
                assert(chunkSize % 8 == 0);
                const int numHashes = chunkSize / 8;
                assert(numHashes > 0);

                printf("\thashes: %d\n", chunkSize / 8);

                for (auto j = 0; j < chunkSize / 8; ++j)
                {
                    auto hash = readGeneric<int>(stream);

                    printf("\t\thash #%d: 0x%08x\n", j + 1, hash);

                    stream.seekg(4, stream.cur);

                    m_texturePack->textureHashes.emplace_back(hash);
                }

                break;
            }
            case TPK_COMPRESSION_ID:
            {
                assert(chunkSize % sizeof(World_CompressionInfo) == 0);
                const int numBlocks = chunkSize / sizeof(World_CompressionInfo);
                assert(numBlocks > 0);

                printf("\tcompression blocks: %d\n", numBlocks);

                for (auto j = 0; j < chunkSize / sizeof(World_CompressionInfo); ++j)
                {
                    auto compInfo = readGeneric<World_CompressionInfo>(stream);

                    printf("\t\tblock #%d: \n", j + 1);
                    printf("\t\t\thash: 0x%08x\n", compInfo.textureHash);
                    printf("\t\t\toff:  0x%08x (%d)\n", compInfo.offset, compInfo.offset);
                    printf("\t\t\tsize: %d\n", compInfo.size);

                    m_compressionBlocks.emplace_back(compInfo);
                }

                break;
            }
            case TPK_TEXTURES_ID:
            {
                auto lastOffset = 0;

                for (auto j = 0; j < m_texturePack->textureHashes.size(); ++j)
                {
                    auto textureInfo = readGeneric<World_TextureInfo>(stream);
                    char name[textureInfo.nameLength];
                    stream.read(name, textureInfo.nameLength);

                    pushTexture(textureInfo, std::string(name));
                    dumpTextureInfo(textureInfo, std::string(name));

                    printf("offset diff: %d\n", textureInfo.dataOffset - lastOffset);
                    printf("size mod: %d\n", textureInfo.dataSize % 32);

                    lastOffset = textureInfo.dataOffset + textureInfo.dataSize;
                }

                break;
            }
            case TPK_DXT_ID:
            {
                for (auto j = 0; j < m_texturePack->textureHashes.size(); ++j)
                {
                    stream.seekg(12, stream.cur);
                    m_texturePack->textures[j]->compressionType = (CompressionType) readGeneric<int>(stream);
                    stream.seekg(16, stream.cur);
                }
                break;
            }
            case TPK_DATA_ID:
            {
                stream.seekg(0x78, stream.cur);

                if (m_compressionBlocks.empty())
                {
                    auto dataStartPos = (long) stream.tellg();

                    printf("Normal TPK\n");

                    auto *texData = static_cast<unsigned char *>(malloc(0));

                    for (auto &texture : m_texturePack->textures)
                    {
                        stream.seekg(dataStartPos + texture->dataOffset);

                        texData = static_cast<unsigned char *>(realloc(texData, texture->dataSize));

                        stream.read(reinterpret_cast<char *>(texData), texture->dataSize);

                        texture->data = std::vector<BYTE>(texData, texData + texture->dataSize);

                        generateDDS(texture);
                    }
                    //
                } else
                {
                    printf("Compressed TPK\n");

                    for (auto &compBlock : m_compressionBlocks)
                    {
                        auto bytesRead = 0;

                        printf("\tCompression block:\n");
                        printf("\t\toffset    = %d\n", compBlock.offset);
                        printf("\t\tsize      = %d\n", compBlock.size);
                        stream.seekg(compBlock.offset);

                        printStreamPosition(stream);

                        std::vector<std::vector<BYTE>> blocks;

                        while (bytesRead < compBlock.size)
                        {
                            auto cbh = readGeneric<CompressBlockHead>(stream);
                            std::vector<BYTE> blockData;

                            blockData.resize(cbh.BlockSize - 24);

                            stream.read((char *) &blockData[0], cbh.BlockSize - 24);

                            std::vector<BYTE> decompressedData = JDLZ::decompress(blockData);

                            bytesRead += cbh.BlockSize;

                            printStreamPosition(stream);

                            blocks.emplace_back(decompressedData);
                        }

                        printf("\t\tTotal blocks: %zu\n", blocks.size());

                        if (blocks.size() == 1)
                        {
//                            hex_dump(blocks[0].data(), blocks[0].size(), std::cout);
                            World_TextureInfo textureInfo;

                            memcpy(&textureInfo,
                                   blocks[0].data() + (blocks[0].size() - 212),
                                   sizeof(World_TextureInfo));


                            char name[textureInfo.nameLength];

                            memcpy(&name,
                                   blocks[0].data() + (blocks[0].size() - 212) + sizeof(World_TextureInfo),
                                   textureInfo.nameLength);

                            printf("\t\t\tname  = %s\n", name);
                            dumpTextureInfo(textureInfo, std::string(name));

                            auto newTexture = pushTexture(textureInfo, std::string(name));

                            auto compressionType = (CompressionType) *(int *) (blocks[0].data()
                                                                               + (blocks[0].size() - 212)
                                                                               + sizeof(World_TextureInfo)
                                                                               + textureInfo.nameLength
                                                                               + (47 - textureInfo.nameLength));
                            printf("\t\t\tcomp = 0x%08x\n", compressionType);

                            unsigned char texData[textureInfo.dataSize];

                            memcpy(texData, blocks[0].data(), textureInfo.dataSize);

                            newTexture->compressionType = compressionType;
                            newTexture->data = std::vector<BYTE>(texData, texData + textureInfo.dataSize);

                            generateDDS(newTexture);
                        } else
                        {
                            auto block = blocks[blocks.size() - 2];

                            World_TextureInfo textureInfo;

                            memcpy(&textureInfo,
                                   block.data() + (block.size() - 212),
                                   sizeof(World_TextureInfo));


                            char name[textureInfo.nameLength];

                            memcpy(&name,
                                   block.data() + (block.size() - 212) + sizeof(World_TextureInfo),
                                   textureInfo.nameLength);

                            printf("\t\t\tname  = %s\n", name);
                            dumpTextureInfo(textureInfo, std::string(name));

                            auto newTexture = pushTexture(textureInfo, std::string(name));

                            auto compressionType = (CompressionType) *(int *) (block.data()
                                                                               + (block.size() - 212)
                                                                               + sizeof(World_TextureInfo)
                                                                               + textureInfo.nameLength
                                                                               + (47 - textureInfo.nameLength));
                            printf("\t\t\tcomp = 0x%08x\n", compressionType);

                            unsigned char texData[textureInfo.dataSize];

                            int remainingSize = textureInfo.dataSize;

                            for (auto j = 0; j < blocks.size(); j++)
                            {
                                if (j != blocks.size() - 2)
                                {
                                    memcpy(texData + (textureInfo.dataSize - remainingSize), blocks[j].data(),
                                           blocks[j].size());

                                    remainingSize -= blocks[j].size();
                                }
                            }

                            newTexture->compressionType = compressionType;
                            newTexture->data = std::vector<BYTE>(texData, texData + textureInfo.dataSize);

                            generateDDS(newTexture);
                        }
                    }
                }

                break;
            }
            default:
                break;
        }

        stream.seekg(chunkEnd);
    }
}

void WorldReader::dumpTextureInfo(const World_TextureInfo &textureInfo, const std::string &name) const
{
    printf("\t\tTexture info for %s:\n", name.c_str());
    printf("\t\t\ttexHash = 0x%08x\n", textureInfo.textureHash);
    printf("\t\t\ttype   = 0x%08x\n", textureInfo.typeHash);
    printf("\t\t\tunk1   = 0x%08x\n", textureInfo.unknown1);
    printf("\t\t\tdsz    = %d\n", textureInfo.dataSize);
    printf("\t\t\tunk2   = 0x%08x\n", textureInfo.unknown2);
    printf("\t\t\twidth  = %d\n", textureInfo.width);
    printf("\t\t\theight = %d\n", textureInfo.height);
    printf("\t\t\tmipmap = %d\n", textureInfo.mipMap);
    printf("\t\t\tunk3   = 0x%08x\n", textureInfo.unknown3);
    printf("\t\t\tunk4   = 0x%08x\n", textureInfo.unknown4);
    printf("\t\t\tunk5:\n");
    hex_dump(textureInfo.unknown5, 24, std::cout);
    printf("\t\t\tunk6   = 0x%08x\n", textureInfo.unknown6);
    printf("\t\t\tdoff   = 0x%08x (%d)\n", textureInfo.dataOffset, textureInfo.dataOffset);
    printf("\t\t\tunk7:\n");
    hex_dump(textureInfo.unknown7, 60, std::cout);
    printf("\t\t\tnlen   = %d\n", textureInfo.nameLength);
}

std::shared_ptr <Texture> WorldReader::pushTexture(const World_TextureInfo &textureInfo, const std::string &name) const
{
    auto texture = std::make_shared<Texture>();
    texture->name = name;
    texture->height = textureInfo.height;
    texture->width = textureInfo.width;
    texture->typeHash = textureInfo.typeHash;
    texture->texHash = textureInfo.textureHash;
    texture->dataOffset = textureInfo.dataOffset;
    texture->dataSize = textureInfo.dataSize;
    texture->mipMap = textureInfo.mipMap;

    TEXTURE_PARAMETER unk1(textureInfo.unknown1);
    TEXTURE_PARAMETER unk2(textureInfo.unknown2);
    TEXTURE_PARAMETER unk3(textureInfo.unknown3);
    TEXTURE_PARAMETER unk4(textureInfo.unknown4);
    TEXTURE_PARAMETER unk5(std::vector<BYTE>(
            textureInfo.unknown5,
            textureInfo.unknown5 +
            sizeof(textureInfo.unknown5) / sizeof(textureInfo.unknown5[0])
    ));
    TEXTURE_PARAMETER unk6(textureInfo.unknown6);
    TEXTURE_PARAMETER unk7(std::vector<BYTE>(
            textureInfo.unknown7,
            textureInfo.unknown7 +
            sizeof(textureInfo.unknown7) / sizeof(textureInfo.unknown7[0])
    ));

    texture->properties["unknown1"] = unk1;
    texture->properties["unknown2"] = unk2;
    texture->properties["unknown3"] = unk3;
    texture->properties["unknown4"] = unk4;
    texture->properties["unknown5"] = unk5;
    texture->properties["unknown6"] = unk6;
    texture->properties["unknown7"] = unk7;

    m_texturePack->textures.emplace_back(texture);

    return texture;
}

void WorldReader::generateDDS(const std::shared_ptr <Texture> &texture) const
{
    DDS_HEADER header{};
    memset(&header, '\0', sizeof(header));

    header.dwSize = 124;
    header.dwFlags = 0x21007;
    header.dwHeight = texture->height;
    header.dwWidth = texture->width;
    header.dwMipMapCount = 0;

    DDS_PIXELFORMAT pixelFormat{};
    memset(&pixelFormat, '\0', sizeof(pixelFormat));
    pixelFormat.dwSize = 32;

    if (texture->compressionType == P8)
    {
        pixelFormat.dwFlags = 0x41;
        pixelFormat.dwRGBBitCount = 0x20;
        pixelFormat.dwRBitMask = 0xFF0000;
        pixelFormat.dwGBitMask = 0xFF00;
        pixelFormat.dwBBitMask = 0xFF;
        pixelFormat.dwABitMask = 0xFF000000;
        header.dwCaps = 0x40100A;
    } else
    {
        pixelFormat.dwFlags = 4;
        pixelFormat.dwFourCC = (uint32_t) texture->compressionType;
        header.dwCaps = 0x401008;
    }

    header.ddspf = pixelFormat;

    std::ofstream ddsStream(
            boost::filesystem::path(texture->name).concat(".dds").c_str(),
            std::ios::binary | std::ios::trunc
    );

    writeGeneric<int>(ddsStream, 0x20534444);
    writeGeneric<DDS_HEADER>(ddsStream, header);

    ddsStream.write((const char *) texture->data.data(), texture->data.size());
}

WorldReader::WorldReader()
= default;

WorldReader::~WorldReader()
= default;
