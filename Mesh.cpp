#include "Mesh.h"

#include <glad/glad.h>
#include <iostream>
#include <string>

Mesh::Mesh(
    vector<Vertex> vertices,
    vector<unsigned int> indices,
    vector<Texture> textures,
    glm::vec3 materialColor)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->materialColor = materialColor;

    this->hasDiffuseTexture = false;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        if (textures[i].type == "texture_diffuse")
        {
            this->hasDiffuseTexture = true;
            break;
        }
    }

    setupMesh();
}

void Mesh::Draw(unsigned int shaderProgram)
{
    glUseProgram(shaderProgram);

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "materialColor"),
        1,
        &materialColor[0]
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "hasDiffuseTexture"),
        hasDiffuseTexture ? 1 : 0
    );

    unsigned int diffuseNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);

        string number;
        string name = textures[i].type;

        if (name == "texture_diffuse")
        {
            number = std::to_string(diffuseNr++);
        }

        string uniformName = name + number;

        glUniform1i(
            glGetUniformLocation(shaderProgram, uniformName.c_str()),
            i
        );

        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glBindVertexArray(VAO);

    glDrawElements(
        GL_TRIANGLES,
        (GLsizei)indices.size(),
        GL_UNSIGNED_INT,
        0
    );

    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        &vertices[0],
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        &indices[0],
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, Normal)
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, TexCoords)
    );

    glBindVertexArray(0);
}