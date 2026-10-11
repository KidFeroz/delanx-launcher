# Compilar Delanx

## 1. Toolchain

Delanx se compila con **devkitPro / devkitA64** para Nintendo Switch (libnx + SDL2).

### Windows (lo que se usó para construir el `.nro` de este repo)

1. Descarga la base de MSYS2 (`msys2-base-x86_64-latest.tar.xz`) desde
   <https://github.com/msys2/msys2-installer/releases/latest> y descomprímela, por ejemplo,
   en `D:\devkitPro\msys2`. El instalador gráfico de devkitPro también sirve, pero la base
   de MSYS2 evita depender de un instalador con interfaz.
2. Añade los repositorios de devkitPro al final de `etc/pacman.conf` de MSYS2:

   ```ini
   [dkp-libs]
   Server = https://pkg.devkitpro.org/packages
   SigLevel = Optional TrustAll

   [dkp-windows]
   Server = https://pkg.devkitpro.org/packages/windows/$arch/
   SigLevel = Optional TrustAll
   ```

3. Instala la toolchain y las librerías (desde el shell de MSYS2):

   ```bash
   pacman -Sy
   pacman -S --needed switch-dev switch-examples \
       switch-sdl2 switch-sdl2_image switch-sdl2_ttf \
       switch-curl switch-mbedtls switch-jansson switch-libarchive \
       switch-zlib switch-bzip2 switch-liblzma switch-libzstd \
       switch-libpng switch-libjpeg-turbo switch-freetype switch-harfbuzz \
       switch-pkg-config switch-cmake make git pkg-config zip
   ```

   Notas verificadas durante la puesta en marcha:
   - el paquete de libnx se llama **`libnx`** (viene en el grupo `switch-dev`), no `switch-libnx`;
   - **`switch-minizip` no existe**: `switch-zlib` ya compila `libminizip`;
   - `switch-curl` está construido con zlib, así que al enlazar hace falta `-lz`
     (el `pkg-config` de devkitPro ya lo añade).

