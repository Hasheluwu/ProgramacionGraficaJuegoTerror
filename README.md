# 🌑 Hunted: UNI

> **[ES] Proyecto de Programación Gráfica** — Juego de terror en primera persona desarrollado con OpenGL desde cero (sin motor)
>
> **[EN] Computer Graphics Project** — First-person horror game developed with OpenGL from scratch (no engine)

---

## 📖 Sinopsis / Synopsis

**[ES]** Eres un estudiante de la UNI que, tras un incidente aún por determinar, despiertas en los niveles subterráneos de la universidad. Solo, en oscuridad absoluta y sin señal telefónica, tu única herramienta es la linterna de tu móvil.

Sin embargo, no estás solo. Un **Skinwalker** que ha tomado la forma de uno de tus amigos te acecha. Debes resolver los puzzles del entorno, encontrar las llaves y escapar hacia la superficie antes de ser capturado.

---

**[EN]** You are a UNI student who, after an incident yet to be determined, wakes up in the underground levels of the university. Alone, in absolute darkness and with no phone signal, your only tool is your phone's flashlight.

However, you are not alone. A **Skinwalker** that has taken the form of one of your friends is stalking you. You must solve the environmental puzzles, find the keys, and escape to the surface before being caught.

---

## 🎮 Jugabilidad / Gameplay

| Mecánica / Mechanic | Descripción [ES] / Description [EN] |
|---|---|
| 🎯 **Perspectiva / Perspective** | Primera persona (FPS) / First person (FPS) |
| 🏃 **Estamina / Stamina** | Sistema de resistencia al correr — al agotarse el jugador se ralentiza y jadea / Resistance system while running — when depleted the player slows down and pants |
| 🦆 **Sigilo / Stealth** | Agáchate para acceder a ductos y zonas donde el monstruo no puede entrar / Crouch to access ducts and areas where the monster cannot enter |
| 🔊 **Dualidad de Sonido / Sound Duality** | El jugador genera ruido al moverse; el monstruo también, permitiendo localizarlo sin verlo / The player generates noise while moving; so does the monster, allowing you to locate it without seeing it |
| 🪤 **Distracción / Distraction** | Activa switches, alarmas y objetos del escenario para desviar la atención de la IA / Activate switches, alarms and scene objects to divert the AI's attention |
| 🚪 **Puertas y Llaves / Doors and Keys** | Algunas puertas están bloqueadas y requieren una llave específica para abrirse / Some doors are locked and require a specific key to open |
| 🗄️ **Escondites / Hiding Spots** | Entra en armarios/lockers para ocultarte del monstruo mientras éste patrulla / Enter cabinets/lockers to hide from the monster while it patrols |
| 🔦 **Linterna / Flashlight** | Herramienta principal de iluminación — encenderla puede alertar al monstruo / Main lighting tool — turning it on can alert the monster |
| 🧩 **Puzzles** | El entorno contiene interruptores de luz y elementos interactivos que forman parte del escape / The environment contains light switches and interactive elements that are part of the escape |
| 🔑 **Objetivo / Objective** | Recolectar 2 llaves específicas y escapar del sótano / Collect 2 specific keys and escape the basement |

---

## 🕹️ Controles / Controls

| Tecla / Key | Acción [ES] / Action [EN] |
|---|---|
| `W` `A` `S` `D` | Moverse / Move |
| `Shift` | Correr (consume estamina) / Sprint (consumes stamina) |
| `Space` | Saltar / Jump |
| `Tab` / `Ctrl` | Agacharse / Crouch |
| `E` | Interactuar (puertas, llaves, switches, armarios) / Interact (doors, keys, switches, lockers) |
| `F` | Encender / apagar linterna / Toggle flashlight |
| `Esc` | Pausa / salir / Pause / quit |

---

## 🗄️ Sistema de Escondites / Hiding System

**[ES]** El jugador puede meterse en armarios (lockers) del escenario para evadir al monstruo. Al esconderse:

