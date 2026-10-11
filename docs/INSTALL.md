# Instalar y usar Delanx en la Switch

## 1. Qué necesitas

- Una Switch con **Atmosphère** (CFW) y acceso al **Homebrew Menu** (hbmenu).
- Una tarjeta SD con espacio libre para los ports (varios GB si vas a instalar ports de N64).
- Conexión a internet en la consola para descargar releases y consultar el catálogo.

## 2. Copiar el launcher

Descomprime `Delanx-<version>-sdcard.zip` en la raíz de la SD. La estructura final es:

```
sdmc:/
└── switch/
    └── Delanx/
        └── Delanx.nro
```

Delanx crea el resto solo en el primer arranque:

```
sdmc:/switch/Delanx/
├── Delanx.nro
├── apps.json        # biblioteca (mismo formato que Quiver Launcher de escritorio)
├── settings.json    # ajustes (mismo formato que Quiver, claves PascalCase)
├── Apps/            # apps que no exigen una ruta concreta
├── Cache/           # estado de releases y carátulas
├── Temp/            # descargas en curso
└── delanx.log       # registro de diagnóstico
```

## 3. Formas de arrancarlo

### hbmenu

Coloca `hbmenu.nro` en la raíz de la SD (o entra desde el álbum) y abre **Delanx** desde el
menú homebrew. Es la forma recomendada.

### Sphaira

Sphaira escanea `sdmc:/switch/<carpeta>/<carpeta>.nro` (solo un nivel de anidamiento), y la ruta
de Delanx cumple ese patrón, así que aparece directamente en su lista. Delanx detecta que lo ha
lanzado Sphaira y lo muestra en el pie del menú lateral.

### Forwarder NSP

Puedes crear un acceso directo en el menú de la consola con cualquier herramienta de *forwarders*
apuntando a `sdmc:/switch/Delanx/Delanx.nro`. Los forwarders basados en `hbloader` exponen los
búferes de `envSetNextLoad`, así que Delanx **también puede lanzar otros homebrew** desde ahí:
cuando lanzas una app, Delanx escribe la ruta en esos búferes y termina, y el cargador arranca el
siguiente NRO (el mismo mecanismo que usan hbmenu y Sphaira).

### Álbum (applet mode)

Si tu firmware no permite el *title override*, entrarás en modo applet con memoria reducida.
Delanx lo detecta y lo avisa en la barra superior; el catálogo y las instalaciones funcionan,
pero conviene evitar carátulas grandes y no abrir muchas pantallas a la vez.

## 4. Uso básico

| Botón | Acción |
|---|---|
| **A** | Abrir detalles / lanzar |
| **X** | Instalar o actualizar la app enfocada |
| **Y** | Cambiar filtro (biblioteca) o fuente del catálogo (catálogo) |
| **B** | Atrás / cerrar diálogo / cancelar una instalación |
| **+** | Menú rápido del elemento enfocado |
| **−** | Buscar en el catálogo (teclado en pantalla) |
| **L / R** | Cambiar de pestaña |
| **Stick derecho** | Mover el cursor en pantalla |
| **Pantalla táctil** | Primer toque enfoca una tarjeta, segundo toque la abre |

La entrada se lee del **HID de libnx** (`padUpdate` + `hidGetTouchScreenStates`), no de SDL: el
puerto SDL2 de Switch no entrega `SDL_GameController`, así que la navegación con los mandos y el
táctil funcionan a través de libnx.

Pestañas: **Biblioteca**, **Catálogo**, **Harbour Masters**, **Actualizaciones** y **Ajustes**.

### Añadir apps

- **Catálogo Delanx (sin conexión)**: 46 proyectos verificados que publican `.nro` para Switch,
  con la ruta de instalación y los ficheros que debes aportar. Es el catálogo recomendado para
  instalar.
- **Catálogo Quiver (online)**: el mismo catálogo de escritorio (`api.quiverlauncher.com`).
  Sirve para **descubrir** proyectos, pero Quiver está pensado para PC: su selector de assets
  **descarta deliberadamente** los ficheros con marcador `switch`, así que muchas entradas no
  publican binarios de Switch. Delanx lo advierte antes de añadirlas.
- Las apps sin repositorio se pueden añadir como **manuales**: Delanx crea la carpeta y tú dejas
  los ficheros dentro.

### Actualizaciones y verificación

Delanx aplica la misma semántica que Quiver Launcher:

- elige la release **fijada por ti → verificada → la más reciente** (nunca una bloqueada sin avisar);
- comprueba el **SHA-256** del fichero descargado contra el checksum del catálogo o contra el
  `digest` que publica GitHub;
- guarda `version.txt` e `installed-build.txt` (`fichero\nsha256`) dentro de la carpeta de la app,
  igual que en escritorio.

## 5. Harbour Masters

La pestaña **Harbour Masters** existe aparte porque estos ports tienen reglas propias:

| Port | Build de Switch | Dónde |
|---|---|---|
| Ship of Harkinian (OoT) | Solo hasta 8.0.6 (`SoH-MacReady-Golf-Switch.zip`, con `soh.nro`) | `HarbourMasters/Shipwright` |
| SoH Switch Rev1 | 8.0.6-1, repo oficial solo-Switch | `HarbourMasters/Shipwright-Switch` |
| Starship (Star Fox 64) | v2.0.0 | `HarbourMasters/Starship` |
| Ghostship (Super Mario 64) | 2.0.0 | `HarbourMasters/Ghostship` |
| SpaghettiKart (Mario Kart 64) | Solo el prerelease 0.9.9.1 | `HarbourMasters/SpaghettiKart` |
| 2 Ship 2 Harkinian (Majora's Mask) | **sin build de Switch** | nightly de CI disponible en el README del proyecto |
| Lighthouse (Banjo-Kazooie) | **sin build de Switch** | — |
| PaperBoat (Paper Mario) | **sin build de Switch** | — |

En todos los casos **necesitas tu propia ROM** y, en la mayoría, **generar los ficheros
`.otr`/`.o2r` en un PC** antes de copiarlos a la SD. Delanx te muestra qué ficheros faltan en la
ficha de cada app y te avisa antes de lanzarla si no están.

Rutas típicas: `sdmc:/switch/soh/` para SoH y `sdmc:/switch/<nombre>/` para el resto, tal como
indica cada release.

## 6. Problemas conocidos

- **Los ports de Harbour Masters cambian de assets entre versiones**: si el release más reciente
  ya no trae build de Switch, Delanx lo dirá al buscar candidato. El catálogo local indica el
  filtro de asset correcto para cada uno.
- **Sin red no se pueden descargar releases nuevas**: el catálogo local y la biblioteca siguen
  funcionando, y las apps instaladas se lanzan igual.
- **Revisiones de firmware**: el `.nro` está verificado de forma estática (estructura NRO, NACP,
  icono, RomFS) y **probado en hardware**: la interfaz arranca y dibuja. Si algo se comporta de
  forma rara, el autotest de `tests/device` deja un informe en
  `sdmc:/switch/Delanx/instcheck-report.txt`.
