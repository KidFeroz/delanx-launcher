# Historial de Delanx

## 0.1.0 — primera versión funcional

**Base**

- NRO compilable con devkitA64/libnx + SDL2 (devkitPro), con icono de NACP, catálogo verificado y
  fuente Inter dentro de `romfs:/`.
- Interfaz tipo Sphaira: barra lateral de pestañas, rejilla de carátulas, fichas de detalle,
  diálogos modales, barra de progreso de descarga, ajustes y teclado del sistema.
- Biblioteca y ajustes en los **mismos formatos que Quiver Launcher** (`apps.json`,
  `settings.json`), conservando las claves desconocidas para no perder ajustes del escritorio.
- Catálogo propio de **46 proyectos con `.nro` real** y **sección exclusiva Harbour Masters**
  (5 ports con build de Switch y 3 documentados como sin build, con su alternativa *nightly*).
- Catálogo online de Quiver (`api.quiverlauncher.com`) para descubrir proyectos, advirtiendo de
  que su selector de assets descarta los ficheros de Switch.
- Descarga/instalación desde releases de **GitHub** y **GitLab** con verificación **SHA-256** y
  semántica *verified / unverified / blocked*.
- Lanzamiento encadenado de NROs con `envSetNextLoad` (hbmenu, Sphaira y forwarders NSP).
- Autotest de dispositivo (`tests/device`) para verificar en la consola lo que no se puede
  comprobar desde el PC.

**Correcciones tras la primera prueba en hardware**

- **La entrada no respondía** (ni mandos ni táctil). Causa: el puerto SDL2 de Switch no entrega
  `SDL_GameController`, así que `SDL_GameControllerOpen()` no devolvía nada. Ahora el pad y la
  pantalla táctil se leen directamente del HID de libnx (`padInitializeDefault`/`padUpdate`,
  `hidGetTouchScreenStates`), con cursor propio: el primer toque enfoca una tarjeta y el segundo
  la activa. También hay navegación con el stick derecho.
- Se evitó un fallo al salir: se marcaba "mando conectado" con un `SDL_GameController*` falso que
  `shutdown()` habría pasado a `SDL_GameControllerClose()`.
- El tema y las columnas guardados en `settings.json` no se aplicaban hasta el segundo arranque.
- Zonas pulsables: se rehacen cada fotograma (antes una pestaña sin rejilla podía reutilizar las
  tarjetas del fotograma anterior) y los ajustes también responden al tacto.

**Carátulas**

- Descarga y caché en `Cache/artwork/<folderName>.png`, de una en una entre fotogramas para no
  bloquear la entrada. Antes las tarjetas solo mostraban iniciales.

**Bugs reales encontrados por las pruebas automáticas**

- **GitLab devolvía cero assets** en todas las releases: el parser buscaba `links`/`sources` en la
  raíz del release y la API v4 los anida en `assets`. Cualquier instalación desde GitLab habría
  fallado en silencio. Verificado contra 20 releases reales (36 enlaces + 4 fuentes cada una).
- **La extracción sobrescribía ficheros del usuario**: los paquetes de Harbour Masters incluyen su
  propio `.o2r` de ejemplo, que se escribía encima del que genera el jugador. Ahora la entrada del
  archivo se omite y se conserva el fichero del usuario (y se informa en el mensaje).
- `to_sd_path()` no era idempotente: un valor ya normalizado podía recibir el prefijo dos veces.
- `upcoming_release` de GitLab se leía como cadena cuando es un booleano.

**Bugs críticos encontrados por las auditorías y las pruebas**

- **Pérdida de datos en destinos compartidos**: 21 entradas del catálogo instalan en `/switch` y
  11 en la raíz de la SD (`/`). Reinstalarlas vaciaba el destino completo y desinstalarlas
  borraba el árbol entero — se habría llevado por delante Atmosphere, payloads, `hbmenu`, los
  overlays y las partidas. Ahora:
  - los destinos compartidos **nunca** se vacían ni se borran recursivamente;
  - Delanx escribe un **manifiesto** (`Cache/installed/<folder>.manifest`) con los ficheros que
    instaló, y al desinstalar elimina exactamente esos y ninguno más;
  - los ficheros que declara el catálogo (`requiredUserFiles`, incluso si son carpetas) quedan
    protegidos frente al purgado, la extracción y la desinstalación.
- **El táctil moría tras el primer toque**: `touch_poll()` devolvía `false` cuando no había dedo
  encima, el `else` que liberaba el botón era inalcanzable y `pressed(Touch)` quedaba en falso
  para siempre. Ahora el sondeo informa siempre y `active` lleva el estado.