- El movimiento del jugador queda **bloqueado** — la cámara se fija en la posición interior del armario.
- El monstruo **no puede detectarte** dentro si no te ha visto entrar.
- Se activan eventos de **respiración** según el nivel de estamina (jadeo suave o fuerte) que pueden delatarte si el enemigo está cerca.
- Puedes salir del escondite presionando `E` de nuevo.

**[EN]** The player can hide inside lockers in the scene to evade the monster. While hiding:

- The player's movement is **locked** — the camera is fixed at the locker's interior position.
- The monster **cannot detect you** inside if it didn't see you enter.
- **Breathing events** are triggered based on stamina level (soft or heavy panting) that can give you away if the enemy is nearby.
- You can exit the hiding spot by pressing `E` again.

```cpp
// Flags de estado / State flags (Player.h)
bool isHiding;          // true cuando el jugador está dentro de un armario / true when player is inside a locker
glm::vec3 hidePosition; // posición de cámara fija durante el escondite / fixed camera position during hiding
bool softBreathEvent;   // jadeo leve (estamina < 30%) / soft panting (stamina < 30%)
bool hardBreathEvent;   // jadeo fuerte (estamina agotada) / heavy panting (stamina depleted)
```

---

## 🚪 Sistema de Puertas y Llaves / Door and Key System

**[ES]** Las puertas tienen animación de apertura/cierre y lógica de interacción por raycast:

- El jugador debe **mirar directamente** a la puerta e interactuar (`E`) estando a distancia suficiente.
- Algunas puertas requieren una **llave** específica — si no la tienes, la puerta reproduce un sonido de error y permanece cerrada.
- Hay puertas de **doble hoja** que comparten estado de apertura.
- El **monstruo puede forzar puertas** (`ForceOpen()`) si te encuentra detrás de una.
- Una puerta abierta con llave **deja de requerirla** para siempre.

**[EN]** Doors have open/close animation and raycast interaction logic:

- The player must **look directly** at the door and interact (`E`) while close enough.
- Some doors require a specific **key** — if you don't have it, the door plays an error sound and stays closed.
- There are **double-leaf doors** that share an open/close state.
- The **monster can force doors open** (`ForceOpen()`) if it finds you behind one.
- A door opened with a key **no longer requires it** forever after.

---

## 💡 Sistema de Interruptores / Switch System

**[ES]** Los interruptores de luz son objetos interactivos en el escenario:

- Controlan grupos de lámparas por índice.
- Tienen **animación de palanca** con interpolación angular.
- El monstruo puede **manipularlos** para apagar zonas y crear zonas de oscuridad como trampa.
- También pueden usarse para distraer al enemigo activándolos ruidosamente.

**[EN]** Light switches are interactive objects in the scene:

- They control groups of lamps by index.
- They have **lever animation** with angular interpolation.
- The monster can **manipulate them** to turn off areas and create darkness zones as traps.
- They can also be used to distract the enemy by activating them noisily.

---

## 🧠 Inteligencia Artificial del Monstruo / Monster Artificial Intelligence

**[ES]** El Skinwalker usa un sistema de detección dual: **visión + sonido**.

**[EN]** The Skinwalker uses a dual detection system: **vision + sound**.

### Detección Visual / Visual Detection

**[ES]**
- Si la **luminosidad del jugador supera 0.80** (linterna encendida o zona iluminada), el monstruo inicia la persecución.
- La IA rastrea el ángulo de visión para determinar si el jugador está dentro de su campo visual.

**[EN]**
- If the **player's luminosity exceeds 0.80** (flashlight on or lit area), the monster begins pursuit.
- The AI tracks the viewing angle to determine if the player is within its field of vision.

### Detección por Sonido / Sound Detection

**[ES]** El radio de detección varía según la acción del jugador:
- Caminar normal → radio pequeño
- Sprint → radio grande
- Colisión con objetos → radio máximo
- Escondido y quieto → indetectable
- El jugador agotado emite **sonidos de jadeo** que el monstruo puede escuchar.

