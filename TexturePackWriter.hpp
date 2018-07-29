#ifndef TPKTOOL_TEXTUREPACKWRITER_HPP
#define TPKTOOL_TEXTUREPACKWRITER_HPP

#include <fstream>
#include "TexturePack.hpp"

class TexturePackWriter
{
public:
    virtual void Write(std::ofstream &stream, const std::shared_ptr<TexturePack>& texturePack) = 0;
};


#endif //TPKTOOL_TEXTUREPACKWRITER_HPP
