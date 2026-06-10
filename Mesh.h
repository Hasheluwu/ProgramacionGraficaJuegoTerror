#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

using namespace std;

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture
{
    unsigned int id;
    string type;
    string path;
};

class Mesh
{
public:
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    glm::vec3 materialColor;
    bool hasDiffuseTexture;

    unsigned int VAO;

    Mesh(
        vector<Vertex> vertices,
        vector<unsigned int> indices,
        vector<Texture> textures,
        glm::vec3 materialColor
    );

    void Draw(unsigned int shaderProgram);

private:
    unsigned int VBO;
    unsigned int EBO;

    void setupMesh();
};

#endif