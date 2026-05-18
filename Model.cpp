#include "Model.h"
#include "Mesh.h"
#include "TextureUtils.h"

#include <iostream>

Model::Model(string const& path)
{
    loadModel(path);
}

void Model::Draw(unsigned int shaderProgram)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shaderProgram);
}

void Model::loadModel(string const& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR ASSIMP: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of("/\\"));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // ---------------- VERTICES ----------------
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex v;

        v.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        v.Normal = glm::vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z
        );

        if (mesh->mTextureCoords[0])
        {
            v.TexCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                1.0f - mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            v.TexCoords = glm::vec2(0.0f);
        }

        vertices.push_back(v);
    }

    // ---------------- INDICES ----------------
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // ---------------- TEXTURES ----------------
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        vector<Texture> diffuseMaps =
            loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");

        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    string typeName)
{
    vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        Texture texture;

        texture.id = TextureFromFile(str.C_Str(), directory);
        texture.type = typeName;
        texture.path = str.C_Str();

        textures.push_back(texture);
    }

    return textures;
}