#ifndef TPKTOOL_TEXTUREPACKCONTAINER_HPP
#define TPKTOOL_TEXTUREPACKCONTAINER_HPP

#include <fstream>
#include "TexturePack.hpp"

class TexturePackReader
{
public:
    virtual std::shared_ptr<TexturePack> Read(std::ifstream &stream, int chunkSize) = 0;

private:
    virtual void ReadChunks(std::ifstream &stream, int size) = 0;

protected:
    std::shared_ptr<TexturePack> m_texturePack;
};


#endif //TPKTOOL_TEXTUREPACKCONTAINER_HPP
