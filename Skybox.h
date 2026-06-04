#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>

class Skybox
{
public:
    unsigned int textureID;
    unsigned int VAO, VBO;

    Skybox(std::vector<std::string> faces);

    void Draw(unsigned int shader);

private:
    unsigned int loadCubemap(std::vector<std::string> faces);
};