#pragma once
#include <vector>
#include <queue>
#include <cmath>
#include <random>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>

using namespace std;

// ============================================================
//  Nodo para la cola de prioridad de A*
// ============================================================
struct Nodo {
    glm::ivec2 pos;
    float costoG;
    float costoF;

    bool operator>(const Nodo& otro) const {
        return costoF > otro.costoF;
    }
};

// ============================================================
//  Hash para usar glm::ivec2 como clave de unordered_map
// ============================================================
struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const {
        return hash<int>()(v.x * 73856093) ^ hash<int>()(v.y * 19349663);
    }
};

// ============================================================
//  Clase principal de pathfinding
// ============================================================
class Pathfinding {
public:

    // --- Parámetros tunables ---
    float temperatura = 0.5f;  // 0 = determinista, 1 = muy aleatorio
    float penalizacionPared = 2.5f;  // coste extra por celda adyacente a muro (8 dirs)
    float pesoOctile = 1.05f; // peso de la heurística (>1 = weighted A*, más rápido y algo subóptimo)

    // --------------------------------------------------------
    //  PlanificarRuta
    //  A* con heurística Octile + penalización de paredes en 8 dirs
    //  + ruido de temperatura por celda para variabilidad de ruta
    // --------------------------------------------------------
    vector<glm::ivec2> PlanificarRuta(
        glm::ivec2 inicio,
        glm::ivec2 objetivo,
        const vector<vector<int>>& mapa,
        float temp)
    {
        int filas = (int)mapa.size();
        int columnas = (int)mapa[0].size();

        if (!EsTransitable(inicio, mapa, filas, columnas)) return {};
        if (!EsTransitable(objetivo, mapa, filas, columnas)) return {};

        // --- Precalcular mapa de proximidad a paredes (8 direcciones) ---
        // Cuántos vecinos inmediatos (8-dirs) son pared — más vecinos = más penalización
        vector<vector<float>> costoPared(filas, vector<float>(columnas, 0.0f));
        const glm::ivec2 dirs8[8] = {
            {0,1},{0,-1},{1,0},{-1,0},
            {1,1},{1,-1},{-1,1},{-1,-1}
        };
        for (int f = 0; f < filas; ++f) {
            for (int c = 0; c < columnas; ++c) {
                if (mapa[f][c] == 1) continue;
                int vecPared = 0;
                for (auto& d : dirs8) {
                    int nf = f + d.y, nc = c + d.x;
                    if (nf >= 0 && nf < filas && nc >= 0 && nc < columnas && mapa[nf][nc] == 1)
                        ++vecPared;
                }
                // Escala: 1 vecino-pared = penalizacionPared, 8 = 8x
                costoPared[f][c] = vecPared * penalizacionPared;
            }
        }

        // --- Semilla determinista basada en origen + destino + temperatura ---
        // Esto hace que la misma petición produzca el mismo camino (reproducible),
        // pero cambia si cambia el destino o la temperatura.
        size_t seed = (size_t)(inicio.x * 1000003)
            ^ (size_t)(inicio.y * 999983)
            ^ (size_t)(objetivo.x * 73856093)
            ^ (size_t)(objetivo.y * 19349663)
            ^ (size_t)(temp * 100000.0f);

        // Ruido por celda: determinista dado el seed, distinto para cada (x,y)
        auto celdaNoise = [&](int x, int y) -> float {
            size_t h = (size_t)(x * 1103515245 + 12345)
                ^ (size_t)(y * 1664525 + 1013904223)
                ^ seed;
            // Mapear a [0,1]
            return (float)((h >> 16) & 0xFFFF) / 65535.0f;
            };

        // --- Estructuras A* ---
        // Usamos unordered_map para padre y costoG (evita inicializar array gigante)
        unordered_map<glm::ivec2, glm::ivec2, IVec2Hash> padre;
        unordered_map<glm::ivec2, float, IVec2Hash> costoG;
        unordered_map<glm::ivec2, bool, IVec2Hash> visitado;

        priority_queue<Nodo, vector<Nodo>, greater<Nodo>> frontera;

        costoG[inicio] = 0.0f;
        frontera.push({ inicio, 0.0f, HeuristicaOctile(inicio, objetivo) });

        // Solo 4 direcciones cardinales para el movimiento
        // (la matriz es densa y el modelo Blender es ortogonal;
        //  diagonales generarían cortes de esquina)
        const glm::ivec2 dirs4[4] = { {0,1},{0,-1},{1,0},{-1,0} };

        while (!frontera.empty()) {
            Nodo actual = frontera.top();
            frontera.pop();

            if (actual.pos == objetivo) {
                return ReconstruirCamino(padre, objetivo, inicio);
            }

            if (visitado.count(actual.pos)) continue;
            visitado[actual.pos] = true;

            for (auto& dir : dirs4) {
                glm::ivec2 vecino = actual.pos + dir;

                if (!EsTransitable(vecino, mapa, filas, columnas)) continue;
                if (visitado.count(vecino))                         continue;

                // Coste de movimiento: 1 base + penalización de pared + ruido de temperatura
                float movCosto = 1.0f
                    + costoPared[vecino.y][vecino.x]
                    + celdaNoise(vecino.x, vecino.y) * temp;

                float nuevoG = actual.costoG + movCosto;

                auto it = costoG.find(vecino);
                if (it == costoG.end() || nuevoG < it->second) {
                    costoG[vecino] = nuevoG;
                    float h = HeuristicaOctile(vecino, objetivo) * pesoOctile;
                    frontera.push({ vecino, nuevoG, nuevoG + h });
                    padre[vecino] = actual.pos;
                }
            }
        }
        return {}; // Sin camino
    }

