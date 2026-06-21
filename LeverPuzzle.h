#pragma once
#include <array>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Model.h"
#include "Shader.h"
#include "AudioManager.h"

class LeverPuzzle {
public:
    LeverPuzzle();
    bool IsComplete() const { return puzzleResuelto; }

    void Init();

    void Update(
        const glm::vec3& playerPos,
        bool interactPressed,
        float deltaTime,
        AudioManager* audio
    );

    // Usa modelMat del mapa/casa.
    // Como los objetos ya vienen colocados desde Blender,
    // se dibujan con la misma matriz del sotano.
    void Draw(Shader& shader, const glm::mat4& mapModelMatrix);

    bool IsSolved() const;

private:
    std::unique_ptr<Model> leverBasesModel;
    std::array<std::unique_ptr<Model>, 4> leverMangoModels;

    std::unique_ptr<Model> buttonBaseModel;
    std::unique_ptr<Model> buttonModel;
    std::unique_ptr<Model> lightModel;
    bool puzzleResuelto = false;

    // Pivotes de giro en coordenadas del modelo Blender convertido a OpenGL.
    // NO llevan -1 en Y porque mapModelMatrix ya baja el mapa.
    std::array<glm::vec3, 4> leverPivotModelPositions;

    // Posiciones para interacción del jugador.
    // Estas sí están en coordenadas mundo.
    std::array<glm::vec3, 4> leverInteractPositions;

    glm::vec3 buttonInteractPosition;

    std::array<bool, 4> leverUp;
    std::array<bool, 4> solution;
    std::array<float, 4> leverAngles;

    bool solved;
    float errorBlinkTimer;

    float interactDistance;
    float buttonDistance;

    float angleUp;
    float angleDown;
    float animationSpeed;

private:
    void ToggleLever(int index, AudioManager* audio);
    void CheckPuzzle(AudioManager* audio);

    int GetNearestLever(const glm::vec3& playerPos) const;

    bool IsNearHorizontal(
        const glm::vec3& playerPos,
        const glm::vec3& objectPos,
        float distance
    ) const;

    float HorizontalDistance(
        const glm::vec3& a,
        const glm::vec3& b
    ) const;

    void DrawStaticModel(
        Shader& shader,
        Model& modelToDraw,
        const glm::mat4& mapModelMatrix
    );

    void DrawLeverMango(
        Shader& shader,
        int index,
        const glm::mat4& mapModelMatrix
    );

    void DrawPuzzleLight(
        Shader& shader,
        const glm::mat4& mapModelMatrix
    );

    void SetModelSolidColor(
        Model& modelToColor,
        const glm::vec3& color
    );


};