# Informe técnico: construir un NRO launcher/instalador para Nintendo Switch (QuiverNX)

**Alcance:** informe verificado con fuentes primarias (código fuente real y PKGBUILDs de devkitPro) para construir desde cero un NRO en C/C++ con devkitPro/libnx que descargue releases (GitHub/GitLab), descomprima, instale y lance otros homebrew, funcionando desde **hbmenu**, **Sphaira** y un **Forwarder NSP**.

**Fecha de verificación:** todas las URLs de este informe se consultaron y devolvieron HTTP 200 en el momento de redacción.

---

## 0. CORRECCIONES DE PREMISAS (leer primero)

Varias suposiciones de la pregunta son incorrectas. Las corrijo con evidencia, porque afectan directamente al Makefile y a los `pacman -S`:

| Premisa habitual | Realidad verificada | Evidencia |
|---|---|---|
| `switch-libnx` existe | **No existe ese nombre.** El paquete es **`libnx`** (pertenece al grupo `switch-dev`). `switch-dev` es un metapaquete/meta-grupo que lo arrastra. | `libnx/PKGBUILD` → `pkgname=('libnx')`, `groups=('switch-dev')` — https://raw.githubusercontent.com/devkitPro/pacman-packages/master/libnx/PKGBUILD |
| `switch-deko3d` existe | **No.** El paquete es **`deko3d`** (sin prefijo). | `deko3d/PKGBUILD` → `pkgname=('deko3d')`, `depends=('libnx')` — https://raw.githubusercontent.com/devkitPro/pacman-packages/master/deko3d/PKGBUILD |
| Existe `switch-minizip` | **No existe como paquete.** **`switch-zlib` ya compila e instala `libminizip`** desde `contrib/minizip`, con `-DUSE_FILE32API`. | `switch/zlib/PKGBUILD` — https://raw.githubusercontent.com/devkitPro/pacman-packages/master/switch/zlib/PKGBUILD |
| `devkitPro/switch-examples` | **Ese repo no existe.** El real es **`switchbrew/switch-examples`**, y el template está en **`templates/application/`** (no `application/template/`). | https://github.com/switchbrew/switch-examples/tree/master/templates/application |
| `devkitPro/installer` releases sirven para instalar hoy | Sus releases son de **2018** (v3.0.3, `devkitProUpdater-3.0.3.exe`). El camino actual es **devkitPro pacman** (instalador MSYS2 / `.pkg` en macOS) o MSYS2 propio. | https://github.com/devkitPro/installer/releases |
| Hay que configurar un CA bundle (`curl-ca-bundle`) | **No existe paquete de CA bundle en devkitPro y no hace falta.** `switch-curl` se compila `--with-libnx --with-default-ssl-backend=libnx`, es decir TLS va por el **servicio `ssl` de libnx**, que usa el **CertStore del sistema** (los CA de Nintendo). | `switch/curl/PKGBUILD` + `switch/curl/switch-curl.patch` (añade `lib/vtls/libnx.c`) |
| `setDefaultCertStore` es API de libnx | **No existe en libnx.** Grep exhaustivo sobre `nx/include/**` y `nx/source/**` de switchbrew/libnx: 0 resultados. | https://github.com/switchbrew/libnx |
| `AppletJumpToApplication` es API de libnx | **No existe.** No hay ningún símbolo `JumpToApplication` en `applet.h`. Lo que existe es `envSetNextLoad` + el buffer `NextLoadPath`. | https://raw.githubusercontent.com/switchbrew/libnx/master/nx/include/switch/services/applet.h |
| Sphaira usa SDL2 | **No.** Sphaira usa **deko3d** con **libpulsar** + **nanovg-deko3d** (vectorial) + **yyjson** + **minIni** + **libnxtc**, y su propio loader **hbl**. No enlaza SDL2. | `sphaira/sphaira/CMakeLists.txt` |
| `nlohmann/json` / `rapidjson` son lo que usa la gente | Sphaira usa **yyjson** (vía FetchContent). Paquetes pacman reales: **`switch-jansson`**, **`switch-libjson-c`**. No hay `switch-nlohmann-json` ni `switch-rapidjson` en pacman-packages. | `sphaira/CMakeLists.txt`; listing de `switch/` |

---

## 1. TOOLCHAIN EN WINDOWS

### 1.1 Instalación (camino recomendado)

