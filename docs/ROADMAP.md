# Delanx — hoja de ruta

## Fase 0 — Base (completada)

- [x] Toolchain devkitA64 + libnx + SDL2 + curl + libarchive + jansson verificada y compilando NROs.
- [x] Investigación del proyecto original: Quiver Launcher **no tiene ninguna ruta ejecutable
      para Switch** (`nro`, `nsp`, `hbmenu`, `sphaira`, `homebrew` → 0 coincidencias en 590
      ficheros); Switch aparece solo como metadato de catálogo y como token con el que Quiver
      **excluye** assets de Switch. Delanx invierte ese matcher.
- [x] Catálogo propio verificado: 46 proyectos con `.nro` real, 5 ports de Harbour Masters y 3
      proyectos HM **sin** build de Switch documentados con su motivo.
- [x] Estructura en la SD compatible con hbmenu y con el escaneo de un solo nivel de Sphaira.

## Fase 1 — Primer NRO funcional (completada; instalación verificada con paquetes reales)

- [x] Modelo de datos intercambiable con Quiver (`apps.json`, `settings.json`, conservando
      claves desconocidas para no destruir ajustes del escritorio).
- [x] Cliente HTTP con TLS del sistema (libcurl de devkitPro), GitHub, GitLab y catálogo Quiver.
- [x] Verificación SHA-256 y semántica *verified / unverified / blocked*.
- [x] Instalador con libarchive (zip, 7z, tar.*), protección anti *path traversal*, respeto de
      los ficheros que aporta el usuario y `strip_root` para zips con carpeta raíz única.
- [x] **Instalación verificada en el PC con tres paquetes reales de Switch** (Starship, Sphaira y
      Spaghettify): descarga, SHA-256, extracción, metadatos de Quiver, localización del `.nro`,
      desinstalación y conservación de los ficheros del usuario (`tests/host/test_installer.cpp`).
- [x] Lanzador de NROs con `envSetNextLoad` (hbmenu, Sphaira y cadena de forwarders).
- [x] Interfaz SDL2 estilo Sphaira: sidebar, rejilla de carátulas, fichas de detalle, diálogos,
      barra de progreso de descarga, ajustes y teclado en pantalla.
- [x] Pruebas automáticas en el PC: 107 (datos) + 1024 (red) + 40 (instalador), y verificador del
      `.nro` (17/17).

## Fase 2 — Robustez y experiencia de uso

- [x] **Entrada por libnx HID** (pad + táctil): SDL2 en Switch no expone `SDL_GameController`,
      así que la interfaz lee `padUpdate`/`hidGetTouchScreenStates` directamente. Cursor propio
      con el stick derecho; primer toque enfoca, segundo activa.
- [x] **Carátulas** descargadas a `Cache/artwork` de una en una entre fotogramas.
- [x] **Autotest de dispositivo** (`tests/device`) para verificar en la consola lo que no se
      puede comprobar desde el PC.
- [x] **Pantalla de actualizaciones**: lista las apps con actualización pendiente, permite
      actualizar una a una o todas de golpe, y respeta la regla de Quiver (las actualizaciones
      automáticas solo instalan releases verificadas).
- [ ] **Re-probar en hardware** el mando y el táctil tras el cambio a libnx HID.
- [ ] Hilos de libnx para que las consultas de red no bloqueen el bucle de dibujo
      (hoy se usa un modal de espera síncrono).
- [ ] Vista de lista compacta además de la rejilla (el ajuste `UseGridView` ya existe).
- [ ] Buscador local dentro de la biblioteca y filtros por etiqueta, como en escritorio.
- [ ] Confirmación previa con espacio libre real estimado por asset (`Content-Length`).

## Fase 3 — Paridad con Quiver

- [ ] Mods: Thunderstore y GameBanana (`mods.path`, `mods.layout` y `mods.sources` ya se
      leen y escriben).
- [ ] Listas propias (`CustomAppListLocation`), anuncios y reseñas de `quiverlauncher.com`.
- [ ] Informes de problema de descarga (`POST /apps/{slug}/download-problem`) desde la consola.
- [ ] Temas: usar `PrimaryColor`/`SecondaryColor` de `settings.json` en lugar de la paleta fija.
- [ ] Kiosco/PIN (modo expositor) adaptado a consola.
- [ ] Escaneo de `sdmc:/switch` para importar homebrew ya instalado y crear la biblioteca sola.

## Fase 3b — Catálogo maestro del proyecto

- [x] Importador `tools/import_ports_md.py` que convierte `switch_ports_catalog.md` (110 títulos)
      al formato de Delanx, en un fichero **aparte** (`ports-master.json`) con `verified: false`.
- [x] Tercera fuente en la pestaña Catálogo (Delanx verificado → Maestro sin verificar → Quiver).
- [x] Carátulas de **Libretro Thumbnails** (`raw.githubusercontent.com`, sin clave ni límite) para
      las entradas verificadas que no traen icono propio.
- [ ] Verificar los títulos del catálogo maestro que sí publican `.nro` y promoverlos al catálogo
      verificado (empezando por los que ya sabemos que existen).
- [ ] Leer el `art-map.json` directamente desde el código (hoy se aplica al catálogo con un
      script) para poder actualizar carátulas sin recompilar.

## Fase 4 — Integración con la consola

- [ ] Creación de **forwarders NSP** desde Delanx (requiere *title override*, `ncm`/`es` y
      `prod.keys`; es la parte con más riesgo y por eso va al final).
- [ ] Instalación de `.nsp`/`.xci` con `ncm` (misma advertencia).
- [ ] Importar/exportar `apps.json` por FTP o por la SD para sincronizar con el escritorio.
- [ ] Overlay (Tesla) para lanzar Delanx desde un juego.

## Riesgos conocidos

1. **Assets que cambian de plataforma**: Harbour Masters dejó de publicar build de Switch en
   SoH 9.x, Ghostship 3.0.0 y SpaghettiKart 1.0.0. El catálogo local fija el filtro de asset y
   la versión; conviene revisarlo cuando el upstream cambie. Una auditoría contra los releases
   reales verificó 45 de 46 assets y el contenido de 17 paquetes.
2. **`looks_like_switch()` es heurístico** cuando el nombre del asset no lleva marcador: con
   filtro explícito de catálogo se confía en el filtro; sin filtro, se exige marcador positivo.
3. **Destinos compartidos**: 21 entradas instalan en `/switch` y 11 en la raíz de la SD. Están
   protegidas (manifiesto + nada de borrados recursivos), pero el desinstalador depende de que
   exista el manifiesto: una instalación anterior a esta versión se marca como "no se borra, es
   una carpeta compartida" y hay que limpiarla a mano.
4. **`mGBA`**: su asset es un `.7z` y el catálogo no puede declarar la ruta interna del `.nro`;
   queda como `verified: false` y sin instalación automática.
5. **Modo applet**: menos memoria y sin instalación de NSP. Delanx lo detecta y avisa.
6. **Pendiente en hardware**: confirmar el mando y el táctil tras el cambio a libnx HID, y
   ejecutar una instalación completa desde la consola.
