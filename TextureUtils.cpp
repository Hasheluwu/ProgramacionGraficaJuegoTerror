#include "TextureUtils.h"

#include <SOIL2/SOIL2.h>
#include <glad/glad.h>

#include <iostream>
#include <string>

unsigned int TextureFromFile(const char* path, const std::string& directory)
{
    std::string texturePath = std::string(path);

    for (char& c : texturePath)
    {
        if (c == '\\')
            c = '/';
    }

    std::string filename = directory + "/" + texturePath;

    unsigned int textureID = SOIL_load_OGL_texture(
        filename.c_str(),
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y
    );

    if (textureID == 0)
    {
        std::cout << "ERROR TEXTURA: No se pudo cargar: " << filename << std::endl;
        std::cout << "SOIL2 dice: " << SOIL_last_result() << std::endl;
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "TEXTURA CARGADA: " << filename << std::endl;

    return textureID;
}