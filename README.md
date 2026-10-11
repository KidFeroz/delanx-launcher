# Delanx — Homebrew Port & Decompilation Manager para Nintendo Switch

[![Release](https://img.shields.io/badge/release-v0.1.0--experimental-blue.svg)](https://github.com/KidFeroz/Delanx/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Language](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch%20(Horizon%20OS)-E60012?logo=nintendoswitch)](https://github.com/switchbrew/libnx)
[![Toolchain](https://img.shields.io/badge/Toolchain-devkitA64%20%2F%20libnx-E65C00)](https://devkitpro.org)
[![Vibecoded with AI](https://img.shields.io/badge/developed%20with-AI%20%2F%20Vibecoding-8A2BE2.svg)](#-declaración-de-desarrollo-asistido-por-ia-vibecoding)

**Delanx** es un gestor y cargador (*launcher*) nativo de alto rendimiento implementado en **C++20** para Nintendo Switch (Horizon OS). Su propósito es centralizar la catalogación, instalación, verificación criptográfica y ejecución de adaptaciones de código (*source ports*), decompilaciones (*decomps*) y recompilaciones estáticas (*recomps*) dentro del ecosistema homebrew.

El proyecto implementa interoperabilidad a nivel de datos con el ecosistema de **Quiver Launcher**, reimplementando la lógica cliente en binario nativo AArch64 ante la ausencia de un runtime .NET sobre Horizon OS.

> [!NOTE]
> **Hecho por un entusiasta en casa con asistencia de IA (*Vibecoding*)**  
> Este proyecto no nace de un laboratorio ni de una gran empresa: soy un apasionado del homebrew desarrollando y probando directamente en mi Nintendo Switch en casa, usando Inteligencia Artificial como copiloto para acelerar la programación en C++.
> 
> * 🎮 **Probado en consola real:** El código se compila y se testea directamente en mi propia consola y tarjeta SD en el día a día.
> * 🤝 **100% abierto a aportes humanos:** Si programas en C++, conoces `libnx`/`SDL2` o quieres ayudar a pulir la arquitectura, ¡eres más que bienvenido! Las Pull Requests y revisiones humanas son súper apreciadas.
> * ✌️ **Buena vibra y respeto:** Entiendo perfectamente que a muchas personas no les agrada el *vibecoding* con IA y respeto totalmente esa postura. Solo pido esa misma tolerancia y respeto en los Issues y debates: las críticas técnicas y constructivas son bienvenidas, aquí estamos para sumar a la comunidad homebrew y pasarla bien. 🕹️

---

## 📌 Tabla de Contenidos

- [Aviso de Desarrollo con IA](#-aviso-de-desarrollo-asistido-por-ia-vibecoding-y-auditoría-comunitaria)
- [Motivación Técnica y Compatibilidad](#-motivación-técnica-y-compatibilidad)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Características Principales](#-características-principales)
- [Estructura en Tarjeta MicroSD](#-estructura-en-tarjeta-microsd)
- [Compilación y Toolchain](#-compilación-y-toolchain)
- [Batería de Pruebas y Auditoría](#-batería-de-pruebas-y-auditoría)
- [Transparencia y Filosofía Vibecoding](#-transparencia-y-filosofía-vibecoding)
- [Guía para Contribuidores](#-guía-para-contribuidores)
- [Licencia y Reconocimientos](#-licencia-y-reconocimientos)

---

## ⚙️ Motivación Técnica y Compatibilidad

El software de escritorio *Quiver Launcher* opera sobre .NET 10 y Avalonia UI, tecnologías carentes de entorno de ejecución en Horizon OS. Delanx resuelve esto mediante:

1. **Reimplementación Nativa:** Código C++20 compilado directamente contra `libnx`, `SDL2` y bibliotecas portadas por `devkitPro`.
2. **Inversión de Filtros de Plataforma:** Mientras que el catálogo central de Quiver excluye binarios etiquetados como `switch` para entornos de escritorio, Delanx consume dichos metadatos como marcador positivo de compatibilidad.
3. **Persistencia No Destructiva (`apps.json` / `settings.json`):** Delanx preserva los esquemas de metadatos de Quiver, serializando y deserializando claves PascalCase sin alterar configuraciones de usuario existentes y añadiendo metadatos específicos bajo el objeto `"switch"`.

```json
{
  "name": "Ship of Harkinian",
  "folderName": "shipofharkinian",
  "installPath": "/switch/shipofharkinian",
  "switch": {
    "targetDir": "/switch/shipofharkinian",
    "nro": "shipofharkinian.nro",
    "requiredUserFiles": [
      { "path": "sdmc:/switch/shipofharkinian/oot.otr", "present": true }
    ]
  }
}
```

---

## 🏛️ Arquitectura del Sistema

El diseño sigue una separación estricta de responsabilidades por capas sin dependencias cíclicas:

```
source/
├── core/       -> Primitivas de bajo nivel: JSON (jansson), paths (sdmc:/), hashing SHA-256, logs.
├── model/      -> Entidades de dominio desacopladas: AppEntry, CatalogEntry, Release, Settings.
├── services/   -> Lógica de infraestructura: HTTP (libcurl), libarchive (descompresión), artwork.
├── platform/   -> Abstracción de Horizon OS: libnx HID, applet/application mode, gestión de batería y red.
└── ui/         -> Renderizado gráfico SDL2: vistas (Grid/List), componentes, layout y bucle de eventos.
```

### Reglas de Dependencia
`UI` $\longrightarrow$ `Services` $\longrightarrow$ `Model` $\longrightarrow$ `Core`  
*(La capa `Platform` es consumida exclusivamente por el punto de entrada `main.cpp` y subsistemas de servicios concretos).*

### Flujo de Ejecución e Intercambio de Contexto
Delanx implementa el protocolo de encadenamiento binario de `libnx` (`envSetNextLoad`):
1. El usuario selecciona un título verificado en la interfaz.
2. Delanx comprueba los activos requeridos por el juego (ROMs/OTR/assets de usuario).
3. Se invoca `envSetNextLoad(target_nro_path, target_argv)`.
4. Delanx libera sus recursos (contexto SDL2, descriptores de archivos, buffers de red) y finaliza con código de salida limpio.
5. El cargador del entorno (`hbmenu`, `Sphaira` o forwarder NSP) ejecuta de forma atómica el NRO objetivo.

---

## 🚀 Características Principales

* **Catálogo Maestro de 45+ Proyectos:** Base de datos validada con ports de Nintendo 64 (*Ship of Harkinian, 2Ship, Ghostship, Banjo, Paper Mario*), PlayStation (*Driver 2*), PC (*Jak & Daxter OpenGOAL, The Simpsons: Hit & Run*) y Arcade.
* **Integridad Criptográfica:** Validación obligatoria por hash **SHA-256** de paquetes descargados frente a manifiestos de catálogo (*verified / unverified / blocked*).
* **Mitigación de Vulnerabilidades de Descompresión:** Extractor de archivos basado en `libarchive` con validación estricta contra ataques de desbordamiento de rutas (*path traversal / zip-slip*).
* **Arte y Carátulas a Costo Cero ($0):** Arquitectura híbrida que consume CDNs públicos sin API Keys:
  - *Libretro Thumbnails* para sistemas retro con nomenclatura No-Intro.
  - *SteamGridDB* para adaptaciones modernas de PC e indies.
* **Soporte de Entorno Dual:** Detección de ejecución en **Applet Mode** (memoria restringida de 400 MB) vs **Application Mode / Title Override** (acceso a ~3.2 GB de RAM), adaptando las cuotas de caché y resolución de texturas dinámicamente.
* **Entrada Robusta:** Lectura directa mediante `libnx HID` (sticks analógicos, botones y pantalla táctil capacitiva).

---

## 📂 Estructura en Tarjeta MicroSD

Delanx estandariza la jerarquía de almacenamiento respetando las convenciones comunitarias de Nintendo Switch:

```text
sdmc:/
├── roms/                               # Repositorio de assets originales del usuario
│   ├── n64/                            # ROMs .z64 (OoT, MM, SM64, etc.)
│   ├── ps1/                            # Imágenes de disco (.bin / .cue)
│   └── arcade/                         # ROMsets de emulación
│
└── switch/                             # Directorio estándar de aplicaciones Homebrew
    ├── Delanx/
    │   ├── Delanx.nro                  # Ejecutable principal
    │   ├── apps.json                   # Base de datos local de títulos instalados
    │   ├── settings.json               # Configuración persistente del usuario
    │   ├── Cache/                      # Miniaturas descargadas y metadatos cacheados
    │   └── delanx.log                  # Registro de diagnóstico en ejecución
    │
    ├── shipofharkinian/                # Ejemplo: Puerto individual aislado
    │   ├── shipofharkinian.nro
    │   └── oot.otr
    └── [resto_de_ports]/
```

---

## 🛠️ Compilación y Toolchain

### Prerrequisitos
Se requiere el entorno oficial de desarrollo cruzado **devkitPro** para arquitectura ARM64 (`devkitA64`):

* `devkitA64`
* `libnx`
* Portlibs de Switch:
  ```bash
  pacman -S --needed switch-dev switch-examples \
      switch-sdl2 switch-sdl2_image switch-sdl2_ttf \
      switch-libcurl switch-jansson switch-libarchive switch-freetype
  ```

### Variables de Entorno
Asegúrate de definir la variable base:
```bash
export DEVKITPRO=/opt/devkitpro
```

### Proceso de Construcción
```bash
# Limpiar compilaciones previas
make clean

# Compilar binarios (.elf, .nacp y .nro empaquetado con romfs)
make -j$(nproc)
```

El proceso generará en la raíz del repositorio:
* `Delanx.nro`: Binario ejecutable final para Horizon OS.
* `Delanx.nacp`: Archivo de metadatos del título (ID, autor, versión).
* `Delanx.elf`: Binario ELF intermedio con símbolos de depuración.

---

## 🧪 Batería de Pruebas y Auditoría

La integridad del código se audita mediante dos capas independientes de verificación:

### 1. Pruebas Nativas en Host (`tests/host`)
Permiten probar el motor en Linux/macOS/Windows sin necesidad de hardware físico o emulador:
* **Capa de Datos (`test_data_layer.cpp`):** Serialización y tolerancia a fallos de esquemas JSON.
* **Capa de Red (`test_network_layer.cpp`):** Mocking de endpoints HTTP, descargas por bloques y gestión de timeouts.
* **Instalador Seguro (`test_installer.cpp`):** Pruebas de descompresión real con paquetes de prueba y verificación de preservación de ficheros de usuario.

```bash
cd tests/host
./run.sh
```

### 2. Autotest en Hardware Real (`tests/device`)
Validación de hardware sobre Horizon OS:
* Comprobación del subsistema de montaje `romfs:/` y `sdmc:/`.
* Verificación de asignación de memoria heap en modo Title Override.
* Lectura del estado de la batería, conexión de red y reloj RTC.

---

## 🤖 De Entusiasta a Entusiasta: Vibecoding y Comunidad

Este proyecto está hecho por pura pasión por el homebrew y la preservación de videojuegos. Para poder construir un launcher completo en C++ sin pasar meses picando código repetitivo desde cero, me apoyé en agentes de IA (**Vibecoding**) como copilotos de programación.

### Cómo se construye y prueba Delanx:
1. **Desarrollo Asistido:** La IA ayuda a escribir la estructura, parsear JSON, enlazar librerías (`libcurl`, `libarchive`) y generar casos de prueba.
2. **Pruebas en Casa:** Todo se compila con `devkitA64` y lo pruebo directamente en mi propia Nintendo Switch y tarjeta SD, asegurando que funcione bien en hardware real.
3. **100% Abierto a Aportes Humanos:** El proyecto nació vibecodeado, pero no es exclusivo de máquinas. ¡Al contrario! Si eres dev y quieres auditar, optimizar memoria o enviar Pull Requests, tu ayuda es más que bienvenida.
4. **Espacio de Buena Onda y Tolerancia:** Se entiende y se respeta que a muchos en la comunidad no les guste el desarrollo con IA. Cualquier sugerencia técnica, reporte de error o mejora es bienvenida; solo pedimos mantener el respeto mutuo en Issues y Discussions. Al final, lo que nos une a todos es disfrutar de los ports en nuestra consola. 🎮✨

---

## 🤝 Guía para Contribuidores

Agradecemos las contribuciones de la comunidad. Para mantener la robustez y calidad de la base de código, solicitamos seguir estas directrices:

1. **Estándar C++20:**
   - Evitar RTTI (`-fno-rtti`) y excepciones (`-fno-exceptions`) para alinearse con los lineamientos de rendimiento de consolas empotradas.
   - Usar semántica de movimiento (`std::move`) y punteros inteligentes (`std::unique_ptr`) para control determinista de memoria.
2. **Formato de Código:**
   - Respetar el archivo de configuración `.clangd` incluido en la raíz del proyecto.
3. **Nuevos Ports en el Catálogo:**
   - Al sugerir un nuevo port en `switch_ports_catalog.md` o en `romfs/catalog/switch-catalog.json`, incluye el enlace al release de GitHub, el nombre exacto del archivo `.nro` interno, la ruta esperada de datos y el hash SHA-256 del asset.
4. **Flujo de Pull Requests:**
   - Realizar commits descriptivos (`feat:`, `fix:`, `refactor:`, `docs:`).
   - Asegurarse de que `make` compile sin advertencias (`-Wall -Wextra`).

---

## 📄 Licencia y Reconocimientos

Este software se distribuye bajo la licencia **MIT**. Consulta el archivo [LICENSE](LICENSE) para más detalles.

* **devkitPro & Switchbrew:** Por la toolchain y la biblioteca `libnx`.
* **Comunidad Quiver:** Por el diseño conceptual del catálogo y la especificación de esquemas.
* **Proyectos Decomp & Porting:** A todos los equipos de ingeniería inversa que hacen posible la preservación del software histórico en plataformas modernas.