**Opción A — Instalador gráfico devkitPro (recomendado para empezar).** La wiki de switchbrew indica descargar el instalador gráfico
(https://switchbrew.org/wiki/Setting_up_Development_Environment):

> "On Windows, there's a graphical installer. … Ensure at least 'Switch Development' is ticked."

Tras instalar, se lanza **MSYS** desde `Start → devkitPro → MSYS`. Ese MSYS trae `pacman` y el entorno ya configurado.

**Opción B — MSYS2 existente + repos devkitPro.** Si ya usas MSYS2, se añaden los repos de devkitPro siguiendo las instrucciones de
https://github.com/devkitPro/pacman/releases/latest (release actual publicada: **v6.0.2**, asset `devkitpro-pacman-installer.pkg`, que es para macOS/Linux; en Windows el paquete es el instalador de MSYS2).

**Rutas típicas de instalación (Windows):**

| Elemento | Ruta típica |
|---|---|
| `DEVKITPRO` (raíz) | `C:\devkitPro` |
| Toolchain AArch64 | `C:\devkitPro\devkitA64\bin` |
| libnx | `C:\devkitPro\libnx` |
| Portlibs Switch (SDL2, curl, PNG…) | `C:\devkitPro\portlibs\switch` |
| Herramientas (elf2nro, nacptool, npdmtool, build_pfs0) | `C:\devkitPro\tools\bin` |
| Ejemplos instalados | `C:\devkitPro\examples\switch` |
| Shell MSYS2 | `C:\devkitPro\msys2\usr\bin\bash.exe` (según instalador) |

> Verificado indirectamente: `switch-cmake/PKGBUILD` instala en `/opt/devkitpro/…`, que bajo Windows el instalador mapea a `C:\devkitPro\...`; y `switch-examples/PKGBUILD` hace `cp -R ... $pkgdir/opt/devkitpro/examples/switch`. La wiki confirma que los ejemplos quedan en `$DEVKITPRO/examples/switch`.

### 1.2 Paquetes pacman: nombres REALES verificados

**Núcleo (grupo `switch-dev`):**
```
libnx            deko3d           switch-tools       switch-cmake
dkp-toolchain-vars                devkitA64          switch-pkg-config
```
- `libnx` (v4.12.0), `deko3d` (v0.5.0) → verificados en `libnx/PKGBUILD`, `deko3d/PKGBUILD` (raíz del repo `pacman-packages`, **no** bajo `switch/`).
- `switch-cmake` → `cmake/switch/PKGBUILD`, `depends=('devkita64-cmake' 'switch-tools' 'uam' 'switch-pkg-config')`.
- `switch-tools` es dependencia de `switch-cmake` (mismo PKGBUILD), y provee `elf2nro`, `nacptool`, `npdmtool`, `build_pfs0`.

**Portlibs (todo bajo `switch/` del repo `pacman-packages`) — nombres exactos verificados en su PKGBUILD:**

| Lo que pediste | Nombre REAL del paquete | Nota |
|---|---|---|
| `switch-sdl2` | **`switch-sdl2`** | `depends=('switch-mesa' 'libnx')` |
| `switch-curl` | **`switch-curl`** | `depends=('switch-zlib' 'libnx')`, TLS vía libnx |
| `switch-libjpeg-turbo` | **`switch-libjpeg-turbo`** | `_libname=libjpeg-turbo` |
| `switch-libpng` | **`switch-libpng`** | `depends=('switch-zlib')` |
| `switch-zlib` | **`switch-zlib`** | **incluye `libminizip`** |
| `switch-bzip2` | **`switch-bzip2`** | |
| `switch-libexpat` | **`switch-libexpat`** | |
| `switch-cmake` | **`switch-cmake`** | |
| `switch-freetype` | **`switch-freetype`** | `depends=(switch-bzip2 switch-libpng switch-zlib)` |
| `switch-harfbuzz` | **`switch-harfbuzz`** | `depends=('switch-freetype')` |
| `switch-minizip` | **NO EXISTE** | ya viene en `switch-zlib` |
| `switch-libnx` | **NO EXISTE** | es `libnx` |

**Extras útiles verificados:** `switch-jansson`, `switch-libjson-c`, `switch-libarchive`, `switch-mbedtls` (v2.28.10), `switch-libwebp`, `switch-libzstd`, `switch-lz4`, `switch-liblzma`, `switch-glm`, `switch-physfs`, `switch-libconfig`, `switch-ntfs-3g`, `switch-lwext4`, `switch-pkg-config`, `switch-sdl2_image`, `switch-sdl2_ttf`.

> Nombres de imagen/ttf de SDL2 son **minúsculas**: `switch-sdl2_image`, `switch-sdl2_ttf` (`pkgname=switch-sdl2_image`), y ambos en grupo `switch-sdl2-libs`/`switch-sdl2`.

**Comando de instalación consolidado para QuiverNX:**
```bash
# dentro de MSYS de devkitPro
(dkp-)pacman -Syu
(dkp-)pacman -S switch-dev switch-cmake \
  switch-sdl2 switch-curl switch-zlib switch-libpng switch-libjpeg-turbo \
  switch-bzip2 switch-libexpat switch-freetype switch-harfbuzz \
  switch-jansson switch-libarchive deko3d
```
Referencia cruzada: el README oficial de Sphaira instala exactamente
`sudo pacman -S switch-dev deko3d switch-cmake switch-curl switch-glm switch-zlib switch-mbedtls switch-ntfs-3g`
(https://raw.githubusercontent.com/NaGaa95/sphaira/master/README.md) — nótese que **`deko3d` va sin prefijo `switch-`**.

### 1.3 Variables de entorno

`switch_rules` de libnx exige `DEVKITPRO` y calcula el resto:
```make
PORTLIBS := $(PORTLIBS_PATH)/switch
LIBNX    ?= $(DEVKITPRO)/libnx
```
(https://raw.githubusercontent.com/switchbrew/libnx/master/nx/switch_rules)

**PowerShell (sesión actual):**
```powershell
$env:DEVKITPRO = "C:\devkitPro"
$env:DEVKITA64 = "C:\devkitPro\devkitA64"
$env:PATH = "C:\devkitPro\devkitA64\bin;C:\devkitPro\tools\bin;C:\devkitPro\portlibs\switch\bin;$env:PATH"
```
**CMD:**
```cmd
set DEVKITPRO=C:\devkitPro
set DEVKITA64=C:\devkitPro\devkitA64
set PATH=C:\devkitPro\devkitA64\bin;C:\devkitPro\tools\bin;C:\devkitPro\portlibs\switch\bin;%PATH%
```
**Persistente (una vez):**
```powershell
[Environment]::SetEnvironmentVariable("DEVKITPRO","C:\devkitPro","User")
```
`DEVKITARM` **no** aplica a Switch (es para 3DS/GBA). Para Switch es **`DEVKITA64`**. `PORTLIBS_PATH` y `PORTLIBS` los define `base_rules`/`switch_rules`; no hay que ponerlos a mano.

### 1.4 Verificar la instalación

```bash
# 1) Toolchain
aarch64-none-elf-gcc --version
aarch64-none-elf-g++ --version
# 2) Herramientas de packaging
which elf2nro nacptool npdmtool build_pfs0 elf2nso
# 3) libnx + rules
ls $DEVKITPRO/libnx/switch_rules $DEVKITPRO/libnx/switch.specs $DEVKITPRO/libnx/lib/libnx.a
# 4) Portlibs
ls $DEVKITPRO/portlibs/switch/lib/libcurl.a
ls $DEVKITPRO/portlibs/switch/lib/libminizip.a    # viene de switch-zlib
pkg-config --list-all | grep -Ei 'curl|zlib|freetype|harfbuzz|libpng|jpeg'
# 5) Build real del template oficial
cp -r $DEVKITPRO/examples/switch/templates/application ~/test_nro && cd ~/test_nro && make
ls -la *.nro *.nacp
```
La wiki confirma este flujo: *"To compile it, type `make` in the project directory"* (https://switchbrew.org/wiki/Setting_up_Development_Environment).

### 1.5 Alternativas: WSL/Linux y Docker

**WSL/Linux:** la wiki dice explícitamente que *"Linux x86_64 binaries are usable under WSL"*. Pasos: instalar pacman de devkitPro (`https://github.com/devkitPro/pacman/releases/latest`), luego
```bash
sudo (dkp-)pacman -S switch-dev
# logout/login para recoger el entorno
```
Nota WSL: para `nxlink` (transferencia USB/red al Switch) y acceso a dispositivos USB hace falta usbipd/WSL2 con passthrough; `nxlink` por red (WiFi) funciona sin más.

**Docker (verificado):** imágenes reales en Docker Hub **`devkitpro/devkita64`** (401.756 pulls), `devkitarm`, `devkitppc`, `toolchain-base`. Tags reales de `devkita64`: `latest`, `20260219`, `20260215`, `20260202`, `20251231`, `20251117`, … (tag = fecha). La wiki: *"devkitPro also provides a set of Docker images which come pre-installed with all the portlibs packages."*

```bash
docker run --rm -it -v "$PWD":/work -w /work devkitpro/devkita64:latest bash
# dentro:
make
```
Usa `:latest` o un tag de fecha reproducible (p. ej. `devkita64:20260219`).

---

## 2. MAKEFILE MÍNIMO (y completo) PARA UN NRO

### 2.1 Cómo funciona realmente el sistema de reglas

`include $(DEVKITPRO)/libnx/switch_rules` arrastra `$(DEVKITPRO)/devkitA64/base_rules` y define las reglas reales. **Verificado en `switch_rules`:**

```make
%.nacp: $(MAKEFILE_LIST)
	@nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ $(NACPFLAGS)
%.nro: %.elf
	@elf2nro $< $@ $(NROFLAGS)
%.nso: %.elf
	@elf2nso $< $@
%.nsp: %.nso %.npdm   # (o %.nso si no hay APP_JSON)
	$(make_pfs0)
```
Defaults si no se definen: `APP_TITLE := $(notdir $(OUTPUT))`, `APP_AUTHOR := Unspecified Author`, `APP_VERSION := 1.0.0`, `APP_ICON := $(LIBNX)/default_icon.jpg`.

**El icono:** `switch_rules` NO genera ningún JPEG. El template busca en este orden (ver §2.3):
1. `<TARGET>.jpg`
2. `icon.jpg`
3. `$(LIBNX)/default_icon.jpg`

y lo pasa como `--icon=$(APP_ICON)` a `elf2nro`. Es decir: **tú debes aportar un `icon.jpg` JPEG válido** (128x128 recomendado; hbmenu usa `libjpeg-turbo` para leerlo, que **no soporta JPEG lossless** — dato del README de hbmenu).

### 2.2 Makefile completo y comentado (SDL2 + curl + libpng + zlib/minizip + freetype + C++20)

Basado en el template oficial (`templates/application/Makefile`) y en el ejemplo curl (`network/curl/Makefile`, que usa `` `curl-config --cflags/--libs` ``). Guardar como `Makefile` en la raíz del proyecto.

```make
#---------------------------------------------------------------------------------
# QuiverNX — NRO launcher/instalador
# Basado en devkitPro/switchbrew templates/application + network/curl example
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Metadatos de la app (van al .nacp)
#---------------------------------------------------------------------------------
TARGET      := QuiverNX          # nombre del .nro resultante
BUILD       := build
SOURCES     := source
DATA        := data
INCLUDES    := include
ROMFS       := romfs             # descomentar/definir si quieres RomFS

APP_TITLE   := QuiverNX
APP_AUTHOR  := TuNombre
APP_VERSION := 1.0.0
# APP_TITLEID := 0100000000000000   # opcional (16 hex). OJO: no colisionar.
ICON        := icon.jpg          # relativo al proyecto; debe existir y ser JPEG
# NO_ICON / NO_NACP: definir para omitir icono/nacp

#---------------------------------------------------------------------------------
# Opciones de compilación
#---------------------------------------------------------------------------------
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

# OJO: -fno-exceptions / -fno-rtti son el default de devkitPro (binarios mucho más
# pequeños). Si usas std::filesystem o librerías que lanzan excepciones, quítalos.
CFLAGS := -g -Wall -O2 -ffunction-sections -fdata-sections \
          $(ARCH) $(DEFINES)

# curl provee sus flags vía pkg-config (curl-config está en portlibs/switch/bin)
CURL_CFLAGS := $(shell curl-config --cflags)
CURL_LIBS   := $(shell curl-config --libs)

# pkg-config para el resto de portlibs
PKG_CONFIG_FLAGS := $(shell pkg-config --cflags freetype2 libpng zlib)
PKG_CONFIG_LIBS  := $(shell pkg-config --libs   freetype2 libpng zlib)

CFLAGS   += $(INCLUDE) -D__SWITCH__ $(CURL_CFLAGS) $(PKG_CONFIG_FLAGS)
CXXFLAGS := $(CFLAGS) -std=c++20 -fno-rtti -fno-exceptions

ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# Librerías a enlazar
#   -lnx        : libnx (SIEMPRE)
#   -lSDL2      : SDL2  (usa sdl2-config si prefieres: $(shell sdl2-config --libs))
#   -lcurl      : libcurl (TLS vía servicio ssl de libnx)
#   -lminizip   : libminizip -> la provee switch-zlib
#   -lz -lpng -ljpeg -lfreetype -lbz2 : resto
#   -lnx SIEMPRE al final (o al menos después de las portlibs que la usan)
#---------------------------------------------------------------------------------
LIBS := -lSDL2 -lSDL2_image -lcurl -lminizip -lpng -ljpeg -lfreetype -lbz2 -lz -lm -lnx

#---------------------------------------------------------------------------------
# Directorios con include/ y lib/  (portlibs primero, libnx después)
#---------------------------------------------------------------------------------
LIBDIRS := $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# A partir de aquí no hace falta tocar (excepto añadir reglas bin2o)
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                  $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

# Enlaza con CXX si hay fuentes C++ (nuestro caso)
ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES     := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# --- NACP/AUTODETECCIÓN DE ICONO (copiado del template oficial) ---------------
ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif
ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif
ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif
ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif
# -----------------------------------------------------------------------------

.PHONY: $(BUILD) clean all

all: $(BUILD)

# Nota: los comandos usan mkdir/rm/cp POSIX -> requieren el MSYS de devkitPro,
# NO funciona en cmd.exe/PowerShell nativo.
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
ifeq ($(strip $(APP_JSON)),)
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
	@rm -fr $(BUILD) $(TARGET).nsp $(TARGET).nso $(TARGET).npdm $(TARGET).elf
endif

#---------------------------------------------------------------------------------
else
.PHONY: all

DEPENDS := $(OFILES:.o=.d)

# Target principal: NRO que depende del ELF y del NACP
ifeq ($(strip $(APP_JSON)),)
all : $(OUTPUT).nro
ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro : $(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro : $(OUTPUT).elf
endif
else
all : $(OUTPUT).nsp
$(OUTPUT).nsp : $(OUTPUT).nso $(OUTPUT).npdm
$(OUTPUT).nso : $(OUTPUT).elf
endif

$(OUTPUT).elf : $(OFILES)
$(OFILES_SRC) : $(HFILES_BIN)

# Regla para datos binarios embebidos (opcional): data/foo.bin -> build/foo.bin.o
%.bin.o %_bin.h : %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)
endif
```

### 2.3 El icono: cómo se genera/necesita

- **No se genera.** `switch_rules` sólo hace `nacptool` para el `.nacp`. El icono es entrada.
- Debe ser **JPEG** (`.jpg`). `elf2nro --icon=...` lo incrusta en la sección `NroAssetHeader`.
- Ruta por defecto: `<TOPDIR>/icon.jpg` (o `$(TARGET).jpg`, o `$(LIBNX)/default_icon.jpg`).
- El NACP se autogenera: `$(OUTPUT).nacp : $(MAKEFILE_LIST)` → `nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@`.
  Como depende de `$(MAKEFILE_LIST)`, **cambiar `APP_VERSION` en el Makefile regenera el `.nacp`**.
- Limitación real: hbmenu usa `libjpeg-turbo`, que **no soporta JPEG lossless**.
- **Sphaira** lee el icono directamente del NRO (offset `header.size + asset.icon.offset`) — ver §7.

**Comando de build desde MSYS de devkitPro:**
```bash
cd /c/Users/tu_usuario/proyectos/QuiverNX
make -j$(nproc)
# -> QuiverNX.nro, QuiverNX.nacp, QuiverNX.elf
# Enviar al Switch por red:
nxlink -s QuiverNX.nro
```

---

## 3. LIBRERÍAS DISPONIBLES (HTTP/TLS, JSON, ZIP, 7z, imágenes, fuentes, hilos, FS)

### 3.1 HTTP(S) con TLS — **sin CA bundle**

**Cómo funciona de verdad (evidencia fuerte).** `switch-curl/PKGBUILD`:
```
./configure --prefix=$PORTLIBS_PREFIX --host=aarch64-none-elf \
  --disable-shared --enable-static --disable-ipv6 --disable-unix-sockets \
  ... --without-ssl --without-mbedtls --with-libnx \
  --with-default-ssl-backend=libnx
```
Y el parche `switch-curl.patch` **añade un backend TLS nuevo**, `lib/vtls/libnx.c`, que:
- inicializa el servicio con `sslInitialize(0x3)` y `csrngInitialize()`;
- crea contexto con `sslCreateContext(&ctx, ssl_version)`;
- **NO** importa ningún CA por defecto;
- hace el handshake con `sslConnectionDoHandshake(...)`;
- expone `{ CURLSSLBACKEND_LIBNX, "libnx" }` y `Curl_ssl_libnx`.

Consecuencias prácticas:
1. **No necesitas `curl-ca-bundle` ni `--cacert`.** La verificación la hace el servicio `ssl` de Horizon con el **CertStore del sistema** ("The CertStore is used automatically, regardless of what cmds are used" — doc de `sslCreateContext` en `nx/include/switch/services/ssl.h`).
2. Si **sí** pasas `CURLOPT_CAINFO`/`--cacert`, el backend ejecuta `sslContextImportServerPki(ctx, buf, size, SslCertificateFormat_Pem, NULL)` para importar tu PEM. Útil para CAs privadas.
3. **No existe `setDefaultCertStore`** en libnx. Si lo viste, es de otra librería (no de libnx).
4. Límite real: *"ssl-service only supports TLS 1.0-1.2"* (comentario en `libnx.c`). TLS 1.3 sólo aparece como constantes en libnx desde [11.0.0]; `libnx_version_from_curl` **no** mapea `CURL_SSLVERSION_TLSv1_3` (cae en `CURLE_SSL_CONNECT_ERROR`). Para GitHub/GitLab (TLS 1.2) es suficiente.

**Snippet completo: descargar a memoria con libcurl en Switch.**
```cpp
#include <switch.h>
#include <curl/curl.h>
#include <cstdio>
#include <vector>
#include <string>

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::vector<unsigned char>*>(userdata);
    const size_t n = size * nmemb;
    out->insert(out->end(), ptr, ptr + n);
    return n;
}

// Devuelve true si OK. Sigue redirects (GitHub releases -> objects.githubusercontent.com).
bool http_get(const std::string& url, std::vector<unsigned char>& out, long* http_code = nullptr) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "QuiverNX/1.0");   // GitHub API lo exige
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);   // importante: newlib/Switch

    // NO hace falta CA bundle: el backend libnx usa el CertStore del sistema.
    // Para una CA privada: curl_easy_setopt(curl, CURLOPT_CAINFO, "sdmc:/config/ssl/mi-ca.pem");

    const CURLcode rc = curl_easy_perform(curl);
    if (http_code) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK;
}
```
En el Makefile: `LIBS := ... -lcurl -lnx` y `CFLAGS += $(shell curl-config --cflags)`.
Socket init: libnx requiere `socketInitializeDefault()` (lo hace `__appInit` si enlazas con `-lnx`, pero si gestionas servicios manualmente, llámalo y luego `socketExit()`).

### 3.2 JSON

| Opción | Paquete pacman | Tipo | Notas |
|---|---|---|---|
| **yyjson** | *(no empaquetado)* — FetchContent | C, single-file | **Lo que usa Sphaira** (`YYJSON_*` options + `target_link_libraries(... yyjson)`). Rápido, sin STL. Recomendado. |
| jansson | **`switch-jansson`** | C, requiere link `-ljansson` | Verificado: `pkgname=switch-jansson`. |
| json-c | **`switch-libjson-c`** | C, `-ljson-c` | Verificado en el listing. |
| nlohmann/json | **no empaquetado** | header-only C++ | Copia `json.hpp` a `include/`. Cómodo pero añade tiempo de compilación. |
| rapidjson | **no empaquetado** | header-only C++ | Copia `rapidjson/`. |

**Snippet: parsear la GitHub Releases API con yyjson** (para `application/vnd.github+json`, `GET /repos/{owner}/{repo}/releases/latest`) extrayendo `tag_name` y cada asset (`name`, `browser_download_url`, `size`):

```cpp
#include <yyjson.h>
#include <string>
#include <vector>

struct Asset { std::string name, url; uint64_t size; };

struct Release {
    std::string tag;
    std::vector<Asset> assets;
};

bool parse_github_release(const std::vector<unsigned char>& body, Release& out) {
    yyjson_doc* doc = yyjson_read(reinterpret_cast<const char*>(body.data()),
                                  body.size(), 0);
    if (!doc) return false;

    yyjson_val* root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) { yyjson_doc_free(doc); return false; }

    if (yyjson_val* tag = yyjson_obj_get(root, "tag_name"); yyjson_is_str(tag))
        out.tag = yyjson_get_str(tag);

    if (yyjson_val* assets = yyjson_obj_get(root, "assets"); yyjson_is_arr(assets)) {
        size_t idx, max;
        yyjson_val* a;
        yyjson_arr_foreach(assets, idx, max, a) {
            Asset as;
            if (yyjson_val* v = yyjson_obj_get(a, "name"); yyjson_is_str(v))
                as.name = yyjson_get_str(v);
            if (yyjson_val* v = yyjson_obj_get(a, "browser_download_url"); yyjson_is_str(v))
                as.url = yyjson_get_str(v);
            if (yyjson_val* v = yyjson_obj_get(a, "size"); yyjson_is_uint(v))
                as.size = yyjson_get_uint(v);
            if (!as.name.empty() && !as.url.empty())
                out.assets.push_back(std::move(as));
        }
    }

    yyjson_doc_free(doc);
    return true;
}
```
Con jansson, los equivalentes son `json_loadb()`, `json_object_get()`, `json_array_foreach()`.

### 3.3 ZIP

**`switch-zlib` ya trae `libminizip`.** Del `PKGBUILD`:
```make
  # libminizip
  cd contrib/minizip
  autoreconf --force --verbose --install
  CFLAGS="${CFLAGS} -DUSE_FILE32API"
  ./configure --prefix="${PORTLIBS_PREFIX}" --host=aarch64-none-elf \
    --disable-shared --enable-static
  make
```
Es el **minizip clásico** (`unzOpen`/`unzOpenCurrentFile`/`unzReadCurrentFile`), no minizip-ng. Enlaza `-lminizip`.

Evidencia de que es suficiente en la práctica: **Sphaira** hace
`find_library(minizip_lib minizip REQUIRED)` y `find_path(minizip_inc minizip REQUIRED)`, y tiene `source/minizip_helper.cpp`.

```cpp
#include <minizip/unzip.h>   // o <unzip.h> según cómo instale el paquete
#include <switch.h>
#include <vector>
#include <cstdio>
#include <cstring>

// Extrae una entrada concreta de un .zip a memoria.
bool zip_extract_to_memory(const char* zip_path, const char* entry_name,
                           std::vector<unsigned char>& out) {
    unzFile zf = unzOpen(zip_path);
    if (!zf) return false;

    if (unzLocateFile(zf, entry_name, 0) != UNZ_OK) { unzClose(zf); return false; }
    if (unzOpenCurrentFile(zf) != UNZ_OK)            { unzClose(zf); return false; }

    unsigned char buf[64 * 1024];
    int n;
    while ((n = unzReadCurrentFile(zf, buf, sizeof(buf))) > 0)
        out.insert(out.end(), buf, buf + n);

    unzCloseCurrentFile(zf);
    unzClose(zf);
    return n >= 0;
}

// Listar entradas y extraer todas a sdmc:/...
void zip_extract_all(const char* zip_path, const char* dest_dir) {
    unzFile zf = unzOpen(zip_path);
    if (!zf) return;

    unz_global_info gi{};
    unzGetGlobalInfo(zf, &gi);
    unzGoToFirstFile(zf);

    for (uLong i = 0; i < gi.number_entry; ++i) {
        char name[256] = {0};
        unz_file_info fi{};
        unzGetCurrentFileInfo(zf, &fi, name, sizeof(name), nullptr, 0, nullptr, 0);

        // Un .nro dentro del zip:
        if (strstr(name, ".nro")) {
            std::vector<unsigned char> data;
            if (unzOpenCurrentFile(zf) == UNZ_OK) {
                unsigned char buf[64 * 1024];
                int n;
                while ((n = unzReadCurrentFile(zf, buf, sizeof(buf))) > 0)
                    data.insert(data.end(), buf, buf + n);
                unzCloseCurrentFile(zf);

                char full[512];
                snprintf(full, sizeof(full), "%s/%s", dest_dir, name);
                if (FILE* f = fopen(full, "wb")) {
                    fwrite(data.data(), 1, data.size(), f);
                    fclose(f);
                }
            }
        }
        if (unzGoToNextFile(zf) != UNZ_OK) break;
    }
    unzClose(zf);
}
```
> `-DUSE_FILE32API` en el PKGBUILD hace que minizip use `fopen`/`fseek` de 32 bits (`long`) en vez de `fopen64`; compatible con newlib.

### 3.4 7z

**No hay librería 7z empaquetada para Switch.** No existe `switch-7zip` ni similar en `pacman-packages`. Alternativas verificadas:
- **`switch-libarchive`** — sí existe. `depends=('switch-bzip2' 'switch-zlib' 'switch-liblzma' 'switch-lz4' 'switch-libexpat' 'switch-libzstd')`. Soporta zip/tar/**7z** (LZMA vía `switch-liblzma`) con `archive_read_*`. Tamaño notable.
- **`switch-libzstd`, `switch-lz4`, `switch-liblzma`, `switch-bzip2`** — para compresión suelta.
- Comprimir en **ZIP** en tus releases es lo pragmático: minizip ya está y no añade peso.

> No verifiqué el tamaño exacto del binario con libarchive; mídlo si te importa el tamaño del `.nro`.

### 3.5 PNG/JPEG

Paquetes verificados: **`switch-libpng`** (`depends=('switch-zlib')`), **`switch-libjpeg-turbo`**, **`switch-libwebp`**, **`switch-sdl2_image`** (que `depends=(switch-sdl2 switch-libpng switch-libjpeg-turbo switch-libwebp)`), STB es header-only vía FetchContent (lo que usa Sphaira).

Vía SDL2_image (más simple si ya usas SDL2):
```cpp
#include <SDL2/SDL_image.h>
SDL_RWops* rw = SDL_RWFromMem(data.data(), (int)data.size());
SDL_Surface* surf = IMG_Load_RW(rw, 1);      // 1 = liberar rw
SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
SDL_FreeSurface(surf);
```
Vía libpng desde memoria:
```cpp
#include <png.h>
bool decode_png(const unsigned char* data, size_t len, int& w, int& h,
                png_bytep* rgba_out) {
    png_image img{};
    img.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&img, data, len)) return false;
    img.format = PNG_FORMAT_RGBA;
    *rgba_out = (png_bytep)malloc(PNG_IMAGE_SIZE(img));
    const bool ok = png_image_finish_read(&img, nullptr, *rgba_out, 0, nullptr);
    w = img.width; h = img.height;
    return ok;
}
```
> Importante para QuiverNX: el **icono del NRO es JPEG**, así que para previsualizar iconos de homebrew necesitas decodificador **JPEG** (libjpeg-turbo o SDL2_image), no PNG. Sphaira usa `nvjpg` (decodificación por hardware) opcionalmente, y stb/nanovg para el resto.

### 3.6 Fuentes TTF

Paquetes verificados: **`switch-freetype`** (2.13.3; `depends=(switch-bzip2 switch-libpng switch-zlib)`) y **`switch-harfbuzz`** (10.0.1; `depends=('switch-freetype')`; construido con ICU/glib/cairo deshabilitados).

```cpp
#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library ft;
FT_Init_FreeType(&ft);
FT_Face face;
FT_New_Face(ft, "sdmc:/switch/QuiverNX/fonts/Inter.ttf", 0, &face);
FT_Set_Pixel_Sizes(face, 0, 24);

// Cargar un glifo y volcarlo a un bitmap
FT_Load_Char(face, 'A', FT_LOAD_RENDER);
const FT_Bitmap& bmp = face->glyph->bitmap;
// bmp.buffer, bmp.width, bmp.rows, bmp.pitch -> subir a textura
```
**HarfBuzz sólo si necesitas shaping complejo** (árabe, devanagari, ligaduras). Para español/inglés con freetype + kerning básico es suficiente y ahorras peso. hbmenu usa freetype directo (paquete `switch-freetype`) y su propio atlas (`common/font.c`).

### 3.7 Hilos y sincronización

**No puedo confirmar pthreads en devkitA64.** El `pthread.h` que encuentro (`newlib/libc/include/pthread.h`) es de **newlib upstream** y cubre targets como RTEMS; no he verificado que la build de newlib de devkitA64 para `aarch64-none-elf` enlace una implementación pthread. `libnx` **no** expone pthreads (sólo `switch/kernel/thread.h`). No lo afirmo en ningún sentido.

**Lo que sí usan los proyectos reales y está documentado: la API de hilos de libnx.**
Sphaira (`sphaira/source/yati/yati.cpp`, `InstallNcaInternal`) crea 3 hilos con `utils::CreateThread(&t, fn, arg, 1024*64)` y pinnea a cores:
```cpp
#define READ_THREAD_CORE       1
#define DECOMPRESS_THREAD_CORE 2
#define WRITE_THREAD_CORE      0
```
```cpp
#include <switch.h>

static void worker(void* arg) {
    // ...
}

Thread t;
threadCreate(&t, worker, nullptr, nullptr, 0x10000 /*stack*/, 0x2 /*prio*/,
            /*cpuid*/ 1);
threadStart(&t);
threadWaitForExit(&t);
threadClose(&t);
```
Sincronización: `Mutex`, `CondVar`, `UEvent` (`mutexInit/mutexLock`, `condvarInit/condvarWait`, `ueventInit/ueventWait`). No requiere `-lpthread`.

### 3.8 Almacenamiento (`sdmc:`, FAT)

- `fsdevMountSdmc()` monta la SD como `sdmc:` (lo hace libnx en `__appInit` por defecto).
- Se puede usar **newlib stdio** (`fopen("sdmc:/switch/QuiverNX/config.json", "rb")`) — es lo que hace hbmenu (`fopen(me->path, "rb")` en `nx_launch.c`).
- O las **APIs `fs` de libnx** (`fsOpenFileSystemWithId`, `fsFsOpenFile`, `fsFileRead`, `fsFsCreateDirectory`, `fsFsDeleteFile`) — lo que hace Sphaira vía sus wrappers `fs::FsNativeSd`.
- **Modo texto vs binario:** en Windows usas `"rb"/"wb"`; en Switch newlib **no** traduce CRLF, así que `"r"` y `"rb"` son equivalentes. Usa `"rb"/"wb"` por claridad y portabilidad del código.
- Crear directorios: `mkdir("sdmc:/switch/QuiverNX", 0777)` (newlib) o `fsFsCreateDirectory`.
- **Escritura:** `sdmc:` es escribible desde homebrew en general (la excepción histórica es FAT32 con la SD montada por el sistema en ciertos casos; no reproducible aquí). Sphaira escribe ficheros en `sdmc:` sin problema (`fs.write_entire_file(path, ...)` en `nro.cpp`).

**JSON seguro (patrón atómico, igual que Sphaira en `nro_update_info`):**
```cpp
// 1) escribir a temporal  2) renombrar sobre el original.
// Sphaira hace backup .sphaira.bak y restaura si falla la escritura.
bool write_json_atomic(const char* path, const std::string& json) {
    const std::string tmp = std::string(path) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    const bool ok = fwrite(json.data(), 1, json.size(), f) == json.size();
    fclose(f);
    if (!ok) { remove(tmp.c_str()); return false; }

    // Renombrar: en newlib/FAT, remove+rename (rename sobre existente puede fallar en FAT)
    remove(path);
    if (rename(tmp.c_str(), path) != 0) { remove(tmp.c_str()); return false; }
    return true;
}
```
Escritura de config **fuera del hilo de UI** (Sphaira usa `threaded_file_transfer.cpp`) para no bloquear el render.

---

## 4. UI: cómo se hace una UI estilo Switch OS

### 4.1 Cómo renderiza Sphaira su UI (código real, `master`)

**Veredicto: NO usa SDL2 ni borealis.** Stack real, de `sphaira/sphaira/CMakeLists.txt`:

| Capa | Tecnología | Evidencia |
|---|---|---|
| GPU / backend | **deko3d** (paquete `deko3d`) | `pacman -S deko3d` en el README; `nv`/`nvg` en el código |
| Render 2D vectorial | **nanovg** sobre deko3d | `FetchContent_Declare(nanovg GIT_REPOSITORY https://github.com/ITotalJustice/nanovg-deko3d.git GIT_TAG 2acb0f3)`; `set(NANOVG_NO_JPEG OFF)`, `set(NANOVG_NO_PNG OFF)` |
| Render de alto nivel | **libpulsar** | `FetchContent_Declare(libpulsar GIT_REPOSITORY https://github.com/ITotalJustice/switch-libpulsar.git GIT_TAG ac7bc97)`; `target_link_libraries(sphaira PRIVATE libpulsar ...)` |
| Imágenes | **stb** (+ `nvjpg` opcional por HW) | `FetchContent_Declare(stb ... nothings/stb ...)`, `option(ENABLE_NVJPG "" OFF)`, `oss-nvjpg` |
| Texto | nanovg + fuente cargada en **romfs** | `add_library(stb INTERFACE)`; textos vía `gfx::drawText(vg, ...)` |
| JSON | **yyjson** | `FetchContent_Declare(yyjson ... 0.12.0)` + `YYJSON_*` |
| Config | **minIni** | `FetchContent_Declare(minIni ... ITotalJustice/minIni-nx.git)` |
| ZIP | **minizip** (el de `switch-zlib`) | `find_library(minizip_lib minizip REQUIRED)` + `source/minizip_helper.cpp` |
| Crypto (NSP) | **mbedtls** (`switch-mbedtls`) | `find_library(mbedcrypto_lib mbedcrypto REQUIRED)`; `find_path(mbedtls_inc mbedtls REQUIRED)` |
| NSP/NCA/ncm | **YATI** (propio) + nxdumptool | `source/yati/container/nsp.cpp`, `source/yati/nx/{ncm,nca,es,ns,ncz}.cpp`, `source/yati/nx/nxdumptool_rsa.c` |
| Loader propio | **hbl** embebido | `add_dependencies(sphaira hbl_nso hbl_npdm)`, `--embed-dir=${CMAKE_BINARY_DIR}/hbl` |

**UI/focus/gamepad (de `sphaira/source/ui/` y `forwarder_editor.cpp`):**
- Sistema de **`Widget`** con árbol de UI: `source/ui/widget.cpp`, `include/ui/widget.hpp`, `include/ui/object.hpp`.
- Listas con foco, scroll y scrollbar: `source/ui/list.cpp` (`class List`, `m_list->OnUpdate(controller, touch, m_index, rows, callback)`), `sidebar.cpp`, `grid_menu_base.cpp`.
- **Botones y acciones declarativas**: `SetActions(std::make_pair(Button::A, Action{"Select", [this](){ Activate(); }}), std::make_pair(Button::B, Action{"Back", ...}))`.
- **Gamepad**: clase propia `Controller` con `controller->GotDown(Button::UP/DOWN/LEFT/RIGHT)`; **touch** con `touch->is_clicked`, `touch->in_range(rect)`. No usa `padGetButtons` directo en la UI, sino su wrapper.
- **Temas**: `Theme` + `ThemeEntryID_*` (`ThemeEntryID_BACKGROUND`, `_TEXT`, `_TEXT_SELECTED`, `_TEXT_INFO`, `_LINE`, `_GRID`) vía `theme->GetColour(...)`. Sphaira tiene menú de temas y descarga temas de **Themezer** (`source/ui/menus/themezer.cpp`).
- **Animaciones**: transiciones en los widgets (p. ej. `scrolling_text.cpp`, `scrollable_text.cpp`) + `DrawElement`/`gfx::drawRect` en `source/ui/nvg_util.cpp`.
- **Sonidos**: `App::PlaySoundEffect(SoundEffect::Focus)` en el foco.
- **Pantallas**: 1280x720 (`SCREEN_WIDTH`, `SCREEN_HEIGHT`).

> Nota: existe **otro** proyecto llamado `switch-libpulsar` (`p-sam/switch-libpulsar`) que es una librería de **audio BFSAR**, sin relación. Sphaira usa el de **ITotalJustice**. No los confundas.

### 4.2 Borealis

- Repo: https://github.com/natinusala/borealis — rama **`main`**, 307 stars, **último push 2023-10-31** (no archivado, pero **sin actividad desde 2023**).
- Contiene `Makefile` en la raíz y `library/lib/switch_wrapper.c`; el árbol incluye `library/lib/extern/fmt`, `glm`, `tinyxml2`, `yoga`, `nanovg`, `glfw` (deps típicas de borealis).
- **No pude verificar con evidencia concluyente** que exista un backend deko3d nativo listo para devkitPro en la rama `main` actual (el árbol que inspeccioné no mostró un directorio de backend deko3d dedicado, sólo `switch_wrapper.c`). Históricamente borealis en Switch va por **OpenGL/EGL (mesa)** con `switch_wrapper.c`. **Verifícalo antes de decidir**: `git clone` y revisa `library/lib/` y el README de integración.
- Coste de integración en un Makefile devkitPro: añadir borealis como submódulo e `include` de su Makefile, más las deps (`glm`, `fmt`, `tinyxml2`, `yoga`, `nanovg`, `glfw`/`mesa`). Si va por GL, necesitas **`switch-mesa`** y el ejemplo `graphics/opengl/*`. Requiere **excepciones y RTTI activadas** (rompe el default `-fno-exceptions -fno-rtti`), lo que **hincha el `.nro`** — problema real en applet mode (memoria limitada).

### 4.3 Pluto / Plutonium

- **Plutonium** existe y es real: https://github.com/XorTroll/Plutonium — *"An easy-to-use UI framework for Nintendo Switch homebrew"*. Está **sin mantenimiento** (usado por homebrew antiguo tipo TinWoo; hay forks como `drizzt/Plutonium`).
- **"Pluto" como librería de UI separada: no pude verificarlo.** Todas mis búsquedas devuelven **Plutonium**. No afirmo que exista un `Pluto` distinto.
- Ambos están **obsoletos** frente a deko3d+nanovg. No los recomiendo para un proyecto nuevo.

### 4.4 deko3d vs SDL2: qué implementas tú

| Necesidad | SDL2 puro | deko3d + nanovg | borealis |
|---|---|---|---|
| Ventana/framebuffer | gratis | tú (swapchain, cmd buffers) | gratis |
| Rectángulos/texto/imágenes | primitivas básicas + SDL2_ttf | nanovg (paths, antialias, texto) | widgets ya hechos |
| Layout (filas, scroll) | **tú** | **tú** | incluido (yoga) |
| Focus/animation/temas | **tú** | **tú** | incluido |
| Look "Switch OS" | hay que construirlo | alcanzable (es lo que hace Sphaira) | genérico borealis |
| Peso/excepciones | bajo | bajo | **alto** (fmt/glm/yoga/GL) |
| Mantenimiento | activo (devkitPro) | activo | **parado 2023** |

### 4.5 Recomendación razonada para QuiverNX

1. **Si quieres el look Sphaira de verdad y control total → deko3d + nanovg** (como Sphaira). Es el camino con mejor resultado visual y el que ya está probado en este caso de uso exacto. Coste: alto (gestión de swapchain, atlas de fuentes, sistema de layout/focus propio). Referencia directa a copiar: `nvg_util.cpp`, `widget.cpp`, `list.cpp` de Sphaira.
2. **Si quieres iterar rápido y priorizar funcionalidad (descargar/instalar/lanzar) sobre el pixel-perfect → SDL2 + SDL2_image + SDL2_ttf.** Mucho menos código de infraestructura, paquetes `switch-sdl2*` verificados, y sigue siendo tú quien dibuja el "look Switch". Es la opción que recomiendo **para la primera versión** de QuiverNX, con arquitectura de UI separada para poder migrar el renderer a deko3d después.
3. **Borealis:** sólo si aceptas dependencia parada desde 2023, peso alto (excepciones/RTTI, GL/mesa) y un look genérico. **No** lo recomiendo para un launcher que debe caber en applet mode.
4. **Plutonium/Pluto:** descartar (obsoleto / no verificable).

---

## 5. LANZAR OTROS NRO / HOME* (la parte crítica)

### 5.1 `envSetNextLoad` — qué hace EXACTAMENTE

Firma real (`nx/include/switch/runtime/env.h`):
```c
/**
 * @brief Configures the next homebrew application to load.
 * @param path Path to the next homebrew application to load (.nro).
 * @param argv Argument string to pass.
 */
Result envSetNextLoad(const char* path, const char* argv);

/// Returns true if the environment supports envSetNextLoad.
bool envHasNextLoad(void);
```

Implementación real (`nx/source/runtime/env.c`):
```c
Result envSetNextLoad(const char* path, const char* argv)
{
    if (g_nextLoadPath == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    strcpy(g_nextLoadPath, path);

    if (g_nextLoadArgv != NULL)
    {
        if (argv == NULL)
            g_nextLoadArgv[0] = '\0';
        else
            strcpy(g_nextLoadArgv, argv);
    }

    return 0;
}

bool envHasNextLoad(void) {
    return g_nextLoadPath != NULL;
}
```

**Conclusión decisiva (esto es la clave de todo el diseño):** `envSetNextLoad` **no lanza nada**. Sólo hace `strcpy` en **dos buffers que pertenecen al loader que te lanzó**, obtenidos de las entradas de config `EntryType_NextLoadPath`:
```c
case EntryType_NextLoadPath:
    g_nextLoadPath = (char*) ent->Value[0];   // buffer de 512 bytes
    g_nextLoadArgv = (char*) ent->Value[1];   // buffer de 2048 bytes
    break;
```
Si el loader **no** pasó `NextLoadPath`, devuelve `LibnxError_NotInitialized` y **no hay forma de lanzar otro NRO**. Consúltalo siempre con `envHasNextLoad()` antes.

Especificación oficial (https://switchbrew.org/wiki/Homebrew_ABI):
> **NextLoadPath** — *"Homebrew menu uses this pointer to write the path of next NRO to load, before returning back to Homebrew loader."*
> - `Value[0]`: Pointer to buffer of size **512**. NRO file path, should start with `"sdmc:/"`.
> - `Value[1]`: Pointer to buffer of size **2048**. Argv string to boot NRO with.

**El patrón completo es: `envSetNextLoad(...)` + salir del `main` (o `return` desde el bucle) para devolver el control al loader**, que entonces carga el NRO indicado. NO se salta inmediatamente.

```cpp
#include <switch.h>
#include <cstdio>
#include <cstring>

// Lanza otro NRO. Devuelve false si el entorno no lo soporta.
bool launch_nro(const char* nro_path, const char* argv_str) {
    if (!envHasNextLoad()) {
        // Estamos en un loader que no soporta encadenar NROs
        // (p. ej. NRO lanzado por applet sin hbl, o ciertas rutas de applet mode).
        return false;
    }
    const Result rc = envSetNextLoad(nro_path, argv_str);
    if (R_FAILED(rc)) return false;
    return true;   // el llamador debe salir del bucle principal -> vuelve al loader
}

// Uso típico:
//   if (launch_nro("sdmc:/switch/QuiverNX/apps/foo.nro", "\"sdmc:/switch/QuiverNX/apps/foo.nro\""))
//       g_should_exit = true;   // rompe el bucle y hace return
```
> Comillas: si la ruta contiene espacios, **debe ir entre comillas dobles** dentro del string argv. Sphaira lo hace así (`nro_add_arg`: si contiene espacio, `'"' + arg + '"'`), y hbmenu emite `"\"" arg "\""` para cada argumento.

### 5.2 `AppletJumpToApplication`, `__nx_applet_type`

- **`envAppletJumpToApplication` no existe.** No hay ningún símbolo así en libnx.
- **`AppletJumpToApplication` no existe.** Grep exhaustivo sobre `nx/include/switch/services/applet.h`: **0** coincidencias de `JumpToApplication`. Lo que existe para lanzar títulos es la familia `appletRequestToLaunchApplication*` (que es lo que usa el sistema/applets, **no** homebrew para encadenar NROs).
- **`__nx_applet_type` sí existe** y es el mecanismo real de "override":
```c
extern __attribute__((weak)) u32 __nx_applet_type;
...
case EntryType_AppletType:
    __nx_applet_type = ent->Value[0];
    if ((ent->Value[1] & EnvAppletFlags_ApplicationOverride) &&
         __nx_applet_type == AppletType_SystemApplication)
        __nx_applet_type = AppletType_Application;
    break;
```
Es decir: el **loader** decide el tipo; se declara vía la entrada `EntryType_AppletType` (Key 7, `Value[0]`=AppletType, `Value[1]`=Flags). `EnvAppletFlags_ApplicationOverride = BIT(0)` convierte `SystemApplication` → `Application`. El ABI lo especifica:
> **AppletType** — Key 7. `Value[0]`: AppletType. `Value[1]`: AppletFlags. ApplicationOverride: *"Use AppletType_Application instead of AppletType_SystemApplication, only when AppletType is SystemApplication."*

**Cuándo funciona `envSetNextLoad`:** cuando el proceso que te lanzó puso `NextLoadPath` en tus config entries **y** ese loader sigue vivo para recogerte al salir. En la práctica: **hbmenu (vía hbloader/hbl) y Sphaira** — ambos lo hacen. Si te lanza el **Home Menu** directamente vía title override sin hbl, no hay `NextLoadPath` y `envHasNextLoad()` es `false`.

### 5.3 Cómo lo hacen hbmenu y Sphaira (código real)

**hbmenu — `nx_main/loaders/builtin.c`** (el loader "builtin"):
```c
static bool init(void)
{
    return envHasNextLoad();          // <-- hbmenu sólo habilita el loader si el entorno lo permite
}

static void launchFile(const char* path, argData_s* args)
{
    char msg[256];
    memset(argBuf, 0, sizeof(argBuf));

    uint32_t remote = args->nxlink_host.s_addr;
    if (remote) {
        char nxlinked[17];
        sprintf(nxlinked, "%08" PRIx32 "_NXLINK_", remote);
        launchAddArg(args, nxlinked);
    }

    init_args(argBuf, sizeof(argBuf)-1, args->buf, sizeof(args->buf));  // empaqueta argv

    struct stat st;
    if (stat(path, &st) == -1) {
        // ... msgbox "NRO not found"
    } else {
        Result rc = envSetNextLoad(path, argBuf);        // <-- escribe el buffer del loader
        if (R_FAILED(rc)) {
            // ... msgbox con el Result
        } else {
            uiExitLoop();                                // <-- y sale del bucle
        }
    }
}
```
Y el empaquetado de argv (`init_args` en el mismo archivo) produce **cada argumento entre comillas dobles, separados por espacio**:
```c
*dst++ = '"';
strncpy(dst, in_argdata, tmplen);
dst += tmplen;
*dst++ = '"';
if (argi+1 < in_args[0]) { *dst++ = ' '; }
```
(`static char argBuf[ENTRY_ARGBUFSIZE];` con `#define ENTRY_ARGBUFSIZE 0x400` en `common/menu.h`, y `argData_s::buf` de 0x400 → **límite de ~1024 bytes para el argv que puedes pasar a un NRO desde hbmenu**.)

**Sphaira — `sphaira/source/nro.cpp`:**
```cpp
auto launch_internal(const std::string& path, const std::string& argv) -> Result {
    R_TRY(envSetNextLoad(path.c_str(), argv.c_str()));
    log_write("set launch with path: %s argv: %s\n", path.c_str(), argv.c_str());
    evman::push(evman::LaunchNroEventData{path, argv});
    R_SUCCEED();
}

auto nro_launch(std::string path, std::string args) -> Result {
    if (path.empty()) return 1;

    // keeps compat with hbloader
    // https://github.com/ITotalJustice/Gamecard-Installer-NX/blob/master/source/main.c#L73
    if (!path.starts_with("sdmc:")) {
        path = "sdmc:" + path;
    }

    if (args.empty()) {
        args = nro_add_arg(path);
    } else {
        args = nro_add_arg(path) + ' ' + args;
    }
    return launch_internal(path, args);
}

auto nro_add_arg(std::string arg) -> std::string {
    if (arg.contains(' ')) return '\"' + arg + '\"';
    return arg;
}
```
> **Detalle importantísimo:** Sphaira **prefija `sdmc:` si falta** y **siempre pone la ruta del NRO como primer argumento** (`argv[0]`). Deja el comentario explícito *"keeps compat with hbloader"* enlazando `Gamecard-Installer-NX`. Para máxima compatibilidad haz lo mismo: `argv = "\"<ruta>\" <tus args>"`.

### 5.4 El Forwarder NSP: qué pasa y la cadena real de arranque

**Respuesta corta: sí, `envSetNextLoad` sigue funcionando desde un Forwarder NSP** — porque el forwarder **embebe un hbloader** como `main` del ExeFS, y hbloader **sí provee `NextLoadPath`**.

**Evidencia dura — `switchbrew/nx-hbloader/source/main.c`** (el loader que va dentro de los forwarders):
```c
static char g_argv[2048];
static char g_nextArgv[2048];
static char g_nextNroPath[512];
static bool g_isApplication = 0;
```
```c
    static ConfigEntry entries[] = {
        { EntryType_MainThreadHandle,     0, {0, 0} },
        { EntryType_ProcessHandle,        0, {0, 0} },
        { EntryType_AppletType,           0, {AppletType_LibraryApplet, 0} },
        { EntryType_OverrideHeap,         M, {0, 0} },
        { EntryType_Argv,                 0, {0, 0} },
        { EntryType_NextLoadPath,         0, {0, 0} },     // <-- ¡AQUÍ!
        { EntryType_LastLoadResult,       0, {0, 0} },
        ...
    };
    ...
    // NextLoadPath
    entries[5].Value[0] = (u64)(uintptr_t)&g_nextNroPath[0];   // 512 bytes
    entries[5].Value[1] = (u64)(uintptr_t)&g_nextArgv[0];      // 2048 bytes
```
Y el bucle del loader (`loadNro()`):
```c
    if (g_nextNroPath[0] == '\0')
    {
        memcpy(g_nextNroPath, DEFAULT_NRO, sizeof(DEFAULT_NRO));   // "sdmc:/hbmenu.nro"
        memcpy(g_nextArgv,    DEFAULT_NRO, sizeof(DEFAULT_NRO));
    }

    memcpy(g_argv, g_nextArgv, sizeof g_argv);
    ...
    int fd = open(g_nextNroPath, O_RDONLY);
    ...
    // Reset NRO path to load hbmenu by default next time.
    g_nextNroPath[0] = '\0';
```
es decir: hbloader **carga un NRO dentro de su propio proceso** (`svcMapProcessCodeMemory`, `svcSetProcessMemoryPermission`, `nroEntrypointTrampoline`) y, al terminar ese NRO, **vuelve a `loadNro()`** para cargar el siguiente. **Ese es el bucle de encadenado.** `envSetNextLoad` desde dentro del NRO escribe `g_nextNroPath` (que vive en el espacio de direcciones de hbloader) y al salir hbloader lo carga.

**`g_isApplication` — la diferencia applet mode vs title override (código real):**
```c
static void getIsApplication(void)
{
    u64 flag=0;
    Result rc = svcGetInfo(&flag, InfoType_IsApplication, CUR_PROCESS_HANDLE, 0);  // [9.0.0+] o mesosphère
    if (R_SUCCEEDED(rc)) { g_isApplication = flag!=0; return; }

    // fallback: comparar nuestro PID con el application PID de pm:shell
    u64 cur_pid=0; svcGetProcessId(&cur_pid, CUR_PROCESS_HANDLE);
    rc = pmshellInitialize();
    if (R_SUCCEEDED(rc)) {
        u64 app_pid=0;
        rc = pmshellGetApplicationProcessIdForShell(&app_pid);
        pmshellExit();
        if (cur_pid == app_pid) g_isApplication = 1;
    }
}
```
y luego:
```c
    if (g_isApplication) {
        entry_AppletType->Value[0] = AppletType_SystemApplication;
        entry_AppletType->Value[1] = EnvAppletFlags_ApplicationOverride;
    }
```
→ **En title override (aplicación) los NROs reciben `AppletType_Application`; en applet mode (Álbum) reciben `AppletType_LibraryApplet`.**

**Memoria (código real de hbloader):**
```c
static u64 calculateMaxHeapSize(void)
{
    u64 size = 0, mem_available = 0, mem_used = 0;
    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used,      InfoType_UsedMemorySize,  CUR_PROCESS_HANDLE, 0);

    if (mem_available > mem_used+0x200000)
        size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
    if (size == 0) size = 0x2000000*16;
    if (size > 0x6000000 && g_isAutomaticGameplayRecording)
        size -= 0x6000000;      // graba gameplay automático -> menos RAM

    return size;
}

static void setupHbHeap(void)
{
    u64 size = calculateMaxHeapSize();
    if (!g_isApplication) {                       // <-- applet mode: heap aún más limitado
        if (g_appletHeapSize) {
            u64 requested_size = (g_appletHeapSize + 0x1FFFFF) & ~0x1FFFFF;
            if (requested_size < size) size = requested_size;
        }
        else if (g_appletHeapReservationSize) {
            u64 reserved_size = (g_appletHeapReservationSize + 0x1FFFFF) & ~0x1FFFFF;
            if (reserved_size < size) size -= reserved_size;
        }
    }
    Result rc = svcSetHeapSize(&addr, size);
    ...
}
```
Los límites `g_appletHeapSize` / `g_appletHeapReservationSize` se leen de la config del sistema en la sección **`hbloader`** (`readSetting("applet_heap_size", ...)` vía `setsys`): configurable por el usuario en Atmósfera.

**Cómo un Forwarder NSP pasa los argumentos: por ROMFS.** Sphaira (`sphaira/source/owo.cpp`) construye el forwarder NSP **a mano** (comentario: *"based on hacbrewpack (romfs creation) and yati (installation)"*) e inyecta el NRO destino y sus args como **ficheros en el romfs** del forwarder:
```cpp
// fix args to include nro path
if (config.args.empty()) {
    config.args = config.nro_path;
} else {
    config.args = config.nro_path + ' ' + config.args;
}
// create tid by using a hash over path + args
const auto hash_path = config.nro_path + config.args;
sha256CalculateHash(hash_data, hash_path.data(), hash_path.length());   // el titleid es un hash!
...
add_file_entry(romfs, "/nextArgv",     config.args.data(),     config.args.length());
add_file_entry(romfs, "/nextNroPath",  config.nro_path.data(), config.nro_path.length());
```
y el ExeFS del forwarder es **el hbl embebido**:
```cpp
add_file_entry(exefs, "main",       HBL_MAIN_DATA);    // hbl.nso  (embebido con --embed-dir)
add_file_entry(exefs, "main.npdm",  HBL_NPDM_DATA);    // hbl.npdm
...
add_file_entry(romfs, "/control.nacp", &config.nacp, sizeof(config.nacp));
```
**Por tanto:** el titleid de un forwarder Sphaira es un **hash SHA-256 de (ruta + args)**, no un ID aleatorio — así el mismo forwarder se puede recrear/borrar de forma determinista.

**Cadena real de arranque (resumen):**
```
[Home Menu / Álbum]
   -> Forwarder NSP (ExeFS main = hbl.nso)   ó   hbmenu.nro
      -> hbloader/hbl: getIsApplication(), setupHbHeap(), getOwnProcessHandle(),
                       getCodeMemoryCapability(), loadNro()
         -> lee romfs:/nextNroPath  (forwarder)   ó   default "sdmc:/hbmenu.nro"
         -> inyecta ConfigEntry[]: NextLoadPath(512)+Argv(2048)+AppletType+OverrideHeap(OBLIGATORIO)
            + ProcessHandle + RandomSeed + UserIdStorage + HosVersion + SyscallAvailableHint
         -> nroEntrypointTrampoline(&entries[0], -1, g_nroAddr)
            -> TU NRO corre; argc/argv poblados; envHasNextLoad() == true
               -> envSetNextLoad(path, argv)  -> escribe g_nextNroPath
               -> return desde main()
                  -> vuelve a hbloader loadNro() -> carga el siguiente NRO  (¡bucle!)
```
`OverrideHeap` va marcado como **obligatorio** (`#define M EntryFlag_IsMandatory`), y el ABI dice de él: *"If present, must not be ignored"*. Si tu NRO intenta `svcSetHeapSize` por su cuenta en ese entorno, romperás el loader. **No toques el heap**: libnx ya lo maneja (`envHasHeapOverride()`).

### 5.5 Pasar ARGUMENTOS/argv y leerlos

**Formato (verificado en código):** una **única cadena**, argumentos separados por espacios, cada uno que contenga espacios **entre comillas dobles**.

- hbmenu produce: `"arg1" "arg2" "arg3"` (comillas en **todos**, ver `init_args`).
- Sphaira produce: `arg1 arg2` sin comillas salvo que el arg contenga espacios.
- La ruta del NRO **debe ser el primer argumento** (`argv[0]`) para compatibilidad con hbloader.

**Parseo en el lado receptor.** libnx hace el split por ti en `nx/source/runtime/argv.c` (`argvSetup()`, llamado desde `__libnx_init` en `init.c`):
```c
    else if (envHasArgv()) {
        args = envGetArgv();
        argdata_allocsize = 0x9000;          // mismo tamaño que NSO
        argdata_strsize = strlen(args);
        argdata = (u8*)fake_heap_start;
        ...
    }
```
El parser **respeta comillas dobles** (`quote_flag`) y separa por `isspace()`:
```c
        if (argstart == NULL) {
            if (args[argi] == '"') { argstart = &args[argi+1]; quote_flag = 1; }
            else if (args[argi]!=0) { argstart = &args[argi]; arglen++; }
        }
        else {
            if (quote_flag) { if (args[argi] == '"') end_flag = 1; }
            else if (isspace((unsigned char)args[argi])) { end_flag = 1; }
            ...
            __system_argv[__system_argc] = argstorage;
            __system_argc++;
        }
```
Así que en `main` **simplemente usas `argc`/`argv` normal**:
```cpp
int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    // argv[0] = ruta del NRO (si el launcher la puso, como hace Sphaira/hbloader)
    // ...
    consoleInit(nullptr);
    while (appletMainLoop()) { consoleUpdate(nullptr); }
    consoleExit(nullptr);
}
```
Comprobación defensiva y alternativa explícita:
```cpp
    if (envHasArgv()) {
        const char* raw = (const char*)envGetArgv();   // string crudo sin parsear
        // ...
    }
```
Modo de salida (importante): `__nx_applet_exit_mode = 1;` hace que el applet salga del todo (hbmenu lo usa en su error screen: `__nx_applet_exit_mode = 1;` en `nx_main/main.c`).

### 5.6 ¿Se puede instalar un NSP (forwarder) desde homebrew?

**Sí.** Sphaira lo hace. Secuencia real (nombres de API exactos, de `sphaira/source/yati/yati.cpp` y `source/yati/nx/ncm.cpp`):

**a) Abrir storages y DB (SD y/o NAND):**
```cpp
    R_TRY(ncmOpenContentStorage(&ncm_cs[i], NCM_STORAGE_IDS[i]));
    cs = ncm_cs[config.sd_card_install];
    db = ncm_db[config.sd_card_install];
    R_TRY(parse_keys(keys, true));       // keys desde /switch/prod.keys (o similar)
```
**b) Instalar/cifrar cada NCA usando *placeholders* (así se evita reescribir):**
```cpp
    R_TRY(ncmContentStorageGeneratePlaceHolderId(&cs, &nca.placeholder_id));
    R_TRY(ncmContentStorageCreatePlaceHolder(&cs, &nca.content_id,
                                             &nca.placeholder_id, nca.size));
    // ... 3 hilos (read / decompress / write) ...
    R_TRY(ncmContentStorageWritePlaceHolder(&cs, &t->nca->placeholder_id,
                                            t->write_offset, buf.data() + off, wsize));
    // filtra ya instalado:
    R_TRY(ncmContentStorageHas(&cs, &nca.skipped, &nca.content_id));
```
**c) Registrar las NCAs y publicar el metadata + application record:**
```cpp
    R_TRY(ncm::Register(&cs, &cnmt.content_id, &cnmt.placeholder_id));   // cnmt.nca
    for (auto& nca : cnmt.ncas)
        R_TRY(ncm::Register(&cs, &nca.content_id, &nca.placeholder_id));

    // construir blob: meta_header + extended_header + content_info + infos
    buf.write(&cnmt.meta_header, sizeof(cnmt.meta_header));
    buf.write(cnmt.extended_header.data(), cnmt.extended_header.size());
    buf.write(&cnmt.content_info, sizeof(cnmt.content_info));
    for (auto& info : cnmt.infos) buf.write(&info.info, sizeof(info.info));

    R_TRY(ncmContentMetaDatabaseSet(&db, &cnmt.key, buf.buf.data(), buf.tell()));
    R_TRY(ncmContentMetaDatabaseCommit(&db));

    // push record -> hace que el título APAREZCA en el Home Menu
    ncm::ContentStorageRecord content_storage_record{};
    content_storage_record.key = cnmt.key;
    content_storage_record.storage_id = storage_id;
    R_TRY(ns::PushApplicationRecord(app_id, &content_storage_record, 1));

    if (hosversionAtLeast(6,0,0)) { R_TRY(avmInitialize()); ON_SCOPE_EXIT(avmExit()); }
```
**d) Rollback / limpieza correctos:**
```cpp
    ncmContentStorageDeletePlaceHolder(&yati->cs, &cnmt.placeholder_id);
    ncmContentStorageDeletePlaceHolder(&yati->cs, &nca.placeholder_id);
    // y para borrar un título:
    //   ncmContentStorageDelete(cs, &info.content_id) x N
    //   ncmContentMetaDatabaseRemove(db, &key); ncmContentMetaDatabaseCommit(db);
```
`ncm::Register` es idempotente vía `Delete()` + `ncmContentStorageRegister`:
```cpp
Result Register(NcmContentStorage* cs, const NcmContentId* content_id,
                const NcmPlaceHolderId* placeholder_id) {
    R_TRY(Delete(cs, content_id));                       // si existe, borrar
    return ncmContentStorageRegister(cs, content_id, placeholder_id);
}
```
También hay `es` (`source/yati/nx/es.cpp` — tickets, `esAddTicket`), `ns` (`source/yati/nx/ns.cpp`) y `lr` (para el truco de `core-launch`, `lrLrRedirectProgramPath`).

**¿Applet mode o title override?** **No puedo confirmarlo con evidencia dura.** Lo que sí puedo afirmar con la evidencia recogida:
- **libnx NO filtra servicios por applet type.** `nx/source/runtime/init.c` no consulta `appletGetAppletType` para decidir qué inicializar, y `nx/source/services/sm.c` sólo tiene overrides; no hay allowlist por applet type. El control de acceso a servicios en Horizon lo impone el **kernel vía el NPDM** del proceso, no el applet type.
- `hbl` (tanto el de Atmosphere como el de Sphaira) se ejecuta como **`AppletType_LibraryApplet`** por defecto (`EntryType_AppletType, 0, {AppletType_LibraryApplet, 0}`), y sólo con `g_isApplication` pasa a `SystemApplication`+`ApplicationOverride`.
- Lo que **sí** cambia drásticamente es **la memoria**: en applet mode el heap es menor (el bloque `if (!g_isApplication)` de `setupHbHeap`), y además el Álbum mantiene el Home Menu cargado. Una instalación NSP que hace streaming de NCAs de varios GB con 3 hilos + buffers necesita RAM y tiempo; en applet mode es mucho más frágil.
- Sphaira, además de instalar, **envía el propio Sphaira a sí mismo** para ciertas operaciones (`prepare_core_launch` escribe un `program.nca` en `sdmc:/config/sphaira/core-launch/` y usa `lrLrRedirectProgramPath`) — es decir, el proyecto asume que puede reiniciarse en un contexto distinto.

**Recomendación práctica:** implementa la instalación con **detección de entorno** y degrada con aviso claro (§6). Documenta al usuario que ejecutar el instalador en **title override / full memory** (Sphaira, o hbmenu vía forwarder con `hbloader` como aplicación) es el modo soportado.

**Requisitos y riesgos (honestos):**
- **Firmware/Atmósfera:** necesitas CFW (Atmósfera) y, para NSPs **cifrados**, las **keys** (`prod.keys`); `parse_keys(keys, true)` en Sphaira. Para tickets/`es` y las NCAs cifradas se usan `mbedtls` (`nxdumptool_rsa.c`, `cryptoAes128Xts`).
- **`ncm`/`es` deben estar autorizados en tu NPDM.** Si empaquetas QuiverNX con un NPDM que no incluye esos servicios, `ncmOpenContentStorage` fallará.
- **Riesgo de brick/corrupción:** escribir en la DB de `ncm` de forma incorrecta puede dejar títulos fantasma o corromper el estado. Sphaira mitiga con `Commit()` sólo tras `Set()` correcto, rollback de placeholders, y `Delete/Remove` ordenados. Copia ese patrón exacto.
- **Instalar un forwarder es el caso "fácil"**: su contenido no está cifrado con keys de Nintendo de la misma forma y Sphaira lo genera localmente (§5.4). Es mucho más seguro empezar por ahí.
- Sphaira también soporta **instalación por streaming** (HTTP/USB) sin guardar el NSP en la SD: `source/yati/source/http.cpp` + `source/usb/usb_installer.cpp`.

---

## 6. EMPAQUETADO Y COMPATIBILIDAD

### 6.1 Estructura recomendada en la SD

Compatible con hbmenu (que escanea `sdmc:/switch/`) y con Sphaira (que escanea `sdmc:/switch/` con fast-path `switch/<carpeta>/<carpeta>.nro`):

```
sdmc:/
└── switch/
    └── QuiverNX/
        ├── QuiverNX.nro              <- el NRO (o sdmc:/switch/QuiverNX.nro)
        ├── config.json               <- ajustes (escritura atómica)
        ├── db.json                   <- índice de apps instaladas
        ├── icon.jpg                  <- icono del NRO (JPEG, para el NACP)
        ├── cache/                    <- zips descargados / temporales
        ├── apps/                     <- homebrew gestionados por QuiverNX
        │   └── <nombre>/<nombre>.nro <- "fast path" que usa Sphaira
        └── fonts/
            └── Inter.ttf
```
**Por qué `apps/<nombre>/<nombre>.nro`:** Sphaira documenta y usa explícitamente ese fast-path (`nro_scan_internal` en `source/nro.cpp`):
```cpp
        if (e.type == FsDirEntryType_Dir) {
            fs::FsPath fullpath;
            std::snprintf(fullpath, sizeof(fullpath), "%s/%s/%s.nro", path.s, e.name, e.name);
            // fast path for detecting an nro in a folder
```
Sphaira **sólo escanea 1 nivel de anidamiento** (comentario literal: *"this function is recursive by 1 level deep. if the nro is in switch/folder/folder2/app.nro it will NOT be found"*). **No anides más de un nivel.**

### 6.2 NACP

Generado por `nacptool` (`$(OUTPUT).nacp : $(MAKEFILE_LIST)`):
```
nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ $(NACPFLAGS)
# NACPFLAGS += --titleid=$(APP_TITLEID)   (si defines APP_TITLEID)
```
Defaults de `switch_rules`: título = nombre del directorio, autor = `Unspecified Author`, versión = `1.0.0`.
El `.nacp` se incrusta en el NRO vía `elf2nro --nacp=...` en la sección `NroAssetHeader`.

**Sobre `titleid` en un NRO:** es **opcional y no debe colisionar** con un titleid real de NSP/forwarder. Si QuiverNX va a existir también como forwarder NSP, el titleid del forwarder lo determina lo que uses al crearlo (Sphaira: SHA-256 de ruta+args). Si defines `APP_TITLEID` en el Makefile, elige un rango no usado y **no lo dupliques entre homebrew**.

### 6.3 RomFS

Si defines `ROMFS := romfs`, `switch_rules` añade `--romfsdir=$(CURDIR)/romfs` a `NROFLAGS`, y se lee con `romfsInit()`:
```cpp
#include <switch.h>
romfsInit();
FILE* f = fopen("romfs:/defaults/config.json", "rb");
// ...
romfsExit();
```
Útil para: config por defecto, fuente TTF empaquetada, iconos, lista de repos preconfigurada.
> Sphaira **desactiva** romfs en su propio NRO por un motivo concreto (`USE_VFS_ROMFS=$<BOOL:FALSE>`) con el comentario *"disable romfs mounting as otherwise we cannot write / modify sphaira.nro"*. Si QuiverNX va a **auto-actualizarse escribiéndose su propio `.nro`**, ten en cuenta que montar romfs puede interferir: escribe el nuevo `.nro` a un temporal y renombra, y evita romfs si te encuentras ese problema.

### 6.4 Detectar applet mode para adaptar la UI

```cpp
#include <switch.h>
#include <cstdio>

enum class QuiverMode { FullMemory, AppletMode };

QuiverMode detect_mode() {
    const AppletType type = appletGetAppletType();
    // En title override: AppletType_Application (o SystemApplication antes del override).
    // En applet mode (Álbum):  AppletType_LibraryApplet.
    if (type == AppletType_Application || type == AppletType_SystemApplication)
        return QuiverMode::FullMemory;
    return QuiverMode::AppletMode;
}

// Alternativa/complemento: memoria disponible real
u64 available_heap() {
    u64 total = 0, used = 0;
    svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&used,  InfoType_UsedMemorySize,  CUR_PROCESS_HANDLE, 0);
    return total - used;
}

void adapt_ui() {
    const auto mode = detect_mode();
    if (mode == QuiverMode::AppletMode) {
        // - desactivar instalación de NSP / operaciones pesadas
        // - reducir caché de iconos y buffers
        // - avisar: "Ejecuta QuiverNX desde Sphaira o en modo aplicación (title override)
        //            para instalar NSP/forwarders."
        // - la descarga y el encadenado de NROs SÍ funcionan (hbl provee NextLoadPath)
    }
}
```
`AppletType` real (`applet.h`): `Default = -1`, `Application = 0`, `SystemApplet = 1`, `LibraryApplet = 2`, `OverlayApplet = 3`, `SystemApplication = 4`.
**Detalles a respetar:** `appletGetAppletType()` sólo es válido tras `appletInitialize()` (que libnx hace en `__appInit`). Y recuerda la semántica de hbloader: con `ApplicationOverride`, `SystemApplication` se reporta como **`Application`**.

### 6.5 Rutas, permisos y JSON seguro

- Raíz de la SD: **`sdmc:/`** (montada por `fsdevMountSdmc()`).
- Rutas largas: usa `FS_MAX_PATH` / `PATH_MAX`. Sphaira usa `fs::FsPath` (`PATH_MAX`) y valida antes de escribir.
- **No asumas que puedes escribir en cualquier sitio**: en `sdmc:/` sí; en `nand:/` y `save:/` necesitas los servicios correctos y son de riesgo. QuiverNX debe escribir **sólo** bajo `sdmc:/switch/QuiverNX/`.
- **Escanear sin explotar:** Sphaira lee sólo `sizeof(NroData)` (cabecera) primero y valida `NROHEADER_MAGIC` antes de leer más (`nro_parse_internal`), y limita iconos: `if (size > 1024 * 1024) return {};`. Haz lo mismo: los `.nro` en la SD pueden estar corruptos.
- **stdin/stdout**: `consoleInit(NULL)` te da consola de texto (útil para logs); desactívala antes de renderizar gráficos.
- **JSON seguro:** ver el patrón atómico de §3.8. Añade validación de tamaño máximo y **escribe siempre a temporal + rename**; nunca sobreescribas in-place (una pérdida de energía a mitad deja el config corrupto). Sphaira guarda además un `.sphaira.bak` y restaura si falla (`nro_update_info`).

---

## 7. REFERENCIAS DE CÓDIGO ÚTILES (20 entradas)

### Toolchain y build

1. **`switchbrew/switch-examples` → `templates/application/Makefile`**
   https://raw.githubusercontent.com/switchbrew/switch-examples/master/templates/application/Makefile
   *Por qué:* el Makefile NRO canónico. Mira la autodetección de `ICON`, `CONFIG_JSON`, `APP_JSON` y las reglas `$(OUTPUT).nro : $(OUTPUT).elf $(OUTPUT).nacp`. Es la base del §2.2.

2. **`switchbrew/libnx` → `nx/switch_rules`**
   https://raw.githubusercontent.com/switchbrew/libnx/master/nx/switch_rules
   *Por qué:* define **realmente** `%.nacp` (con `nacptool --create`), `%.nro` (`elf2nro $@ $(NROFLAGS)`), `%.nso`, `%.nsp`, y los defaults de `APP_TITLE/APP_AUTHOR/APP_VERSION/APP_ICON`. Fuente de verdad para el icono y el NACP.

3. **`switchbrew/switch-examples` → `network/curl/Makefile`**
   https://raw.githubusercontent.com/switchbrew/switch-examples/master/network/curl/Makefile
   *Por qué:* cómo integrar libcurl en un Makefile devkitPro: `CFLAGS += \`curl-config --cflags\`` y `LIBS := \`curl-config --libs\``. Copia esto tal cual.

4. **`devkitPro/pacman-packages` → `switch/curl/PKGBUILD` + `switch-curl.patch`**
   https://raw.githubusercontent.com/devkitPro/pacman-packages/master/switch/curl/PKGBUILD
   https://raw.githubusercontent.com/devkitPro/pacman-packages/master/switch/curl/switch-curl.patch
   *Por qué:* **prueba definitiva** de que TLS va por el backend `libnx` (`sslInitialize`, `sslCreateContext`, `sslConnectionDoHandshake`) y de que **no hay CA bundle**. También implementa la autodetección de proxy del sistema (`nifmGetCurrentNetworkProfile`). Léelo si vas a tocar TLS.

5. **`devkitPro/pacman-packages` → `switch/zlib/PKGBUILD`**
   https://raw.githubusercontent.com/devkitPro/pacman-packages/master/switch/zlib/PKGBUILD
   *Por qué:* demuestra que **`libminizip` viene dentro de `switch-zlib`** (con `-DUSE_FILE32API`). Te ahorra buscar un paquete inexistente.

### Descargar con curl / JSON

6. **`switchbrew/libnx` → `nx/include/switch/services/ssl.h`**
   https://raw.githubusercontent.com/switchbrew/libnx/master/nx/include/switch/services/ssl.h
   *Por qué:* la API completa de TLS/CA (`sslCreateContext`, `sslContextImportServerPki`, `sslConnectionDoHandshake`, `SslCaCertificateId_*`, `SslVerifyOption_*`). Aquí se ve que el CertStore es automático y que **no existe `setDefaultCertStore`**.

7. **`NaGaa95/sphaira` → `sphaira/source/download.cpp` y `source/utils/devoptab_http.cpp`**
   https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/download.cpp
   https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/utils/devoptab_http.cpp
   *Por qué:* descarga HTTP(S) real en Switch con progreso y cancelación. Mejor referencia que un ejemplo mínimo.

8. **`NaGaa95/sphaira` → `sphaira/source/yati/source/http.cpp`**
   https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/yati/source/http.cpp
   *Por qué:* descarga **por streaming** (sin guardar el fichero entero en RAM/SD) para instalar directamente. Patrón clave si quieres instalar sin ocupar el doble de espacio.

9. **`NaGaa95/sphaira` → `sphaira/source/ui/menus/ghdl.cpp`**
   https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/ui/menus/ghdl.cpp
   *Por qué:* **"GitHub Downloader"**: UI + lógica de consultar **releases de GitHub por API**, listar assets y descargarlos. Es casi exactamente el caso de uso de QuiverNX — la referencia más directa de todas.

10. **GitHub REST API — releases** (documentación)
    https://docs.github.com/en/rest/releases/releases#get-the-latest-release
    *Por qué:* campos exactos (`tag_name`, `assets[].name`, `assets[].browser_download_url`, `assets[].size`) y el requisito de `User-Agent`/`Accept: application/vnd.github+json`. Úsalo con el snippet yyjson del §3.2.

### Descomprimir

11. **`NaGaa95/sphaira` → `sphaira/source/minizip_helper.cpp` y `source/utils/devoptab_zip.cpp`**
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/minizip_helper.cpp
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/utils/devoptab_zip.cpp
    *Por qué:* uso real de `libminizip` en Switch, incluido un **devoptab** que monta el `.zip` como un filesystem (`zip:/`). Excelente si quieres navegar el zip en lugar de extraerlo entero.

### UI con focus de gamepad

12. **`NaGaa95/sphaira` → `sphaira/source/ui/widget.cpp` + `include/ui/widget.hpp` + `include/ui/object.hpp`**
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/ui/widget.cpp
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/include/ui/widget.hpp
    *Por qué:* el sistema de widgets: `SetActions`, `FireAction`, `Update(Controller*, TouchInfo*)`, `Draw(NVGcontext*, Theme*)`. **Esta es la arquitectura que quieres para una UI estilo Switch.**

13. **`NaGaa95/sphaira` → `sphaira/source/ui/list.cpp` + `include/ui/list.hpp`**
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/ui/list.cpp
    *Por qué:* lista con foco, scroll, scrollbar, páginas y callback de selección: `m_list->OnUpdate(controller, touch, m_index, rows, cb)`. Reutiliza el diseño.

14. **`NaGaa95/sphaira` → `sphaira/source/ui/nvg_util.cpp` y `source/ui/menus/forwarder_editor.cpp`**
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/ui/nvg_util.cpp
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/ui/forwarder_editor.cpp
    *Por qué:* el primero tiene los helpers de dibujo (`gfx::drawRect`, `gfx::drawText`, `DrawElement`, `drawRectOutline`) y el uso de `Theme`/`ThemeEntryID_*`. El segundo es un **formulario de creación de forwarder** completo — exactamente lo que QuiverNX necesita como UI.

15. **`devkitPro/deko3d`** — https://github.com/devkitPro/deko3d
    **Ejemplos deko3d**: `switchbrew/switch-examples` → `graphics/deko3d/deko_basic`, `deko_console`, `deko_examples`
    https://github.com/switchbrew/switch-examples/tree/master/graphics/deko3d
    *Por qué:* la alternativa de bajo nivel si eliges deko3d puro (swapchain, cmd buffers, `console`). Empieza por `deko_basic` para entender el mínimo viable.
    **Ejemplo SDL2**: `graphics/sdl2/sdl2-simple` (https://github.com/switchbrew/switch-examples/tree/master/graphics/sdl2) — usa `switch-sdl2` y `switch-mesa`.

### Lanzar NROs

16. **`switchbrew/nx-hbmenu` → `nx_main/loaders/builtin.c` y `common/launch.c`**
    https://raw.githubusercontent.com/switchbrew/nx-hbmenu/master/nx_main/loaders/builtin.c
    https://raw.githubusercontent.com/switchbrew/nx-hbmenu/master/common/launch.c
    *Por qué:* el **loader de referencia**: `envHasNextLoad()` en `init()`, `envSetNextLoad(path, argBuf)`, `uiExitLoop()`, y el parser/empaquetador de argv (`init_args`, `launchAddArgsFromString` con soporte de comillas simples/dobles). Copia este formato de argv.

17. **`switchbrew/nx-hbloader` → `source/main.c`** (y el `hbl/` de Sphaira)
    https://raw.githubusercontent.com/switchbrew/nx-hbloader/master/source/main.c
    https://github.com/NaGaa95/sphaira/tree/master/hbl
    *Por qué:* **la pieza que hace posible todo**: provee `EntryType_NextLoadPath` (`g_nextNroPath[512]`, `g_nextArgv[2048]`), `OverrideHeap` obligatorio, resuelve `g_isApplication` (title override vs applet), calcula el heap máximo, y hace el **bucle `loadNro()`** que recarga el siguiente NRO. Si algo de §5 no cuadra en pruebas, el problema está aquí.

### Instalar NSPs / crear forwarders

18. **`NaGaa95/sphaira` → `sphaira/source/yati/yati.cpp` + `source/yati/nx/ncm.cpp` + `source/yati/nx/es.cpp`**
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/yati/yati.cpp
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/yati/nx/ncm.cpp
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/yati/nx/es.cpp
    *Por qué:* la **secuencia completa de instalación NSP** con `ncmContentStorageCreatePlaceHolder` → `WritePlaceHolder` → `Register` → `ncmContentMetaDatabaseSet/Commit` → `ns::PushApplicationRecord`, más rollback y tickets con `es`. Es la referencia más completa que existe en abierto.

19. **`NaGaa95/sphaira` → `sphaira/source/owo.cpp`**
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/owo.cpp
    *Por qué:* **creación de Forwarder NSP desde cero**: construye PFS0/ExeFS y RomFS a mano (comentario: *"based on hacbrewpack (romfs creation) and yati (installation)"*), embebe el `hbl` como `main`/`main.npdm`, inyecta `/nextNroPath` + `/nextArgv` en romfs, parchea el `control.nacp` y deriva el titleid de un SHA-256 de (ruta+args). **El archivo más valioso del informe para tu objetivo.**

20. **`NaGaa95/sphaira` → `sphaira/source/nro.cpp`** y `source/ui/forwarder_editor.cpp`
    https://raw.githubusercontent.com/NaGaa95/sphaira/master/sphaira/source/nro.cpp
    *Por qué:* parseo de NRO (`NroStart`, `NroHeader`, `NROHEADER_MAGIC`), extracción de `NacpStruct` e icono desde `NroAssetHeader`, escaneo de `sdmc:/switch/`, y el lanzamiento con `envSetNextLoad` + `nro_add_arg` (prefijo `sdmc:` y comillas). Plantilla directa para el catálogo de homebrew de QuiverNX.

---

## Apéndice A: lo que NO pude verificar (declaración explícita)

Para que no construyas sobre arena:

1. **`setDefaultCertStore`** — no existe en libnx (verificado por grep exhaustivo). Si lo viste en algún sitio, era otra librería. **No verifiqué** si existe en alguna otra librería de Switch.
2. **`AppletJumpToApplication` / `envAppletJumpToApplication`** — no existen en libnx. No verifiqué exhaustivamente todo el ecosistema, sólo libnx (que es lo relevante).
3. **`Pluto` como librería de UI distinta de Plutonium** — no pude verificarlo. Todas las búsquedas devuelven `XorTroll/Plutonium`.
4. **Backend deko3d en borealis `main`** — el árbol inspeccionado muestra `library/lib/switch_wrapper.c` y deps (`fmt`, `glm`), pero **no confirmé** un backend deko3d listo. Verifícalo clonando el repo. Lo que sí es un hecho: **sin actividad desde 2023-10-31**.
5. **pthreads en devkitA64** — el `pthread.h` que vi es de newlib upstream (targets tipo RTEMS); **no confirmé** que la build `aarch64-none-elf` de devkitA64 enlace una implementación pthread. libnx no la expone. Usa la API de hilos de libnx (§3.7), que es lo que usan los proyectos reales.
6. **Nombres exactos de ficheros de cabecera de minizip** (`unzip.h` vs `minizip/unzip.h`) — depende de cómo lo instale el PKGBUILD; comprueba `ls $DEVKITPRO/portlibs/switch/include` en tu instalación.
7. **Restricción concreta de `ncm`/`es` en applet mode** — verificado que **libnx no filtra por applet type** (el control es del kernel vía NPDM), pero **no verifiqué** empíricamente si Horizon deniega `ncmOpenContentStorage` a un `LibraryApplet`. Lo que sí está probado en el código es que el **heap es menor** en applet mode. Trátalo como riesgo a medir en hardware.
8. **Comportamiento del Home Menu al volver desde un NRO lanzado por forwarder** (si vuelve al menú o relanza hbmenu) — no lo verifiqué; depende de `__nx_applet_exit_mode` y del `NroStart`/loader.
9. **Tamaño de binario con `switch-libarchive`** (para 7z) — no medido.
10. **Contenido exacto de `devkitpro/devkita64:latest`** (si incluye todos los portlibs) — la wiki afirma que *"come pre-installed with all the portlibs packages"*, pero no lo comprobé arrancando la imagen.

## Apéndice B: plan de implementación sugerido para QuiverNX

1. **Fase 0 — esqueleto:** template oficial + Makefile del §2.2 con `-lnx` y una consola. Verifica `make` y `nxlink -s`. Detecta `appletGetAppletType()` y muéstralo en pantalla.
2. **Fase 1 — HTTP + JSON:** integra `switch-curl` con el snippet del §3.1 (sin CA bundle) y yyjson del §3.2. Consulta `ghdl.cpp` (§7.9) y pinta en consola el `tag_name` + assets de un repo.
3. **Fase 2 — ZIP:** extrae a `sdmc:/switch/QuiverNX/apps/<nombre>/<nombre>.nro` con `libminizip` (§3.3). Respeta el **fast-path de 1 nivel** de Sphaira (§6.1).
4. **Fase 3 — Lanzar:** `envHasNextLoad()` + `envSetNextLoad(path, "\"<ruta>\"")` + salir del bucle (§5.1, §5.3). Prueba las **tres** vías: hbmenu, Sphaira, y forwarder NSP. Verifica `argv[0]`.
5. **Fase 4 — UI:** empieza con SDL2 + SDL2_image + SDL2_ttf para iterar rápido; aísla el renderer tras una interfaz propia para poder migrar a **deko3d + nanovg** (§4.5) copiando `widget.cpp`/`list.cpp` de Sphaira.
6. **Fase 5 — Forwarders:** genera el NSP del forwarder copiando `owo.cpp` (§7.19) — es el caso de instalación **más seguro** (sin keys de Nintendo, contenido generado localmente).
7. **Fase 6 — Instalación NSP/NCA:** sólo después, y detrás de un *gate* de entorno (title override) con aviso al usuario en applet mode. Copia la secuencia de `yati.cpp` con rollback de placeholders (§5.6).