**[EN]** The detection radius varies depending on the player's action:
- Normal walking → small radius
- Sprinting → large radius
- Collision with objects → maximum radius
- Hidden and still → undetectable
- An exhausted player emits **panting sounds** that the monster can hear.

### Comportamiento Avanzado / Advanced Behavior

**[ES]**
- **Trampas sonoras:** La IA puede activar interruptores y dejar luces encendidas estratégicamente para engañar al jugador.
- **Forzar puertas:** Si el monstruo sabe que el jugador está detrás de una puerta, puede abrirla por la fuerza.
- **Bloqueo del jugador:** Durante el ataque del monstruo, el input del jugador queda bloqueado (`isBlocked = true`).

**[EN]**
- **Sound traps:** The AI can activate switches and strategically leave lights on to deceive the player.
- **Force doors:** If the monster knows the player is behind a door, it can force it open.
- **Player block:** During the monster's attack, the player's input is blocked (`isBlocked = true`).

---

## 🎛️ Menú Principal / Main Menu

**[ES]** El juego cuenta con un sistema de menú completo renderizado en OpenGL con shaders propios.

**[EN]** The game features a complete menu system rendered in OpenGL with custom shaders.

### Estados del Menú / Menu States

| Estado / State | Descripción [ES] / Description [EN] |
|---|---|
| `MAIN` | Menú principal con fondo animado y parallax / Main menu with animated parallax background |
| `SETTINGS` | Configuración de sensibilidad, brillo y volumen maestro / Sensitivity, brightness and master volume settings |
| `CREDITS` | Créditos del equipo / Team credits |
| `LOADING` | Pantalla de carga con barra de progreso y partículas / Loading screen with progress bar and particles |
| `PLAYING` | Juego en curso / Game in progress |
| `PAUSED` | Pausa en juego (accesible con `Esc`) / In-game pause (accessible with `Esc`) |
| `CONFIRM_QUIT` | Diálogo de confirmación al salir / Quit confirmation dialog |
| `CONFIRM_MAIN` | Confirmación para volver al menú / Confirmation to return to main menu |

### Características Técnicas del Menú / Technical Menu Features

**[ES]**
- **Renderizado de fuentes** con `stb_truetype` — atlas de 512×512 para títulos (52px) y texto normal (28px).
- **Efectos de partículas** en pantallas de carga y fondo.
- **Fade in/out animado** entre transiciones de estado.
- **Parallax en el fondo** sincronizado con el movimiento del ratón.
- Configuración persistente de sensibilidad del ratón (`0.08` por defecto), brillo (`0.5–2.0`) y volumen (`0.0–1.0`).

**[EN]**
- **Font rendering** with `stb_truetype` — 512×512 atlas for titles (52px) and normal text (28px).
- **Particle effects** on loading screens and background.
- **Animated fade in/out** between state transitions.
- **Background parallax** synchronized with mouse movement.
- Persistent configuration for mouse sensitivity (`0.08` default), brightness (`0.5–2.0`) and volume (`0.0–1.0`).

---

## 🔊 Sistema de Audio 3D / 3D Audio System

**[ES]** Implementado con **miniaudio** como motor de audio:

- Soporte para sonidos **2D** (música, UI) y **3D posicional** (efectos del entorno y del monstruo).
- Los sonidos 3D tienen `minDistance` y `maxDistance` configurables para simular atenuación realista.
- La posición del **listener** (jugador) se actualiza cada frame junto con su orientación.
- Sonidos registrados: pasos, jadeos, apertura de puertas, errores de puerta bloqueada, interruptores.

**[EN]** Implemented with **miniaudio** as the audio engine:

- Support for **2D** sounds (music, UI) and **3D positional** (environmental and monster effects).
- 3D sounds have configurable `minDistance` and `maxDistance` to simulate realistic attenuation.
- The **listener** position (player) is updated every frame along with its orientation.
- Registered sounds: footsteps, panting, door opening, locked door errors, switches.

---

## 🖥️ Programación Gráfica / Graphics Programming

