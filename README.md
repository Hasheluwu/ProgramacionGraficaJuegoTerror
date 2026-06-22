# 🌑 Hunted: UNI

> **Proyecto de Programación Gráfica** — Juego de terror en primera persona desarrollado con OpenGL desde cero (sin motor)

---

## 📖 Sinopsis

Eres un estudiante de la UNI que, tras un incidente aún por determinar, despiertas en los niveles subterráneos de la universidad. Solo, en oscuridad absoluta y sin señal telefónica, tu única herramienta es la linterna de tu móvil.

Sin embargo, no estás solo. Un **Skinwalker** que ha tomado la forma de uno de tus amigos te acecha. Debes resolver los puzzles del entorno, encontrar las llaves y escapar hacia la superficie antes de ser capturado.

---

## 🎮 Jugabilidad

| Mecánica | Descripción |
|---|---|
| 🎯 **Perspectiva** | Primera persona (FPS) |
| 🏃 **Estamina** | Sistema de resistencia al correr — al agotarse el jugador se ralentiza y jadea |
| 🦆 **Sigilo** | Agáchate para acceder a ductos y zonas donde el monstruo no puede entrar |
| 🔊 **Dualidad de Sonido** | El jugador genera ruido al moverse; el monstruo también, permitiendo localizarlo sin verlo |
| 🪤 **Distracción** | Activa switches, alarmas y objetos del escenario para desviar la atención de la IA |
| 🚪 **Puertas y Llaves** | Algunas puertas están bloqueadas y requieren una llave específica para abrirse |
| 🗄️ **Escondites** | Entra en armarios/lockers para ocultarte del monstruo mientras éste patrulla |
| 🔦 **Linterna** | Herramienta principal de iluminación — encenderla puede alertar al monstruo |
| 🧩 **Puzzles** | El entorno contiene interruptores de luz y elementos interactivos que forman parte del escape |
| 🔑 **Objetivo** | Recolectar 2 llaves específicas y escapar del sótano |

---

## 🕹️ Controles

| Tecla | Acción |
|---|---|
| `W` `A` `S` `D` | Moverse |
| `Shift` | Correr (consume estamina) |
| `Space` | Saltar |
| `Tab` / `Ctrl` | Agacharse |
| `E` | Interactuar (puertas, llaves, switches, armarios) |
| `F` | Encender / apagar linterna |
| `Esc` | Pausa / salir |

---

## 🗄️ Sistema de Escondites

El jugador puede meterse en armarios (lockers) del escenario para evadir al monstruo. Al esconderse:

- El movimiento del jugador queda **bloqueado** — la cámara se fija en la posición interior del armario.
- El monstruo **no puede detectarte** dentro si no te ha visto entrar.
- Se activan eventos de **respiración** según el nivel de estamina (jadeo suave o fuerte) que pueden delatarte si el enemigo está cerca.
- Puedes salir del escondite presionando `E` de nuevo.

```cpp
// Flags de estado (Player.h)
bool isHiding;          // true cuando el jugador está dentro de un armario
glm::vec3 hidePosition; // posición de cámara fija durante el escondite
bool softBreathEvent;   // jadeo leve (estamina < 30%)
bool hardBreathEvent;   // jadeo fuerte (estamina agotada)
```

---

## 🚪 Sistema de Puertas y Llaves

Las puertas tienen animación de apertura/cierre y lógica de interacción por raycast:

- El jugador debe **mirar directamente** a la puerta e interactuar (`E`) estando a distancia suficiente.
- Algunas puertas requieren una **llave** específica — si no la tienes, la puerta reproduce un sonido de error y permanece cerrada.
- Hay puertas de **doble hoja** que comparten estado de apertura.
- El **monstruo puede forzar puertas** (`ForceOpen()`) si te encuentra detrás de una.
- Una puerta abierta con llave **deja de requerirla** para siempre.

---

## 💡 Sistema de Interruptores (Switches)

Los interruptores de luz son objetos interactivos en el escenario:

- Controlan grupos de lámparas por índice.
- Tienen **animación de palanca** con interpolación angular.
- El monstruo puede **manipularlos** para apagar zonas y crear zonas de oscuridad como trampa.
- También pueden usarse para distraer al enemigo activándolos ruidosamente.

