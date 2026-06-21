#include "LeverPuzzle.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <string>

static bool FileExists(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

LeverPuzzle::LeverPuzzle()
{
    solved = false;
    errorBlinkTimer = 0.0f;

    interactDistance = 3.0f;
    buttonDistance = 3.2f;

    // Asumimos que exportaste los mangos en posición ABAJO.
    // Abajo = 0 grados
    // Arriba = -60 grados
    //
    // Si gira al revés, cambiá angleUp a 60.0f.
    angleUp = 140.0f;
    angleDown = 0.0f;

    animationSpeed = 10.0f;

    for (int i = 0; i < 4; i++)
    {
        leverUp[i] = false;
        solution[i] = false;
        leverAngles[i] = angleDown;
    }
}

void LeverPuzzle::Init()
{
    std::cout << "\n=====================================\n";
    std::cout << "[LeverPuzzle] Cargando puzzle desde Blender...\n";
    std::cout << "=====================================\n";

    std::string basePath = "Resources/Models/palanca/";

    std::string pathBases = basePath + "palanca_base.obj";
    std::string pathMango1 = basePath + "palanca_mango1.obj";
    std::string pathMango2 = basePath + "palanca_mango2.obj";
    std::string pathMango3 = basePath + "palanca_mango3.obj";
    std::string pathMango4 = basePath + "palanca_mango4.obj";
    std::string pathButtonBase = basePath + "button_base.obj";
    std::string pathButton = basePath + "button.obj";
    std::string pathLight = basePath + "luz_puzzle.obj";

    std::cout << "[LeverPuzzle] Existe " << pathBases << "? "
        << (FileExists(pathBases) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathMango1 << "? "
        << (FileExists(pathMango1) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathMango2 << "? "
        << (FileExists(pathMango2) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathMango3 << "? "
        << (FileExists(pathMango3) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathMango4 << "? "
        << (FileExists(pathMango4) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathButtonBase << "? "
        << (FileExists(pathButtonBase) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathButton << "? "
        << (FileExists(pathButton) ? "SI" : "NO") << std::endl;

    std::cout << "[LeverPuzzle] Existe " << pathLight << "? "
        << (FileExists(pathLight) ? "SI" : "NO") << std::endl;

    leverBasesModel = std::make_unique<Model>(pathBases);

    leverMangoModels[0] = std::make_unique<Model>(pathMango1);
    leverMangoModels[1] = std::make_unique<Model>(pathMango2);
    leverMangoModels[2] = std::make_unique<Model>(pathMango3);
    leverMangoModels[3] = std::make_unique<Model>(pathMango4);

    buttonBaseModel = std::make_unique<Model>(pathButtonBase);
    buttonModel = std::make_unique<Model>(pathButton);
    lightModel = std::make_unique<Model>(pathLight);

    // ==================================================
    // Pivotes de giro
    // ==================================================
    leverPivotModelPositions[0] = glm::vec3(37.338f, 4.1385f, -118.37f);
    leverPivotModelPositions[1] = glm::vec3(39.124f, 4.1122f, -118.32f);
    leverPivotModelPositions[2] = glm::vec3(41.009f, 4.1266f, -118.31f);
    leverPivotModelPositions[3] = glm::vec3(42.947f, 4.1238f, -118.30f);

    // ==================================================
    // Posiciones de interacción
    // ==================================================
    leverInteractPositions[0] = glm::vec3(37.002f, 3.5863f - 1.0f, -118.77f);
    leverInteractPositions[1] = glm::vec3(38.790f, 3.5863f - 1.0f, -118.77f);
    leverInteractPositions[2] = glm::vec3(40.679f, 3.5863f - 1.0f, -118.77f);
    leverInteractPositions[3] = glm::vec3(42.617f, 3.5863f - 1.0f, -118.77f);

    buttonInteractPosition = glm::vec3(
        48.377f,
        2.8649f - 1.0f,
        -118.47f
    );

    // Solución:
    // true  = arriba
    // false = abajo
    solution[0] = true;   // palanca 1 arriba
    solution[1] = false;  // palanca 2 abajo
    solution[2] = false;  // palanca 3 abajo
    solution[3] = true;   // palanca 4 arriba

    std::cout << "[LeverPuzzle] Puzzle cargado correctamente.\n";
    std::cout << "=====================================\n\n";
}

void LeverPuzzle::Update(
    const glm::vec3& playerPos,
    bool interactPressed,
    float deltaTime,
    AudioManager* audio
)
{
    if (errorBlinkTimer > 0.0f)
    {
        errorBlinkTimer -= deltaTime;

        if (errorBlinkTimer < 0.0f)
            errorBlinkTimer = 0.0f;
    }

    // Animación suave
    for (int i = 0; i < 4; i++)
    {
        float targetAngle = leverUp[i] ? angleUp : angleDown;
        float t = std::min(deltaTime * animationSpeed, 1.0f);

        leverAngles[i] =
            leverAngles[i] + (targetAngle - leverAngles[i]) * t;
    }

    if (!interactPressed)
        return;

    if (solved)
    {
        std::cout << "[LeverPuzzle] Este puzzle ya esta resuelto.\n";
        return;
    }

    // Primero revisa el botón
    if (IsNearHorizontal(playerPos, buttonInteractPosition, buttonDistance))
    {
        CheckPuzzle(audio);
        return;
    }

    // Luego revisa palancas
    int nearestLever = GetNearestLever(playerPos);

    if (nearestLever != -1)
    {
        ToggleLever(nearestLever, audio);
    }
}

void LeverPuzzle::Draw(Shader& shader, const glm::mat4& mapModelMatrix)
{
    shader.use();

    // Bases estáticas: ya vienen colocadas desde Blender
    if (leverBasesModel)
    {
        DrawStaticModel(shader, *leverBasesModel, mapModelMatrix);
    }

    // Mangos animados: cada uno viene colocado desde Blender,
    // pero se rota alrededor de su pivote.
    for (int i = 0; i < 4; i++)
    {
        DrawLeverMango(shader, i, mapModelMatrix);
    }

    // Botón estático
    if (buttonBaseModel)
    {
        DrawStaticModel(shader, *buttonBaseModel, mapModelMatrix);
    }

    if (buttonModel)
    {
        DrawStaticModel(shader, *buttonModel, mapModelMatrix);
    }

    // Luz roja/verde
    DrawPuzzleLight(shader, mapModelMatrix);
}

bool LeverPuzzle::IsSolved() const
{
    return solved;
}

void LeverPuzzle::ToggleLever(int index, AudioManager* audio)
{
    if (index < 0 || index >= 4)
        return;

    leverUp[index] = !leverUp[index];

    if (audio) {
        audio->Play("palanca_sonido");
    }

    std::cout << "[LeverPuzzle] Palanca " << index + 1 << " ahora esta "
        << (leverUp[index] ? "ARRIBA" : "ABAJO") << ".\n";
}

void LeverPuzzle::CheckPuzzle(AudioManager* audio)
{
    bool correct = true;
    for (int i = 0; i < 4; i++)
    {
        if (leverUp[i] != solution[i])
        {
            correct = false;
            break;
        }
    }

    if (correct)
    {
        solved = true;
        puzzleResuelto = true;   // <--- AÑADE ESTA LÍNEA
        errorBlinkTimer = 0.0f;
        if (audio) audio->Play("correct");
        std::cout << "[LeverPuzzle] Puzzle completado. Luz verde activada.\n";
    }
    else
    {
        errorBlinkTimer = 0.7f;
        if (audio) audio->Play("error");
        std::cout << "[LeverPuzzle] Combinacion incorrecta.\n";
    }
}

int LeverPuzzle::GetNearestLever(const glm::vec3& playerPos) const
{
    int nearestIndex = -1;
    float nearestDistance = interactDistance;

    for (int i = 0; i < 4; i++)
    {
        float dist = HorizontalDistance(playerPos, leverInteractPositions[i]);

        if (dist <= nearestDistance)
        {
            nearestDistance = dist;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

bool LeverPuzzle::IsNearHorizontal(
    const glm::vec3& playerPos,
    const glm::vec3& objectPos,
    float distance
) const
{
    return HorizontalDistance(playerPos, objectPos) <= distance;
}

float LeverPuzzle::HorizontalDistance(
    const glm::vec3& a,
    const glm::vec3& b
) const
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;

    return std::sqrt(dx * dx + dz * dz);
}

void LeverPuzzle::DrawStaticModel(
    Shader& shader,
    Model& modelToDraw,
    const glm::mat4& mapModelMatrix
)
{
    glUniformMatrix4fv(
        glGetUniformLocation(shader.ID, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(mapModelMatrix)
    );

    modelToDraw.Draw(shader.ID);
}

void LeverPuzzle::DrawLeverMango(
    Shader& shader,
    int index,
    const glm::mat4& mapModelMatrix
)
{
    if (index < 0 || index >= 4)
        return;

    if (!leverMangoModels[index])
        return;

    glm::vec3 pivot = leverPivotModelPositions[index];

    glm::mat4 model = mapModelMatrix;

    model = glm::translate(model, pivot);

    model = glm::rotate(
        model,
        glm::radians(leverAngles[index]),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    model = glm::translate(model, -pivot);

    glUniformMatrix4fv(
        glGetUniformLocation(shader.ID, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model)
    );

    leverMangoModels[index]->Draw(shader.ID);
}

void LeverPuzzle::DrawPuzzleLight(
    Shader& shader,
    const glm::mat4& mapModelMatrix
)
{
    if (!lightModel)
        return;

    glm::vec3 color;

    if (solved)
    {
        color = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    else
    {
        color = glm::vec3(1.0f, 0.0f, 0.0f);

        if (errorBlinkTimer > 0.0f)
            color = glm::vec3(1.0f, 0.15f, 0.15f);
    }

    SetModelSolidColor(*lightModel, color);

    DrawStaticModel(shader, *lightModel, mapModelMatrix);
}

void LeverPuzzle::SetModelSolidColor(
    Model& modelToColor,
    const glm::vec3& color
)
{
    for (auto& mesh : modelToColor.meshes)
    {
        mesh.materialColor = color;
        mesh.hasDiffuseTexture = false;
    }
}