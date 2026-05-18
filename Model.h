#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Mesh.h"

using namespace std;

class Model
{
public:
    Model(string const& path);

    void Draw(unsigned int shaderProgram);

private:
    vector<Mesh> meshes;
    string directory;

    void loadModel(string const& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    vector<Texture> loadMaterialTextures(aiMaterial* mat,
        aiTextureType type,
        string typeName);
};