---

## 🧠 Inteligencia Artificial del Monstruo

El Skinwalker usa un sistema de detección dual: **visión + sonido**.

### Detección Visual
- Si la **luminosidad del jugador supera 0.80** (linterna encendida o zona iluminada), el monstruo inicia la persecución.
- La IA rastrea el ángulo de visión para determinar si el jugador está dentro de su campo visual.

### Detección por Sonido
- El radio de detección varía según la acción del jugador:
  - Caminar normal → radio pequeño
  - Sprint → radio grande
  - Colisión con objetos → radio máximo
  - Escondido y quieto → indetectable
- El jugador agotado emite **sonidos de jadeo** que el monstruo puede escuchar.

### Comportamiento Avanzado
- **Trampas sonoras:** La IA puede activar interruptores y dejar luces encendidas estratégicamente para engañar al jugador.
- **Forzar puertas:** Si el monstruo sabe que el jugador está detrás de una puerta, puede abrirla por la fuerza.
- **Bloqueo del jugador:** Durante el ataque del monstruo, el input del jugador queda bloqueado (`isBlocked = true`).

---

## 🎛️ Menú Principal

El juego cuenta con un sistema de menú completo renderizado en OpenGL con shaders propios:

### Estados del Menú
| Estado | Descripción |
|---|---|
| `MAIN` | Menú principal con fondo animado y parallax |
| `SETTINGS` | Configuración de sensibilidad, brillo y volumen maestro |
| `CREDITS` | Créditos del equipo |
| `LOADING` | Pantalla de carga con barra de progreso y partículas |
| `PLAYING` | Juego en curso |
| `PAUSED` | Pausa en juego (accesible con `Esc`) |
| `CONFIRM_QUIT` | Diálogo de confirmación al salir |
| `CONFIRM_MAIN` | Confirmación para volver al menú |

### Características Técnicas del Menú
- **Renderizado de fuentes** con `stb_truetype` — atlas de 512×512 para títulos (52px) y texto normal (28px).
- **Efectos de partículas** en pantallas de carga y fondo.
- **Fade in/out animado** entre transiciones de estado.
- **Parallax en el fondo** sincronizado con el movimiento del ratón.
- Configuración persistente de sensibilidad del ratón (`0.08` por defecto), brillo (`0.5–2.0`) y volumen (`0.0–1.0`).

---

## 🔊 Sistema de Audio 3D

Implementado con **miniaudio** como motor de audio:

- Soporte para sonidos **2D** (música, UI) y **3D posicional** (efectos del entorno y del monstruo).
- Los sonidos 3D tienen `minDistance` y `maxDistance` configurables para simular atenuación realista.
- La posición del **listener** (jugador) se actualiza cada frame junto con su orientación.
- Sonidos registrados: pasos, jadeos, apertura de puertas, errores de puerta bloqueada, interruptores.

---

## 🖥️ Programación Gráfica

| Sistema | Detalle |
|---|---|
| **Shaders GLSL** | Vertex + Fragment shaders propios, compilados en runtime |
| **Cámara FPS** | Yaw/Pitch, sensibilidad configurable, FOV 45° |
| **Iluminación dinámica** | Linterna del jugador + lámparas de entorno con switches |
| **Carga de modelos** | Pipeline completo con **Assimp** (triangulación, normales, UVs) |
| **Escala de texturas UV** | Lectura del `.mtl` para escalar UVs por material (`map_Kd -s`) |
| **Raycast** | Detección de mirada para interactuar con puertas, switches y armarios |
| **Hitboxes** | Colisiones con el entorno y objetos interactivos |
| **Menú en OpenGL** | UI completamente renderizada en OpenGL con shaders propios + stb_truetype |

---

## 🛠️ Stack Tecnológico

| Tecnología | Uso |
|---|---|
| **C++** | Lenguaje principal |
| **OpenGL 3.3 Core** | API gráfica |
| **GLAD** | Cargador de funciones OpenGL |
| **GLFW** | Ventana, contexto e inputs |
| **GLM** | Matemáticas 3D (vectores, matrices, transformaciones) |
| **Assimp** | Carga de modelos 3D (.obj, .fbx, etc.) |
| **miniaudio** | Motor de audio 2D y 3D posicional |
| **SOIL2** | Carga de texturas (PNG, JPG) |
| **stb_truetype** | Renderizado de fuentes TrueType en OpenGL |