- **La extracción sobrescribía ficheros del usuario**: los paquetes de Harbour Masters incluyen su
  propio `.o2r` de ejemplo y se escribía encima del que genera el jugador. La protección ahora es
  por extensión (incluidas `.pak`/`.lmp` y ficheros **sin** extensión) y, sobre todo, por lo que
  declara el catálogo, también dentro de subcarpetas.
- **GitLab devolvía cero assets** en todas las releases: el parser buscaba `links`/`sources` en la
  raíz del release y la API v4 los anida en `assets`. Verificado contra 20 releases reales.
- `to_sd_path()` no era idempotente (prefijo duplicado) ni toleraba `SDMC:` en mayúsculas.
- El nombre del ejecutable de un asset `.nro` suelto ahora lo manda el catálogo (DevilutionX
  exige `devilutionx.nro` aunque el asset se llame `devilutionx-switch.nro`).
- `upcoming_release` de GitLab se leía como cadena cuando es un booleano.
- Los caminos con barra inicial (`/C:/...`) creaban árboles basura dentro del destino.
- Al cambiar de pestaña o dockear la consola podían usarse zonas pulsables y coordenadas
  táctiles obsoletas: ahora se invalidan y se sigue `SDL_WINDOWEVENT_SIZE_CHANGED`.

**Catálogo (auditoría contra los releases reales)**

- Verificados 45 de 46 assets (HTTP 200) y el contenido de 17 paquetes: ningún `nroPath` ni
  `stripRoot` incorrecto.
- Corregido el nombre del ejecutable de **DevilutionX** y la URL de prueba de **JKSV**.
- Confirmado con evidencia (`expanded_assets`) que 2Ship2Harkinian, Lighthouse y PaperBoat no
  publican build de Switch, y que Shipwright 9.x, Ghostship 3.0.0 y SpaghettiKart 1.0.0 dejaron
  de publicarlo.

**Actualizaciones**

- La pestaña **Actualizaciones** ya no es solo un historial: lista las apps con actualización
  pendiente (versión instalada → disponible) y permite **actualizar la seleccionada (A)**,
  **comprobar releases (X)** y **actualizar todo (Y)**.
- La decisión de actualizar vive ahora en el modelo (`AppEntry::update_available_for()`), con
  reglas explícitas: mismo tag no es actualización, se respeta la versión que el jugador se
  saltó, una app manual nunca se actualiza y una app no instalada es una instalación.
- «Actualizar todo» omite las releases que Quiver no ha verificado, igual que Quiver Launcher:
  las actualizaciones automáticas nunca instalan releases sin verificar.
- Si GitHub agota el límite de peticiones (60/hora sin token), Delanx lo dice y ofrece abrir
  Ajustes para configurar un token, en lugar de mostrar un error técnico.

**Verificación**

| Prueba | Resultado |
|---|---|
| Compilación del NRO | 0 errores, 0 warnings en `source/` |
| `tools/verify_nro.py Delanx.nro` | 17/17 |
| `tests/host` · datos | 115/115 |
| `tests/host` · red (fixtures reales de GitHub, GitLab y Quiver) | 1030/1030 |
| `tests/host` · instalador (paquetes reales de Switch) | 66/66 |
| `tools/verify_nro.py tests/device/DelanxSelfTest.nro` | 14/14 |

Todo ello reproducible con una sola orden: `bash tools/verify_all.sh`.

## Catálogo Maestro e Integración Híbrida de Miniaturas

- **Capa de Arte Híbrida y Fallback RomFS**:
  - Implementado flujo en `App::icon_for()`: comprueba caché en MicroSD (`sdmc:/switch/Delanx/Cache/artwork/<folder>.png`), encola descarga asíncrona en segundo plano si hay URL directa de Libretro, y recurre de inmediato a `romfs:/icons/<folder>.png` o `romfs:/icons/default.png` si no hay conexión o falla la descarga.
  - Carátulas en la vista de Catálogo y Harbour Masters: `draw_list_screen()` ahora resuelve las carátulas tanto para elementos instalados como para entradas de catálogo.
- **Sincronización del Catálogo Verificado**:
  - `switch-catalog.json` enriquecido con carátulas CDN No-Intro (`appIconUrl`) mediante `tools/apply_art_map.py`.
- **Integración del Catálogo Maestro (`ports-master.json`)**:
  - `ports-master.json` sincronizado (102 entradas nuevas, 8 verificadas coincidentes) y accesible desde la pestaña de catálogo (`CatalogMode::Master`).
  - Emparejamiento por repositorio y carpeta final corregido en `tools/import_ports_md.py`.