| Sistema / System | Detalle [ES] / Detail [EN] |
|---|---|
| **Shaders GLSL** | Vertex + Fragment shaders propios, compilados en runtime / Custom Vertex + Fragment shaders, compiled at runtime |
| **Cámara FPS / FPS Camera** | Yaw/Pitch, sensibilidad configurable, FOV 45° / Yaw/Pitch, configurable sensitivity, 45° FOV |
| **Iluminación dinámica / Dynamic Lighting** | Linterna del jugador + lámparas de entorno con switches / Player flashlight + environment lamps with switches |
| **Carga de modelos / Model Loading** | Pipeline completo con **Assimp** (triangulación, normales, UVs) / Full pipeline with **Assimp** (triangulation, normals, UVs) |
| **Escala de texturas UV / UV Texture Scale** | Lectura del `.mtl` para escalar UVs por material (`map_Kd -s`) / Reading `.mtl` to scale UVs per material (`map_Kd -s`) |
| **Raycast** | Detección de mirada para interactuar con puertas, switches y armarios / Gaze detection to interact with doors, switches and lockers |
| **Hitboxes** | Colisiones con el entorno y objetos interactivos / Collisions with environment and interactive objects |
| **Menú en OpenGL / OpenGL Menu** | UI completamente renderizada en OpenGL con shaders propios + stb_truetype / UI fully rendered in OpenGL with custom shaders + stb_truetype |

---

## 🛠️ Stack Tecnológico / Technology Stack

| Tecnología / Technology | Uso [ES] / Use [EN] |
|---|---|
| **C++** | Lenguaje principal / Main language |
| **OpenGL 3.3 Core** | API gráfica / Graphics API |
| **GLAD** | Cargador de funciones OpenGL / OpenGL function loader |
| **GLFW** | Ventana, contexto e inputs / Window, context and inputs |
| **GLM** | Matemáticas 3D (vectores, matrices, transformaciones) / 3D math (vectors, matrices, transformations) |
| **Assimp** | Carga de modelos 3D (.obj, .fbx, etc.) / 3D model loading (.obj, .fbx, etc.) |
| **miniaudio** | Motor de audio 2D y 3D posicional / 2D and 3D positional audio engine |
| **SOIL2** | Carga de texturas (PNG, JPG) / Texture loading (PNG, JPG) |
| **stb_truetype** | Renderizado de fuentes TrueType en OpenGL / TrueType font rendering in OpenGL |

---

## 📁 Estructura del Proyecto / Project Structure

```
ProgramacionGraficaJuegoTerror/
├── Camera.h / .cpp         # Sistema de cámara FPS (Yaw, Pitch, FOV) / FPS camera system
├── Player.h / .cpp         # Jugador: movimiento, físicas, estamina, linterna, escondites / Player: movement, physics, stamina, flashlight, hiding
├── Door.h / .cpp           # Puertas: animación, raycast, llave requerida, ForceOpen / Doors: animation, raycast, required key, ForceOpen
├── Switch.h / .cpp         # Interruptores de luz: animación, control de lámparas / Light switches: animation, lamp control
├── Menu.h / .cpp           # Sistema de menú completo con estados, fuentes y partículas / Full menu system with states, fonts and particles
├── AudioManager.h / .cpp   # Audio 2D y 3D con miniaudio / 2D and 3D audio with miniaudio
├── Model.h / .cpp          # Carga de modelos 3D con Assimp / 3D model loading with Assimp
├── Mesh.h / .cpp           # Geometría: vértices, índices, texturas / Geometry: vertices, indices, textures
├── Shader.h / .cpp         # Compilación y uso de shaders GLSL / GLSL shader compilation and usage
├── Raycast.h               # Detección de punto en rayo para interacciones / Ray point detection for interactions
├── TextureUtils.h          # Utilidades de carga de texturas / Texture loading utilities
├── stb_truetype.h          # Renderizado de fuentes (vendor) / Font rendering (vendor)
├── vendor/
│   └── miniaudio/          # Motor de audio (header-only) / Audio engine (header-only)
├── shaders/                # Archivos .vert y .frag / .vert and .frag files
└── assets/                 # Modelos, texturas y recursos del escenario / Models, textures and scene resources
```

