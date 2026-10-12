# Delanx — compila y ejecuta las tres suites de pruebas del PC.
#
#   bash tests/host/run.sh
#
# 1. datos   : contrato apps.json/settings.json, catalogo, assets, SHA-256
# 2. red     : parsers reales sobre fixtures de GitHub/GitLab/Quiver
# 3. instalador: instalacion real de paquetes de Switch (libarchive + SHA-256 + metadatos)
#
# Requisitos (MSYS2): gcc, mingw-w64-ucrt-x86_64-jansson, mingw-w64-ucrt-x86_64-libarchive
# En Windows usa el compilador de UCRT64 (el `msys/gcc` de algunas instalaciones esta roto):
#   pacman -S --needed mingw-w64-ucrt-x86_64-gcc
#   export PATH=/ucrt64/bin:$PATH
set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO" || exit 1

# En MSYS2 el compilador bueno es el de UCRT64; si existe, se pone delante del PATH.
case ":$PATH:" in
  *:/ucrt64/bin:*) ;;
  *) [ -d /ucrt64/bin ] && export PATH="/ucrt64/bin:$PATH" ;;
esac

CXX="${CXX:-g++}"
# Python: en MSYS2 no suele estar en el PATH, asi que se busca tambien una instalacion de
# Windows antes de rendirse (sin el no se generan las fixtures de prueba).
if [ -z "${PYTHON:-}" ]; then
  for candidate in python3 python py; do
    if command -v "$candidate" >/dev/null 2>&1; then PYTHON="$candidate"; break; fi
  done
fi
if [ -z "${PYTHON:-}" ] || ! command -v "$PYTHON" >/dev/null 2>&1; then
  for candidate in /c/Users/*/AppData/Local/Programs/Python/Python3*/python.exe \
                   /c/Python3*/python.exe; do
    if [ -x "$candidate" ]; then PYTHON="$candidate"; break; fi
  done
fi
PYTHON="${PYTHON:-python}"
UCRT_INC="${UCRT_INC:-/ucrt64/include}"
UCRT_LIB="${UCRT_LIB:-/ucrt64/lib}"
JANSSON="${JANSSON:-$UCRT_LIB/libjansson.a}"
ARCHIVE="${ARCHIVE:--larchive}"

if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "falta el compilador ($CXX). En MSYS2: pacman -S --needed mingw-w64-ucrt-x86_64-gcc"
  exit 2
fi

COMMON_SRC="source/core/strings.cpp source/core/json.cpp source/core/paths.cpp \
source/core/time.cpp source/core/sha256.cpp source/core/i18n.cpp \
source/model/app_entry.cpp source/model/catalog.cpp source/model/settings.cpp \
source/services/nro_inspector.cpp source/services/decomp_scanner.cpp"

mkdir -p build/host

build_and_run() {  # nombre-suite fichero-fuente [extras...]
  local name="$1"; shift
  local source_file="$1"; shift
  local out="build/host/delanx_${name}_tests.exe"
  echo "== compilando $name =="
  # shellcheck disable=SC2086
  "$CXX" -std=c++20 -O1 -Wall -Wextra -Wno-unused-parameter \
    -include tests/host/host_compat.h \
    -I source -I "$UCRT_INC" \
    -o "$out" "$source_file" $COMMON_SRC "$@" \
    -L "$UCRT_LIB" $JANSSON || return 2
  echo "== ejecutando $name =="
  "./$out"
}

# Generadores de fixtures: solo hacen falta si no estan ya (y requieren Python).
if command -v "$PYTHON" >/dev/null 2>&1; then
  "$PYTHON" tests/host/make_test_archives.py || true
  if [ ! -f tests/fixtures/assets/Starship-Barnard-Alfa-Switch.zip ]; then
    echo "== descargando assets de release reales (una sola vez) =="
    "$PYTHON" tests/host/fetch_fixtures.py || \
      echo "aviso: sin los assets reales, la suite del instalador fallara"
  fi
else
  echo "aviso: no se encontro Python ($PYTHON); se usan las fixtures ya presentes"
fi

status=0
build_and_run data tests/host/test_data_layer.cpp \
  source/services/asset_selector.cpp source/services/launch_args.cpp || status=1
build_and_run network tests/host/test_network_layer.cpp \
  source/services/github.cpp source/services/gitlab.cpp \
  source/services/quiver_catalog.cpp source/services/release_check.cpp \
  source/services/asset_selector.cpp || status=1
build_and_run installer tests/host/test_installer.cpp \
  source/services/github.cpp source/services/asset_selector.cpp \
  source/services/installer.cpp source/services/artwork.cpp \
  "$ARCHIVE" -lz -lzstd -lbz2 -llzma || status=1

echo
if [ $status -eq 0 ]; then
  echo "TODAS LAS SUITES OK"
else
  echo "ALGUNA SUITE HA FALLADO"
fi
exit $status
