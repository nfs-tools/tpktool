#ifndef TPKTOOL_WORLDWRITER_HPP
#define TPKTOOL_WORLDWRITER_HPP

#include "TexturePackWriter.hpp"

class WorldWriter : public TexturePackWriter
{
public:
    void Write(std::ofstream &stream, const std::shared_ptr<TexturePack> &texturePack) override;
};


#endif //TPKTOOL_WORLDWRITER_HPP
