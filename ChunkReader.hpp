#ifndef TPKTOOL_CHUNKREADER_HPP
#define TPKTOOL_CHUNKREADER_HPP

#include <fstream>
#include <memory>
#include "utils.hpp"
#include "TexturePackReader.hpp"
#include "WorldReader.hpp"

class ChunkReader
{
public:
    ChunkReader(std::ifstream &stream, Game game) : m_stream(stream), m_game(game)
    {}

    std::vector<std::shared_ptr<TexturePack>> Read(size_t fileSize)
    {
        for (auto i = 0; i < 0xFFFF && m_stream.tellg() < fileSize; ++i)
        {
            auto chunkId = readGeneric<int>(m_stream);
            auto chunkSize = readGeneric<int>(m_stream);
            auto chunkEnd = ((long) m_stream.tellg()) + chunkSize;

            if (chunkId == 0xB3300000)
            {
                printf("Found TPK\n");

                std::shared_ptr<TexturePackReader> container;

                switch (m_game)
                {
                    case World:
                    {
                        container = std::make_shared<WorldReader>();
                        break;
                    }
                    default:
                        throw std::runtime_error("Unsupported game");
                }

                texturePacks.emplace_back(container->Read(m_stream, chunkSize));
            }

            m_stream.seekg(chunkEnd, m_stream.beg);
        }

        return texturePacks;
    }

    std::vector<std::shared_ptr<TexturePack>> texturePacks;
private:
    std::ifstream &m_stream;
    Game m_game;
};


#endif //TPKTOOL_CHUNKREADER_HPP
