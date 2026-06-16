#pragma once
#ifndef ANIMATED_MODEL_H
#define ANIMATED_MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "TextureUtils.h"

using namespace std;

// ============================================================
//  Límites del sistema de huesos
// ============================================================
#define MAX_BONES         100
#define MAX_BONES_PER_VERTEX 4

// ============================================================
//  Vertex con datos de skinning
// ============================================================
struct AnimatedVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    int       BoneIDs[MAX_BONES_PER_VERTEX];
    float     Weights[MAX_BONES_PER_VERTEX];

    AnimatedVertex() {
        for (int i = 0; i < MAX_BONES_PER_VERTEX; i++) {
            BoneIDs[i] = 0;
            Weights[i] = 0.0f;
        }
    }
};

// ============================================================
//  Textura
// ============================================================
struct AnimTexture {
    unsigned int id;
    string       type;
    string       path;
};

// ============================================================
//  Info de un hueso: offset matrix (bind-pose inversa)
// ============================================================
struct BoneInfo {
    glm::mat4 offsetMatrix;
    glm::mat4 finalTransform;
};

// ============================================================
//  Keyframes por canal
// ============================================================
struct KeyPosition { glm::vec3 position; double time; };
struct KeyRotation { glm::quat rotation; double time; };
struct KeyScale { glm::vec3 scale;    double time; };

// ============================================================
//  Canal de animación para un hueso
// ============================================================
struct BoneChannel {
    string              boneName;
    vector<KeyPosition> positions;
    vector<KeyRotation> rotations;
    vector<KeyScale>    scales;
};

// ============================================================
//  Una animación completa
// ============================================================
struct Animation {
    string              name;
    double              duration;
    double              ticksPerSecond;
    vector<BoneChannel> channels;
};

// ============================================================
//  Nodo del árbol de huesos (jerarquía) - PÚBLICO
// ============================================================
struct BoneNode {
    string         name;
    glm::mat4      localTransform;
    vector<int>    children;
};

// ============================================================
//  Mesh animado (GPU)
// ============================================================
class AnimatedMesh {
public:
    vector<AnimatedVertex> vertices;
    vector<unsigned int>   indices;
    vector<AnimTexture>    textures;
    glm::vec3              materialColor;
    bool                   hasDiffuseTexture;
    unsigned int           VAO;

    AnimatedMesh(vector<AnimatedVertex> v, vector<unsigned int> i,
        vector<AnimTexture> t, glm::vec3 color)
        : vertices(v), indices(i), textures(t), materialColor(color),
        hasDiffuseTexture(false), VAO(0), VBO(0), EBO(0)
    {
        for (auto& tex : textures)
            if (tex.type == "texture_diffuse") { hasDiffuseTexture = true; break; }
        setupMesh();
    }

