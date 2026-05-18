# 🌑 Hunted: UNI
**Proyecto de Programación Gráfica**

## 📖 Sinopsis
Eres un estudiante de la UNI que, tras un incidente aún por determinar, despiertas en los niveles subterráneos de la universidad. Solo, en oscuridad absoluta y sin señal telefónica, tu única herramienta es la linterna de tu móvil. 

Sin embargo, no estás solo. Un **Skinwalker** que ha tomado la forma de uno de tus amigos te acecha. Debes encontrar las llaves y escapar hacia la superficie antes de ser capturado.

## 🎮 Jugabilidad (Mechanics)
*   **Perspectiva:** Primera persona (FPS).
*   **Gestión de Estamina:** Sistema de resistencia al correr que limita la huida.
*   **Sigilo e Interacción:** Posibilidad de agacharse para acceder a ductos o áreas reducidas donde el monstruo no puede entrar.
*   **Dualidad de Sonido:** El jugador genera ruido al moverse; el monstruo también genera sonido, permitiendo localizarlo, pero él también te escucha.
*   **Sistema de Distracción:** Capacidad de lanzar objetos o activar elementos del escenario (alarmas, tazas) para desviar la atención de la IA.

## 🧠 Inteligencia Artificial y Gráficos
*   **IA de Acecho:** El enemigo utiliza lógica de escucha y visión. Si la luminosidad del jugador supera el umbral de **0.80**, el monstruo iniciará la persecución.
*   **Estrategia del Enemigo:** La IA es capaz de colocar trampas sonoras y dejar luces encendidas para engañar al jugador.
*   **Programación Gráfica:** 
    *   Uso de *Hitboxes* precisas.
    *   Mecánicas de iluminación dinámica (linterna y luces de entorno).
    *   Cálculo de rangos de sonido según la acción (pasos vs. colisiones).

## 🛠️ Aspectos Técnicos
*   **Entorno:** Escenario *indoors* con props decorativos (sillas, mesas, lockers).
*   **Objetivo:** Recolección de 2 llaves específicas y escape.
*   **IA:** Modelo reciclado de personaje (Skinwalker) para optimización de recursos.