4. Variables de entorno (las define `switch_rules`, pero conviene exportarlas):

   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITA64=$DEVKITPRO/devkitA64
   export PATH=$DEVKITPRO/tools/bin:$DEVKITA64/bin:$PATH
   ```

### Linux/macOS

Sigue <https://devkitpro.org/wiki/devkitPro_pacman> (paquete `.pkg` en macOS o el repositorio
apt en Debian/Ubuntu) e instala los mismos paquetes con `dkp-pacman`.

## 2. Compilar

```bash
export DEVKITPRO=/opt/devkitpro
make -j$(nproc)
```

Salida: **`Delanx.nro`** (con el icono del NACP, el catálogo verificado y la fuente dentro de
`romfs:/`). En este repo el build verificado es de 11.285.626 bytes y compila **sin warnings**
en `source/` con `-Wall -Wextra`.

## 3. Verificar el resultado

```bash
python tools/verify_nro.py Delanx.nro
```

Comprueba la cabecera NRO, que los segmentos estén dentro del fichero, la sección `ASET`
(icono/NACP/RomFS), el título y la versión del NACP, que el icono sea un JPEG de 256x256 y que
el RomFS contenga el catálogo y la fuente. Salida esperada: `comprobaciones: 17  fallos: 0`.

## 4. Pruebas en el PC

Hay **tres suites**: datos, red e instalador. La de red sustituye libcurl por un stub que sirve
*fixtures* capturadas de las APIs reales, y la del instalador descarga paquetes reales de Switch
y ejercita el camino completo (elegir asset → descargar → verificar SHA-256 → extraer con
libarchive → escribir metadatos → localizar el `.nro` → desinstalar).

```bash
# Requiere: pacman -S --needed gcc mingw-w64-ucrt-x86_64-jansson mingw-w64-ucrt-x86_64-libarchive
bash tests/host/run.sh
```

Salidas esperadas: `Comprobaciones: 115  Fallos: 0` (datos), `1030  Fallos: 0` (red) y
`66  Fallos: 0` (instalador). Contenido:

| Suite | Fichero | Qué comprueba |
|---|---|---|
| Datos | `tests/host/test_data_layer.cpp` | ida y vuelta de `apps.json`/`settings.json` sin pérdida, catálogo propio, clasificación y selección de assets, SHA-256, rutas y **detección de actualizaciones** (tag igual, tag distinto, versión saltada, app manual, app no instalada) |
| Red | `tests/host/test_network_layer.cpp` | releases reales de GitHub (`HarbourMasters/Starship`), el caso real de **límite de peticiones agotado** (403), releases de GitLab (`gitlab-org/cli`, 36 enlaces + 4 fuentes por release), catálogo y verificación de Quiver, coherencia del catálogo propio |
| Instalador | `tests/host/test_installer.cpp` | instalación real de tres paquetes de Switch (`Starship`, `sphaira.zip`, `Spaghettify`), verificación SHA-256 (incluido el fallo por hash incorrecto), `strip_root`, rechazo de *path traversal*, metadatos de Quiver, **destino compartido** (raíz de la SD y `/switch`) sin tocar nada ajeno, ficheros declarados en `requiredUserFiles`, conservación de los ficheros del usuario en reinstalación y desinstalación, y **cola/caché de carátulas** |

Fixtures:
- `tests/fixtures/*.json` — respuestas reales capturadas con `curl` de `api.github.com`,
  `gitlab.com/api/v4` y `api.quiverlauncher.com`. Para ampliarlas hace falta un token de GitHub
  (el límite sin token es de 60 peticiones/hora y se agota rápido).
- `tests/fixtures/assets/*.zip` — paquetes de release reales (~17 MB en total, **no** versionados):
  se descargan con `python tests/host/fetch_fixtures.py`, que verifica su SHA-256.
- `tests/host/make_test_archives.py` genera `malicious.zip` (con `../`) y `single-root.zip`.

En Windows, el compilador de MSYS2 `msys/gcc` está roto en esta máquina (target cygwin sin
cabeceras completas); usa el de UCRT64, que es un compilador nativo:

```powershell
$ucrt = 'D:\devkitPro\msys2\msys64\ucrt64\bin'
$env:PATH = "$ucrt;$env:PATH"
# suite de datos
& "$ucrt\g++.exe" -std=c++20 -O1 -include tests\host\host_compat.h `
  -I source -I D:\devkitPro\msys2\msys64\ucrt64\include `
  -o build\host\delanx_data_tests.exe `
  tests\host\test_data_layer.cpp source\core\strings.cpp source\core\json.cpp `
  source\core\paths.cpp source\core\time.cpp source\core\sha256.cpp `
  source\model\app_entry.cpp source\model\catalog.cpp source\model\settings.cpp `
  source\services\asset_selector.cpp -L D:\devkitPro\msys2\msys64\ucrt64\lib -ljansson
.\build\host\delanx_data_tests.exe

# suite de red (el stub HTTP vive dentro del propio test)
& "$ucrt\g++.exe" -std=c++20 -O1 -include tests\host\host_compat.h `
  -I source -I D:\devkitPro\msys2\msys64\ucrt64\include `
  -o build\host\delanx_net_tests.exe `
  tests\host\test_network_layer.cpp source\core\strings.cpp source\core\json.cpp `
  source\core\paths.cpp source\core\time.cpp source\core\sha256.cpp `
  source\model\app_entry.cpp source\model\catalog.cpp source\model\settings.cpp `
  source\services\github.cpp source\services\gitlab.cpp source\services\quiver_catalog.cpp `
  source\services\release_check.cpp source\services\asset_selector.cpp `
  -L D:\devkitPro\msys2\msys64\ucrt64\lib -ljansson
.\build\host\delanx_net_tests.exe
```

## 4b. Autotest en la consola

Lo que solo se puede verificar en hardware (vectores SHA-256 del toolchain real, libarchive,
lanzamiento de NROs y capa de plataforma) se comprueba con un NRO aparte:

```bash
make -C tests/device run      # compila DelanxSelfTest.nro y lo envia con nxlink -s
```

Escribe el informe en `sdmc:/switch/Delanx/instcheck-report.txt`. Detalles en
`tests/device/README.md`.

## 5. Paquete para la SD

```bash
make package        # genera Delanx-0.1.0-sdcard.zip con switch/Delanx/Delanx.nro
```

También puedes enviar el NRO por red con `nxlink` (`nxlink -s Delanx.nro`) si tienes un
servidor de homebrew escuchando.

## 6. Regenerar el icono

```bash
python tools/make_icon.py     # escribe icon.jpg (256x256 JPEG) usando romfs/fonts/Inter.ttf
```

## 7. Estructura del código

```
source/
├── main.cpp            arranque: romfs, argv, bucle
├── core/               json (jansson), rutas SD, cadenas, tiempo, sha256
├── model/              AppEntry/Library (apps.json), Settings, catalogo y releases
├── services/           http (curl), github, gitlab, catalogo Quiver, verificacion,
│                       instalador (libarchive), lanzador (envSetNextLoad)
├── ui/                 tema, render SDL2, entrada de mando, pantallas
└── platform/           libnx: modo applet, bateria, firmware, log, swkbd
```

Añadir un módulo nuevo: crea el `.cpp` en el subdirectorio correspondiente y añade ese
subdirectorio a `SOURCES` del `Makefile` (el `make` de devkitPro no busca de forma recursiva).
