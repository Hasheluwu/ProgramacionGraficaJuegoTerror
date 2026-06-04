#include "TextureUtils.h"
#include <SOIL2/SOIL2.h>
#include <glad/glad.h>
#include <iostream>
#include <string>

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
    {
        std::cout << "ERROR TEXTURA: " << filename << std::endl;
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    // Esto permite que las texturas se repitan cuando las UVs pasan de 0 a 1
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Filtros para que se vea mejor
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}