    // --------------------------------------------------------
    //  SuavizarCamino
    //  String-pulling con Bresenham robusto (revisa esquinas)
    //  Solo crea atajos si la línea de visión es completamente libre
    // --------------------------------------------------------
    vector<glm::ivec2> SuavizarCamino(
        const vector<glm::ivec2>& caminoCrudo,
        const vector<vector<int>>& mapa)
    {
        if (caminoCrudo.size() <= 2) return caminoCrudo;

        vector<glm::ivec2> suavizado;
        suavizado.push_back(caminoCrudo.front());

        size_t indexActual = 0;
        while (indexActual < caminoCrudo.size() - 1) {
            // Buscar el nodo más lejano con línea de visión limpia
            size_t mejor = indexActual + 1;
            for (size_t i = caminoCrudo.size() - 1; i > indexActual + 1; --i) {
                if (LineaDeVisionSegura(caminoCrudo[indexActual], caminoCrudo[i], mapa)) {
                    mejor = i;
                    break;
                }
            }
            suavizado.push_back(caminoCrudo[mejor]);
            indexActual = mejor;
        }
        return suavizado;
    }

    // --------------------------------------------------------
    //  CalcularDireccionSteering
    //  En lugar de saltar de nodo a nodo, devuelve una dirección
    //  suavizada hacia el próximo punto de la ruta.
    //  Llámalo cada frame para obtener el vector de movimiento.
    //
    //  posActual      : posición 3D actual del monstruo (xz plano)
    //  ruta           : vector de posiciones 3D (la ruta suave en mundo)
    //  indiceRuta     : índice actual en la ruta (se actualiza internamente)
    //  radioAceptacion: distancia para considerar que llegó al waypoint
    //  Devuelve       : dirección normalizada (o {0,0,0} si no hay ruta)
    // --------------------------------------------------------
    // 
    // Cambia la línea del radioAceptacion default de 0.4f a:
    glm::vec3 CalcularDireccionSteering(
        const glm::vec3& posActual,
        const vector<glm::vec3>& ruta,
        size_t& indiceRuta,
        float radioAceptacion = 1.0f)
    {
        if (ruta.empty() || indiceRuta >= ruta.size())
            return glm::vec3(0.0f);

        while (indiceRuta < ruta.size()) {
            glm::vec3 diff = ruta[indiceRuta] - posActual;
            diff.y = 0.0f;
            if (glm::length(diff) > radioAceptacion) break;
            ++indiceRuta;
        }

        if (indiceRuta >= ruta.size())
            return glm::vec3(0.0f);

        // Apuntar 2 nodos adelante para anticipar giros
        size_t idxTarget = std::min(indiceRuta + 2, ruta.size() - 1);

        glm::vec3 dir = ruta[idxTarget] - posActual;
        dir.y = 0.0f;
        float len = glm::length(dir);
        if (len < 0.001f) return glm::vec3(0.0f);
        return dir / len;
    }

private:

    // --------------------------------------------------------
    //  Heurística Octile
    //  Mejor que Manhattan para espacios donde el agente podría
    //  moverse en diagonal (aunque aquí usemos 4 dirs, Octile
    //  es más admisible que Manhattan y guía mejor a A*)
    // --------------------------------------------------------
    float HeuristicaOctile(glm::ivec2 a, glm::ivec2 b) const {
        float dx = (float)abs(a.x - b.x);
        float dy = (float)abs(a.y - b.y);
        // D=1 (coste cardinal), D2=sqrt(2) (diagonal)
        // Octile: D*(dx+dy) + (D2-2D)*min(dx,dy)
        // Con D=1: dx+dy - (2 - sqrt(2))*min(dx,dy)
        return dx + dy - 0.5858f * min(dx, dy);
    }

    // --------------------------------------------------------
    //  EsTransitable: celda dentro del mapa y no es pared (1)
    // --------------------------------------------------------
    bool EsTransitable(glm::ivec2 pos,
        const vector<vector<int>>& mapa,
        int filas, int columnas) const
    {
        if (pos.y < 0 || pos.y >= filas)   return false;
        if (pos.x < 0 || pos.x >= columnas) return false;
        return mapa[pos.y][pos.x] != 1;
    }

    // --------------------------------------------------------
    //  ReconstruirCamino: backtrack por el mapa de padres
    // --------------------------------------------------------
    vector<glm::ivec2> ReconstruirCamino(
        const unordered_map<glm::ivec2, glm::ivec2, IVec2Hash>& padre,
        glm::ivec2 objetivo,
        glm::ivec2 inicio) const
    {
        vector<glm::ivec2> camino;
        glm::ivec2 actual = objetivo;

        // Límite de seguridad para evitar bucle infinito
        int limite = 100000;
        while (actual != inicio && limite-- > 0) {
            camino.push_back(actual);
            auto it = padre.find(actual);
            if (it == padre.end()) break;
            actual = it->second;
        }
        camino.push_back(inicio);
        reverse(camino.begin(), camino.end());
        return camino;
    }

    // --------------------------------------------------------
    //  LineaDeVisionSegura
    //  Bresenham con comprobación de esquinas (thick line check).
    //  Verifica las celdas "vecinas de transición" para evitar
    //  que la línea se cuele por el hueco entre dos esquinas
    //  de pared diagonalmente adyacentes.
    // --------------------------------------------------------
    bool LineaDeVisionSegura(
        glm::ivec2 desde,
        glm::ivec2 hasta,
        const vector<vector<int>>& mapa) const
    {
        int filas = (int)mapa.size();
        int columnas = (int)mapa[0].size();

        int x0 = desde.x, y0 = desde.y;
        int x1 = hasta.x, y1 = hasta.y;

        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        int x = x0, y = y0;
        int err = dx - dy;

        while (true) {
            // Celda actual bloqueada
            if (y < 0 || y >= filas || x < 0 || x >= columnas) return false;
            if (mapa[y][x] == 1) return false;

            if (x == x1 && y == y1) break;

            int e2 = 2 * err;

            // Comprobación de esquina: si el paso es diagonal en Bresenham,
            // verificar que ninguna de las dos celdas "de esquina" sea pared.
            // Esto evita el bug clásico de "cortar por la esquina".
            if (e2 > -dy && e2 < dx) {
                // Paso diagonal inminente — revisar ambos vecinos cardinales
                int nx = x + sx;
                int ny = y + sy;

                // Vecino en X
                if (nx >= 0 && nx < columnas && y >= 0 && y < filas)
                    if (mapa[y][nx] == 1) return false;

                // Vecino en Y
                if (x >= 0 && x < columnas && ny >= 0 && ny < filas)
                    if (mapa[ny][x] == 1) return false;
            }

            if (e2 >= -dy) { err -= dy; x += sx; }
            if (e2 <= dx) { err += dx; y += sy; }
        }
        return true;
    }
};