---

## 📁 Estructura del Proyecto

```
ProgramacionGraficaJuegoTerror/
├── Camera.h / .cpp         # Sistema de cámara FPS (Yaw, Pitch, FOV)
├── Player.h / .cpp         # Jugador: movimiento, físicas, estamina, linterna, escondites
├── Door.h / .cpp           # Puertas: animación, raycast, llave requerida, ForceOpen
├── Switch.h / .cpp         # Interruptores de luz: animación, control de lámparas
├── Menu.h / .cpp           # Sistema de menú completo con estados, fuentes y partículas
├── AudioManager.h / .cpp   # Audio 2D y 3D con miniaudio
├── Model.h / .cpp          # Carga de modelos 3D con Assimp
├── Mesh.h / .cpp           # Geometría: vértices, índices, texturas
├── Shader.h / .cpp         # Compilación y uso de shaders GLSL
├── Raycast.h               # Detección de punto en rayo para interacciones
├── TextureUtils.h          # Utilidades de carga de texturas
├── stb_truetype.h          # Renderizado de fuentes (vendor)
├── vendor/
│   └── miniaudio/          # Motor de audio (header-only)
├── shaders/                # Archivos .vert y .frag
└── assets/                 # Modelos, texturas y recursos del escenario
```

---

## ⚙️ Parámetros del Jugador (rama `escondites` / `Final_IA_falta_detalles`)

```cpp
PLAYER_SPEED         = 5.0f    // Velocidad normal
PLAYER_SPRINT_SPEED  = 10.0f   // Velocidad al correr
PLAYER_SLOW_SPEED    = 3.5f    // Velocidad al estar agotado
STAMINA_MAX          = 180.0f  // Estamina máxima
STAMINA_DRAIN_RATE   = 14.0f   // Drenaje por segundo al correr
STAMINA_REGEN_RATE   = 22.0f   // Regeneración por segundo
STAMINA_REGEN_DELAY  = 1.3f    // Espera antes de regenerar (segundos)
STAMINA_MIN_TO_SPRINT= 45.0f   // Mínimo de estamina requerida para poder correr
STAMINA_LOW_PERCENT  = 0.30f   // Umbral bajo (30%) — activa jadeo suave
JUMP_FORCE           = 6.0f    // Fuerza de salto
GRAVITY              = 18.0f   // Gravedad aplicada al jugador
HEIGHT_STAND         = 2.2f    // Altura de ojos de pie
HEIGHT_CROUCH        = 1.3f    // Altura de ojos agachado
```

---

## 🗺️ Entorno

- Escenario **indoors** inspirado de los  sótanos de la Universidad Nacional de Ingereria  (UNI)
- Props decorativos: sillas, mesas, lockers, ductos de ventilación
- Sistema de iluminación de entorno con lámparas controladas por switches
- Linterna del jugador como principal fuente de luz dinámica
- Zonas de escape restringidas por puertas con llave

---

## 🚧 Estado del Proyecto y Ramas

> El proyecto se encuentra en **desarrollo activo** con funcionalidades distribuidas en distintas ramas.

| Rama | Contenido |
|---|---|
| `puertas-y-llaves` | Sistema de puertas animadas, llaves y raycast de interacción |
| `escondites` | Sistema de esconderse en armarios, eventos de respiración |
| `Sotano_iluminacion_menu` | Escenario del sótano, iluminación de entorno y menú completo |
| `Final_IA_falta_detalles` | IA del monstruo, audio 3D, switches y sistema de bloqueo de input |
| `main` | Integración general (en progreso) |

---

## 👥 Equipo

Proyecto desarrollado como parte del curso de **Programación Gráfica** de la 
Universidad de Ingeneria Grupo 3t3 con los integrantes:

-Navas Jorge Emilio 
-Lopez Arguello Hashel Ignacio
-Garcia Telleria Luis Angel
-Guido Torrez Maximiliano


---

## 📄 Licencia

Proyecto académico — Todos los derechos reservados.
