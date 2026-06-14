#pragma once
#include <vector>
#include <queue>
#include <cmath>
#include <random>
#include <algorithm>
#include <functional>
#include <glm/glm.hpp>

using namespace std;

struct Nodo {
    glm::ivec2 pos;
    float costoG;
    float costoF;

    bool operator>(const Nodo& otro) const {
        return costoF > otro.costoF;
    }
};

class Pathfinding {
public:
    float temperatura = 0.5f;
    float penalizacionPared = 0.8f;   // coste extra por cada celda adyacente a un muro

    vector<glm::ivec2> PlanificarRuta(glm::ivec2 inicio, glm::ivec2 objetivo,
        const vector<vector<int>>& mapa, float temp)
    {
        int filas = (int)mapa.size();
        int columnas = (int)mapa[0].size();

        // Precalcular qué celdas son adyacentes a una pared (1)
        vector<vector<bool>> adyacenteAPared(filas, vector<bool>(columnas, false));
        for (int f = 0; f < filas; ++f) {
            for (int c = 0; c < columnas; ++c) {
                if (mapa[f][c] == 1) continue;
                // Revisar 4 vecinos
                bool tocaPared = false;
                if (f > 0 && mapa[f - 1][c] == 1) tocaPared = true;
                else if (f < filas - 1 && mapa[f + 1][c] == 1) tocaPared = true;
                else if (c > 0 && mapa[f][c - 1] == 1) tocaPared = true;
                else if (c < columnas - 1 && mapa[f][c + 1] == 1) tocaPared = true;
                adyacenteAPared[f][c] = tocaPared;
            }
        }

        size_t seed = hash<int>()(objetivo.x * 73856093 ^ objetivo.y * 19349663) ^ (int)(temp * 1000.0f);
        mt19937 gen((unsigned int)seed);

        auto cellNoise = [&](int x, int y) -> float {
            size_t h = hash<int>()(x * 1103515245 + y * 12345);
            mt19937 localGen((unsigned int)(h ^ seed));
            uniform_real_distribution<float> dis(0.0f, 1.0f);
            return dis(localGen);
            };

        vector<vector<bool>> visitado(filas, vector<bool>(columnas, false));
        vector<vector<glm::ivec2>> padre(filas, vector<glm::ivec2>(columnas, glm::ivec2(-1, -1)));

        priority_queue<Nodo, vector<Nodo>, greater<Nodo>> frontera;
        frontera.push({ inicio, 0.0f, 0.0f });

        vector<glm::ivec2> direcciones = { {0, 1}, {0, -1}, {1, 0}, {-1, 0} };

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

                if (vecino.y >= 0 && vecino.y < filas && vecino.x >= 0 && vecino.x < columnas
                    && mapa[vecino.y][vecino.x] != 1)
                {
                    if (!visitado[vecino.y][vecino.x]) {
                        float costoExtra = 0.0f;
                        if (adyacenteAPared[vecino.y][vecino.x]) {
                            costoExtra = penalizacionPared;
                        }
                        float nuevoCostoG = actual.costoG + 1.0f + costoExtra;
                        float distanciaH = (float)(abs(vecino.x - objetivo.x) + abs(vecino.y - objetivo.y));
                        float ruido = cellNoise(vecino.x, vecino.y) * temp;
                        float nuevoCostoF = nuevoCostoG + distanciaH + ruido;

                        frontera.push({ vecino, nuevoCostoG, nuevoCostoF });
                        padre[vecino.y][vecino.x] = actual.pos;
                    }
                }
            }
        }
        return {};
    }

    vector<glm::ivec2> SuavizarCamino(const vector<glm::ivec2>& caminoCrudo,
        const vector<vector<int>>& mapa)
    {
        if (caminoCrudo.size() <= 2) return caminoCrudo;

        vector<glm::ivec2> suavizado;
        suavizado.push_back(caminoCrudo.front());

        size_t indexActual = 0;
        while (indexActual < caminoCrudo.size() - 1) {
            size_t mejor = indexActual + 1;
            for (size_t i = indexActual + 2; i < caminoCrudo.size(); ++i) {
                if (LineaDeVision(caminoCrudo[indexActual], caminoCrudo[i], mapa)) {
                    mejor = i;
                }
            }
            suavizado.push_back(caminoCrudo[mejor]);
            indexActual = mejor;
        }
        return suavizado;
    }

private:
    vector<glm::ivec2> ReconstruirCamino(const vector<vector<glm::ivec2>>& padre,
        glm::ivec2 objetivo)
    {
        vector<glm::ivec2> camino;
        glm::ivec2 actual = objetivo;
        while (actual.x != -1 && actual.y != -1) {
            camino.push_back(actual);
            actual = padre[actual.y][actual.x];
        }
        reverse(camino.begin(), camino.end());
        return camino;
    }

    bool LineaDeVision(glm::ivec2 desde, glm::ivec2 hasta,
        const vector<vector<int>>& mapa)
    {
        int x0 = desde.x, y0 = desde.y;
        int x1 = hasta.x, y1 = hasta.y;

        int dx = abs(x1 - x0);
        int dy = -abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        int x = x0, y = y0;
        while (true) {
            if (mapa[y][x] == 1) return false;
            if (x == x1 && y == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x += sx; }
            if (e2 <= dx) { err += dx; y += sy; }
        }
        return true;
    }
};