    void Draw(unsigned int shader) {
        glUseProgram(shader);
        glUniform3fv(glGetUniformLocation(shader, "materialColor"), 1, &materialColor[0]);
        glUniform1i(glGetUniformLocation(shader, "hasDiffuseTexture"), hasDiffuseTexture ? 1 : 0);

        unsigned int diffNr = 1;
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            string name = textures[i].type;
            string num = (name == "texture_diffuse") ? to_string(diffNr++) : "1";
            glUniform1i(glGetUniformLocation(shader, (name + num).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(AnimatedVertex),
            vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
            indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
            (void*)offsetof(AnimatedVertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
            (void*)offsetof(AnimatedVertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
            (void*)offsetof(AnimatedVertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, MAX_BONES_PER_VERTEX, GL_INT, sizeof(AnimatedVertex),
            (void*)offsetof(AnimatedVertex, BoneIDs));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, MAX_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
            (void*)offsetof(AnimatedVertex, Weights));

        glBindVertexArray(0);
    }
};

// ============================================================
//  Conversor de matrices Assimp -> GLM
// ============================================================
static glm::mat4 AiToGlm(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
static glm::vec3 AiToGlm(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
static glm::quat AiToGlm(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }

// ============================================================
//  AnimatedModel
// ============================================================
class AnimatedModel {
public:
    int  currentAnim = 0;
    float animTime = 0.0f;
    bool  loopAnim = true;

    vector<glm::mat4> boneMatrices;

    AnimatedModel() { boneMatrices.resize(MAX_BONES, glm::mat4(1.0f)); }

    bool LoadModel(const string& path) {
        directory = path.substr(0, path.find_last_of("/\\"));

        Assimp::Importer imp;
        const aiScene* scene = imp.ReadFile(path,
            aiProcess_Triangulate | aiProcess_GenNormals |
            aiProcess_FlipUVs | aiProcess_LimitBoneWeights);

        if (!scene || !scene->mRootNode) {
            cout << "[AnimatedModel] Error cargando modelo: " << imp.GetErrorString() << endl;
            return false;
        }

        globalInverseTransform = glm::inverse(AiToGlm(scene->mRootNode->mTransformation));

        processNode(scene->mRootNode, scene);
        loadAnimationsFromScene(scene, path);

        // Determinar el nodo raíz real
        string rootName = scene->mRootNode->mName.C_Str();
        auto it = boneNameToIndex.find(rootName);
        if (it != boneNameToIndex.end()) {
            rootName = it->first;
        }
        else {
            string prefixed = "mixamorig:" + rootName;
            if (boneNameToIndex.find(prefixed) != boneNameToIndex.end())
                rootName = prefixed;
        }
        if (nodeIndex.count(rootName))
            rootNodeIndex = nodeIndex[rootName];
        else
            rootNodeIndex = 0;

        cout << "[AnimatedModel] Nodo raiz: " << boneHierarchy[rootNodeIndex].name
            << " (indice " << rootNodeIndex << ")" << endl;

        cout << "[AnimatedModel] Modelo cargado: " << meshes.size() << " meshes, "
            << boneInfoMap.size() << " huesos." << endl;
        return true;
    }

    bool LoadAnimation(const string& path, const string& name) {
        Assimp::Importer imp;
        const aiScene* scene = imp.ReadFile(path,
            aiProcess_Triangulate | aiProcess_LimitBoneWeights);

        if (!scene) {
            cout << "[AnimatedModel] Error cargando animacion: " << imp.GetErrorString() << endl;
            return false;
        }

        loadAnimationsFromScene(scene, path, name);
        cout << "[AnimatedModel] Animacion '" << name << "' cargada. Total: "
            << animations.size() << endl;
        return true;
    }

    void SetAnimation(int index, bool loop = true) {
        if (index < 0 || index >= (int)animations.size()) return;
        if (currentAnim == index) return;
        currentAnim = index;
        animTime = 0.0f;
        loopAnim = loop;
    }

    void SetAnimation(const string& name, bool loop = true) {
        for (int i = 0; i < (int)animations.size(); i++) {
            if (animations[i].name == name) { SetAnimation(i, loop); return; }
        }
        cout << "[AnimatedModel] Animacion no encontrada: " << name << endl;
    }

    void Update(float deltaTime) {
        if (animations.empty()) return;

        Animation& anim = animations[currentAnim];
        double ticksPerSec = anim.ticksPerSecond > 0.0 ? anim.ticksPerSecond : 25.0;
        double totalTime = anim.duration / ticksPerSec;
        if (totalTime <= 0.0) return;

        const float FRAME_OFFSET = 0.05f;  // salta los primeros 50ms (frame neutro de Mixamo)

        animTime += deltaTime;
        if (animTime >= (float)totalTime)
            animTime = FRAME_OFFSET;
        if (animTime < FRAME_OFFSET)
            animTime = FRAME_OFFSET;

        double ticks = animTime * ticksPerSec;

        // Ya no llenamos con identidad; calculateBoneTransforms se encarga de actualizar
        calculateBoneTransforms(anim, ticks, globalInverseTransform, rootNodeIndex);

        // Depuración: contar matrices no identidad cada 2 segundos
        static float debugTimer = 0.0f;
        debugTimer += deltaTime;
        if (debugTimer >= 2.0f) {
            debugTimer = 0.0f;
            int nonIdentity = 0;
            for (int i = 0; i < (int)boneInfoMap.size(); i++) {
                if (boneMatrices[i] != glm::mat4(1.0f)) nonIdentity++;
            }
            std::cout << "[Anim] Matrices no identidad: " << nonIdentity << " / " << boneInfoMap.size() << std::endl;
        }
    }

    void Draw(unsigned int shader) {
        for (int i = 0; i < (int)boneMatrices.size() && i < MAX_BONES; i++) {
            string uni = "boneMatrices[" + to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(shader, uni.c_str()),
                1, GL_FALSE, glm::value_ptr(boneMatrices[i]));
        }
        for (auto& mesh : meshes) mesh.Draw(shader);
    }

    int  AnimCount() const { return (int)animations.size(); }
    bool HasAnims()  const { return !animations.empty(); }
    const string& AnimName(int i) const { return animations[i].name; }

    // Métodos de depuración
    const unordered_map<string, int>& GetBoneMap() const { return boneNameToIndex; }
    const vector<Animation>& GetAnimations() const { return animations; }
    const vector<BoneNode>& GetBoneHierarchy() const { return boneHierarchy; }

    void PrintHierarchy() const {
        std::cout << "--- JERARQUÍA DE NODOS (" << boneHierarchy.size() << " nodos) ---" << std::endl;
        for (const auto& node : boneHierarchy)
            std::cout << node.name << std::endl;
        std::cout << "------------------------------------------" << std::endl;
    }

private:
    vector<AnimatedMesh>              meshes;
    string                            directory;
    unordered_map<string, int>        boneNameToIndex;
    vector<BoneInfo>                  boneInfoMap;
    vector<Animation>                 animations;
    glm::mat4                         globalInverseTransform;

    vector<BoneNode> boneHierarchy;
    unordered_map<string, int> nodeIndex;
    int rootNodeIndex = 0;

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
            meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
        buildHierarchy(node);
    }

    void buildHierarchy(aiNode* node) {
        string name = node->mName.C_Str();

        // NO filtramos los nodos auxiliares; los conservamos todos

        auto it = boneNameToIndex.find(name);
        if (it != boneNameToIndex.end()) {
            name = it->first;                // ya tiene mixamorig:
        }
        else {
            string prefixed = "mixamorig:" + name;
            if (boneNameToIndex.find(prefixed) != boneNameToIndex.end())
                name = prefixed;
        }

        if (nodeIndex.count(name)) return;
        int idx = (int)boneHierarchy.size();
        nodeIndex[name] = idx;
        BoneNode bn;
        bn.name = name;
        bn.localTransform = AiToGlm(node->mTransformation);
        boneHierarchy.push_back(bn);

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            string childName = node->mChildren[i]->mName.C_Str();
            buildHierarchy(node->mChildren[i]);

            string childKey = childName;
            if (!nodeIndex.count(childKey)) {
                string prefixedChild = "mixamorig:" + childKey;
                if (nodeIndex.count(prefixedChild))
                    childKey = prefixedChild;
            }
            if (nodeIndex.count(childKey))
                boneHierarchy[idx].children.push_back(nodeIndex[childKey]);
        }
    }

    AnimatedMesh processMesh(aiMesh* mesh, const aiScene* scene) {
        vector<AnimatedVertex> vertices(mesh->mNumVertices);
        vector<unsigned int>   indices;
        vector<AnimTexture>    textures;
        glm::vec3              color(1.0f);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            vertices[i].Position = AiToGlm(mesh->mVertices[i]);
            vertices[i].Normal = mesh->HasNormals()
                ? AiToGlm(mesh->mNormals[i])
                : glm::vec3(0, 1, 0);
            vertices[i].TexCoords = mesh->mTextureCoords[0]
                ? glm::vec2(mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y)
                : glm::vec2(0);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);

        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            aiBone* bone = mesh->mBones[b];
            string  bname = bone->mName.C_Str();

            if (!boneNameToIndex.count(bname)) {
                boneNameToIndex[bname] = (int)boneInfoMap.size();
                BoneInfo bi;
                bi.offsetMatrix = AiToGlm(bone->mOffsetMatrix);
                bi.finalTransform = glm::mat4(1.0f);
                boneInfoMap.push_back(bi);
            }
            int bIdx = boneNameToIndex[bname];

            for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                int   vIdx = bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;
                for (int s = 0; s < MAX_BONES_PER_VERTEX; s++) {
                    if (vertices[vIdx].Weights[s] == 0.0f) {
                        vertices[vIdx].BoneIDs[s] = bIdx;
                        vertices[vIdx].Weights[s] = weight;
                        break;
                    }
                }
            }
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiColor3D dc(1, 1, 1);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, dc);
            color = glm::vec3(dc.r, dc.g, dc.b);

