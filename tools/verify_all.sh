#!/usr/bin/env bash
# Delanx — verifica TODO el proyecto con una sola orden.
#
#   D:\devkitPro\msys2\msys64\usr\bin\bash.exe -lc "bash /e/Vibecoding/QuiverNX/tools/verify_all.sh"
#
# Hace, en orden:
#   1. build limpio del NRO (y cuenta los warnings de nuestro codigo),
#   2. verificacion de la estructura de Delanx.nro,
#   3. build del autotest de dispositivo y verificacion de su NRO,
#   4. las tres suites de pruebas del PC (datos, red, instalador),
#   5. el paquete para la SD.
# Al final imprime una tabla con PASS/FAIL y devuelve 0 solo si todo pasa.
set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO" || exit 1

export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
case ":$PATH:" in
  *:/ucrt64/bin:*) ;;
  *) [ -d /ucrt64/bin ] && export PATH="/ucrt64/bin:$PATH" ;;
esac

PYTHON="${PYTHON:-}"
if [ -z "$PYTHON" ]; then
  for candidate in python3 python py; do
    if command -v "$candidate" >/dev/null 2>&1; then PYTHON="$candidate"; break; fi
  done
fi
# En MSYS2 no suele haber Python en el PATH: se busca una instalacion de Windows.
if [ -z "$PYTHON" ] || ! command -v "$PYTHON" >/dev/null 2>&1; then
  for candidate in /c/Users/*/AppData/Local/Programs/Python/Python3*/python.exe \
                   /c/Python3*/python.exe; do
    if [ -x "$candidate" ]; then PYTHON="$candidate"; break; fi
  done
fi
PYTHON="${PYTHON:-python}"
LOG_DIR="${TMPDIR:-/tmp}"
JOBS="$(nproc 2>/dev/null || echo 4)"

declare -a NAMES=() RESULTS=() DETAILS=()
FAILED=0

record() {  # nombre, resultado(0/1), detalle
  NAMES+=("$1")
  if [ "$2" -eq 0 ]; then
    RESULTS+=("PASS")
  else
    RESULTS+=("FAIL")
    FAILED=1
  fi
  DETAILS+=("$3")
}

hr() { printf '%s\n' "------------------------------------------------------------"; }

# ---------------------------------------------------------------- 1. build del NRO
hr; echo "1/5  build limpio del NRO"
make clean >"$LOG_DIR/delanx-clean.log" 2>&1
make -j"$JOBS" >"$LOG_DIR/delanx-build.log" 2>&1
build_status=$?
# grep -c devuelve 1 cuando la cuenta es 0, asi que no se encadena con ||.
errors=$(grep -c 'error:' "$LOG_DIR/delanx-build.log" 2>/dev/null)
warnings=$(grep 'warning:' "$LOG_DIR/delanx-build.log" 2>/dev/null | grep -c 'QuiverNX/source')
errors=${errors:-0}
warnings=${warnings:-0}
size=$(stat -c '%s' Delanx.nro 2>/dev/null || echo 0)
record "build Delanx.nro" "$build_status" "${size} B, ${errors} errores, ${warnings} warnings nuestros"

# ---------------------------------------------------------------- 2. NRO principal
hr; echo "2/5  estructura de Delanx.nro"
"$PYTHON" tools/verify_nro.py Delanx.nro >"$LOG_DIR/delanx-verify.log" 2>&1
verify_status=$?
record "verify_nro Delanx.nro" "$verify_status" "$(tail -2 "$LOG_DIR/delanx-verify.log" | head -1)"

# ---------------------------------------------------------------- 3. autotest
hr; echo "3/5  autotest de dispositivo"
make -C tests/device -j"$JOBS" >"$LOG_DIR/delanx-selftest.log" 2>&1
selftest_build=$?
if [ $selftest_build -eq 0 ]; then
  "$PYTHON" tools/verify_nro.py tests/device/DelanxSelfTest.nro >"$LOG_DIR/delanx-verify-selftest.log" 2>&1
  selftest_status=$?
  record "build+verify autotest" "$selftest_status" \
    "$(tail -2 "$LOG_DIR/delanx-verify-selftest.log" | head -1)"
else
  record "build+verify autotest" 1 "fallo de compilacion (ver $LOG_DIR/delanx-selftest.log)"
fi

# ---------------------------------------------------------------- 4. suites del PC
hr; echo "4/5  suites de pruebas del PC"
if PYTHON="$PYTHON" bash tests/host/run.sh >"$LOG_DIR/delanx-host-tests.log" 2>&1; then
  host_status=0
else
  host_status=1
fi
summary=$(grep -E '^Comprobaciones:' "$LOG_DIR/delanx-host-tests.log" | tr '\n' ' ')
record "suites del PC" "$host_status" "${summary:-sin salida}"

# ---------------------------------------------------------------- 5. paquete
hr; echo "5/5  paquete para la SD"
make package >"$LOG_DIR/delanx-package.log" 2>&1
package_status=$?
package_name=$(ls -1 Delanx-*-sdcard.zip 2>/dev/null | tail -1)
record "make package" "$package_status" "${package_name:-no se genero}"

# ---------------------------------------------------------------- resumen
echo
hr
printf '%-26s %-6s %s\n' "PRUEBA" "RESULT" "DETALLE"
hr
for index in "${!NAMES[@]}"; do
  printf '%-26s %-6s %s\n' "${NAMES[$index]}" "${RESULTS[$index]}" "${DETAILS[$index]}"
done
hr
if [ $FAILED -eq 0 ]; then
  echo "TODO OK  (logs en $LOG_DIR/delanx-*.log)"
else
  echo "HAY FALLOS  (logs en $LOG_DIR/delanx-*.log)"
fi
exit $FAILED
