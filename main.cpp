#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "ChunkReader.hpp"
#include "WorldWriter.hpp"

int main(int argc, const char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: tpktool <game> <file>" << std::endl;
        return 1;
    }

    std::string game(argv[1]);
    std::string file(argv[2]);

    if (game != "world")
    {
        std::cerr << "Valid games: 'world'" << std::endl;
        return 1;
    }

    std::ifstream fileStream(file, std::ios::binary);

    if (!fileStream.good())
    {
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }

    auto texturePacks = (new ChunkReader(fileStream, Game::World))->Read(getLength(fileStream));

    for (auto &texturePack : texturePacks)
    {
        // Check for DDS mods
        for (auto &texture : texturePack->textures)
        {
            auto ddsPath = boost::filesystem::path(texture->name).concat("_mod.dds");

            if (boost::filesystem::is_regular_file(ddsPath))
            {
                std::cout << "* Found DDS mod for: " << texture->name << std::endl;

                std::ifstream modStream(ddsPath.c_str(), std::ios::binary);

                modStream.ignore(4);

                auto ddsHeader = readGeneric<DDS_HEADER>(modStream);
                auto dataLength = (unsigned int) (getLength(modStream) - sizeof(DDS_HEADER));

                assert(ddsHeader.dwMipMapCount > 0);

                printf("\tdataSize = %d/%d\n", ddsHeader.dwPitchOrLinearSize, dataLength);
                printf("\tcompType = 0x%08x\n", ddsHeader.ddspf.dwFourCC);
                printf("\tmipMap   = %d\n", ddsHeader.dwMipMapCount - 1);
                printf("\tdims     = %dx%d\n", ddsHeader.dwWidth, ddsHeader.dwHeight);

                texture->dataSize = dataLength;
                texture->compressionType = (CompressionType) ddsHeader.ddspf.dwFourCC;
                texture->mipMap = ddsHeader.dwMipMapCount - 1;
                texture->width = ddsHeader.dwWidth;
                texture->height = ddsHeader.dwHeight;

                unsigned char data[dataLength];
                memset(data, '\0', dataLength);

                modStream.read((char *) &data, dataLength);

                texture->data = std::vector<BYTE>(data, data + dataLength);
            }
        }

        std::ofstream outStream(
                boost::filesystem::path(texturePack->name).concat(".bin").c_str(),
                std::ios::binary | std::ios::trunc
        );

        (new WorldWriter)->Write(outStream, texturePack);
    }

    return 0;
}