            for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++) {
                aiString str;
                mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
                AnimTexture t;
                t.id = TextureFromFile(str.C_Str(), directory);
                t.type = "texture_diffuse";
                t.path = str.C_Str();
                if (t.id) textures.push_back(t);
            }
        }

        return AnimatedMesh(vertices, indices, textures, color);
    }

    void loadAnimationsFromScene(const aiScene* scene, const string& path,
        const string& forceName = "") {
        for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
            aiAnimation* aiAnim = scene->mAnimations[a];
            Animation anim;
            anim.name = forceName.empty() ? aiAnim->mName.C_Str() : forceName;
            anim.duration = aiAnim->mDuration;
            anim.ticksPerSecond = aiAnim->mTicksPerSecond;

            for (unsigned int c = 0; c < aiAnim->mNumChannels; c++) {
                aiNodeAnim* ch = aiAnim->mChannels[c];
                BoneChannel bc;
                string rawName = ch->mNodeName.C_Str();
                size_t pos = rawName.find("_$AssimpFbx$_");
                if (pos != string::npos)
                    rawName = rawName.substr(0, pos);
                bc.boneName = rawName;

                for (unsigned int k = 0; k < ch->mNumPositionKeys; k++)
                    bc.positions.push_back({ AiToGlm(ch->mPositionKeys[k].mValue),
                                            ch->mPositionKeys[k].mTime });
                for (unsigned int k = 0; k < ch->mNumRotationKeys; k++)
                    bc.rotations.push_back({ AiToGlm(ch->mRotationKeys[k].mValue),
                                            ch->mRotationKeys[k].mTime });
                for (unsigned int k = 0; k < ch->mNumScalingKeys; k++)
                    bc.scales.push_back({ AiToGlm(ch->mScalingKeys[k].mValue),
                                         ch->mScalingKeys[k].mTime });
                anim.channels.push_back(bc);
            }
            animations.push_back(anim);
        }
    }

    glm::vec3 interpPosition(const BoneChannel& ch, double t) {
        if (ch.positions.size() == 1) return ch.positions[0].position;
        for (size_t i = 0; i < ch.positions.size() - 1; i++) {
            if (t < ch.positions[i + 1].time) {
                float f = (float)((t - ch.positions[i].time) /
                    (ch.positions[i + 1].time - ch.positions[i].time));
                return glm::mix(ch.positions[i].position, ch.positions[i + 1].position, f);
            }
        }
        return ch.positions.back().position;
    }

    // Interpolación de rotación con camino más corto
    glm::quat interpRotation(const BoneChannel& ch, double t) {
        if (ch.rotations.size() == 1) return ch.rotations[0].rotation;
        for (size_t i = 0; i < ch.rotations.size() - 1; i++) {
            if (t < ch.rotations[i + 1].time) {
                float f = (float)((t - ch.rotations[i].time) /
                    (ch.rotations[i + 1].time - ch.rotations[i].time));
                glm::quat a = ch.rotations[i].rotation;
                glm::quat b = ch.rotations[i + 1].rotation;
                if (glm::dot(a, b) < 0.0f) b = -b;          // camino más corto
                return glm::normalize(glm::slerp(a, b, f));
            }
        }
        return ch.rotations.back().rotation;
    }

    glm::vec3 interpScale(const BoneChannel& ch, double t) {
        if (ch.scales.size() == 1) return ch.scales[0].scale;
        for (size_t i = 0; i < ch.scales.size() - 1; i++) {
            if (t < ch.scales[i + 1].time) {
                float f = (float)((t - ch.scales[i].time) /
                    (ch.scales[i + 1].time - ch.scales[i].time));
                return glm::mix(ch.scales[i].scale, ch.scales[i + 1].scale, f);
            }
        }
        return ch.scales.back().scale;
    }

    void calculateBoneTransforms(const Animation& anim, double ticks,
        const glm::mat4& parentTransform, int nodeIdx = 0)
    {
        if (nodeIdx < 0 || nodeIdx >= (int)boneHierarchy.size()) return;
        const BoneNode& node = boneHierarchy[nodeIdx];

        glm::mat4 nodeTransform = node.localTransform;

        for (auto& ch : anim.channels) {
            if (ch.boneName == node.name || ch.boneName == "mixamorig:" + node.name) {
                glm::mat4 T = glm::translate(glm::mat4(1.0f), interpPosition(ch, ticks));
                glm::mat4 R = glm::toMat4(interpRotation(ch, ticks));
                glm::mat4 S = glm::scale(glm::mat4(1.0f), interpScale(ch, ticks));
                nodeTransform = T * R * S;
                break;
            }
        }

       

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        auto it = boneNameToIndex.find(node.name);
        if (it == boneNameToIndex.end()) {
            size_t pos = node.name.find(':');
            if (pos != string::npos) {
                it = boneNameToIndex.find(node.name.substr(pos + 1));
            }
        }
        if (it != boneNameToIndex.end()) {
            int idx = it->second;
            boneMatrices[idx] = globalInverseTransform
                * globalTransform
                * boneInfoMap[idx].offsetMatrix;
        }

        for (int child : node.children)
            calculateBoneTransforms(anim, ticks, globalTransform, child);
    }
};

#endif