#pragma once
#include <vector>
#include <queue>
#include <cmath>
#include <random>
#include <algorithm>
#include <glm/glm.hpp>

using namespace std;

// Estructura del Nodo para el A*
struct Nodo {
    glm::ivec2 pos;
    float costoG; // Costo desde el inicio
    float costoF; // CostoG + Heurística (Distancia al objetivo) + TEMPERATURA
    
    // Operador para la Priority Queue (menor costoF tiene prioridad)
    bool operator>(const Nodo& otro) const {
        return costoF > otro.costoF;
    }
};

class Pathfinding {
public:
    // VARIABLE CLAVE: Temperatura (0.0 = Robot perfecto, 1.0+ = Monstruo impredecible que explora)
    float temperatura = 0.5f; 

    vector<glm::ivec2> EncontrarCamino(glm::ivec2 inicio, glm::ivec2 objetivo, const vector<vector<int>>& mapa) {
        int filas = (int)mapa.size();
        int columnas = (int)mapa[0].size();

        // Matrices de control
        vector<vector<bool>> visitado(filas, vector<bool>(columnas, false));
        vector<vector<glm::ivec2>> padre(filas, vector<glm::ivec2>(columnas, glm::ivec2(-1, -1)));

        priority_queue<Nodo, vector<Nodo>, greater<Nodo>> frontera;
        frontera.push({inicio, 0.0f, 0.0f});

        // Direcciones de movimiento (Arriba, Abajo, Izquierda, Derecha)
        vector<glm::ivec2> direcciones = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

        // Generador de ruido aleatorio para la temperatura
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<float> dis(0.0f, temperatura);

        while (!frontera.empty()) {
            Nodo actual = frontera.top();
            frontera.pop();

            if (actual.pos == objetivo) {
                return ReconstruirCamino(padre, objetivo);
            }

            if (visitado[actual.pos.y][actual.pos.x]) continue;
            visitado[actual.pos.y][actual.pos.x] = true;

            for (auto dir : direcciones) {
                glm::ivec2 vecino = actual.pos + dir;

                // Si el vecino está dentro del mapa y NO es pared (1)
                if (vecino.y >= 0 && vecino.y < filas && vecino.x >= 0 && vecino.x < columnas && mapa[vecino.y][vecino.x] != 1) {
                    if (!visitado[vecino.y][vecino.x]) {
                        
                        float nuevoCostoG = actual.costoG + 1.0f;
                        // Heurística de Distancia Manhattan
                        float distanciaH = abs(vecino.x - objetivo.x) + abs(vecino.y - objetivo.y); 
                        
                        // EL TOQUE DE HARVARD/ANNEALING: Agregamos "ruido" al costo basado en la temperatura
                        float ruido = dis(gen); 
                        float nuevoCostoF = nuevoCostoG + distanciaH + ruido;

                        frontera.push({vecino, nuevoCostoG, nuevoCostoF});
                        padre[vecino.y][vecino.x] = actual.pos;
                    }
                }
            }
        }
        return {}; // Devuelve camino vacío si no hay ruta (encerrado)
    }

private:
    vector<glm::ivec2> ReconstruirCamino(const vector<vector<glm::ivec2>>& padre, glm::ivec2 objetivo) {
        vector<glm::ivec2> camino;
        glm::ivec2 actual = objetivo;
        while (actual.x != -1 && actual.y != -1) {
            camino.push_back(actual);
            actual = padre[actual.y][actual.x];
        }
        reverse(camino.begin(), camino.end());
        return camino; // El camino va desde el inicio hasta el objetivo
    }
};