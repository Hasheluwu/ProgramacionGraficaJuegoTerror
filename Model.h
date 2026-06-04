#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "Mesh.h"

using namespace std;

class Model
{
public:
    vector<Texture> textures_loaded;
    vector<Mesh> meshes;
    string directory;

    Model(string const& path);

    void Draw(unsigned int shaderProgram);

private:
    // Carga principal del modelo
    void loadModel(string const& path);

    // Recorre los nodos del modelo
    void processNode(aiNode* node, const aiScene* scene);

    // Procesa cada mesh
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // Carga texturas del material
    vector<Texture> loadMaterialTextures(
        aiMaterial* mat,
        aiTextureType type,
        string typeName
    );

    // Lee el .mtl y detecta escalas tipo:
    // map_Kd -s 4 30 1 textura.jpg
    void loadMtlTextureScales(const string& objPath);

    // Obtiene solo el nombre del archivo desde una ruta
    string getFileNameOnly(const string& path);

    // Guarda la escala UV por material
    // Ejemplo:
    // Material -> glm::vec2(4.0f, 30.0f)
    unordered_map<string, glm::vec2> materialTextureScale;
};

#endif