---

## ⚙️ Parámetros del Jugador / Player Parameters
*(rama `escondites` / `Final_IA_falta_detalles` branch)*

```cpp
PLAYER_SPEED         = 5.0f    // Velocidad normal / Normal speed
PLAYER_SPRINT_SPEED  = 10.0f   // Velocidad al correr / Sprint speed
PLAYER_SLOW_SPEED    = 3.5f    // Velocidad al estar agotado / Exhausted speed
STAMINA_MAX          = 180.0f  // Estamina máxima / Maximum stamina
STAMINA_DRAIN_RATE   = 14.0f   // Drenaje por segundo al correr / Drain per second while sprinting
STAMINA_REGEN_RATE   = 22.0f   // Regeneración por segundo / Regeneration per second
STAMINA_REGEN_DELAY  = 1.3f    // Espera antes de regenerar (segundos) / Wait before regenerating (seconds)
STAMINA_MIN_TO_SPRINT= 45.0f   // Mínimo de estamina requerida para poder correr / Minimum stamina required to sprint
STAMINA_LOW_PERCENT  = 0.30f   // Umbral bajo (30%) — activa jadeo suave / Low threshold (30%) — activates soft panting
JUMP_FORCE           = 6.0f    // Fuerza de salto / Jump force
GRAVITY              = 18.0f   // Gravedad aplicada al jugador / Gravity applied to player
HEIGHT_STAND         = 2.2f    // Altura de ojos de pie / Standing eye height
HEIGHT_CROUCH        = 1.3f    // Altura de ojos agachado / Crouching eye height
```

---

## 🗺️ Entorno / Environment

**[ES]**
- Escenario **indoors** inspirado en los sótanos de la Universidad Nacional de Ingeniería (UNI)
- Props decorativos: sillas, mesas, lockers, ductos de ventilación
- Sistema de iluminación de entorno con lámparas controladas por switches
- Linterna del jugador como principal fuente de luz dinámica
- Zonas de escape restringidas por puertas con llave

**[EN]**
- **Indoors** setting inspired by the basements of the National University of Engineering (UNI)
- Decorative props: chairs, tables, lockers, ventilation ducts
- Environment lighting system with switch-controlled lamps
- Player flashlight as the main dynamic light source
- Escape zones restricted by locked doors

---

## 🚧 Estado del Proyecto y Ramas / Project Status and Branches

> **[ES]** El proyecto se encuentra en **desarrollo activo** con funcionalidades distribuidas en distintas ramas.
>
> **[EN]** The project is in **active development** with features distributed across different branches.

| Rama / Branch | Contenido [ES] / Content [EN] |
|---|---|
| `puertas-y-llaves` | Sistema de puertas animadas, llaves y raycast de interacción / Animated door system, keys and interaction raycast |
| `escondites` | Sistema de esconderse en armarios, eventos de respiración / Locker hiding system, breathing events |
| `Sotano_iluminacion_menu` | Escenario del sótano, iluminación de entorno y menú completo / Basement scene, environment lighting and full menu |
| `Final_IA_falta_detalles` | IA del monstruo, audio 3D, switches y sistema de bloqueo de input / Monster AI, 3D audio, switches and input blocking system |
| `main` | Integración general (en progreso) / General integration (in progress) |

---

## 👥 Equipo / Team

**[ES]** Proyecto desarrollado como parte del curso de **Programación Gráfica** de la Universidad de Ingeniería — Grupo 3t3.

**[EN]** Project developed as part of the **Computer Graphics** course at the National University of Engineering — Group 3t3.

- Navas Jorge Emilio
- Lopez Arguello Hashel Ignacio
- Garcia Telleria Luis Angel
- Guido Torrez Maximiliano

---

## 📄 Licencia / License

**[ES]** Proyecto académico — Todos los derechos reservados.

**[EN]** Academic project — All rights reserved.
