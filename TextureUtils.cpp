#include "TextureUtils.h"
#include <SOIL2/SOIL2.h>
#include <iostream>

unsigned int TextureFromFile(const char* path, const std::string& directory)
{
    std::string filename = directory + "/" + std::string(path);

    unsigned int textureID =
        SOIL_load_OGL_texture(
            filename.c_str(),
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_INVERT_Y
        );

    if (textureID == 0)
        std::cout << "ERROR TEXTURA: " << filename << std::endl;

    return textureID;
}