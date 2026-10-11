# Autotest de dispositivo

Este NRO comprueba **en la Switch real** todo lo que no se puede verificar desde el PC: el
toolchain (vectores SHA-256), el parser de fechas, la extracción con libarchive, el lanzador de
NROs y la capa de plataforma (modo applet, batería, firmware, origen de arranque).

```bash
export DEVKITPRO=/opt/devkitpro
make -C tests/device            # -> tests/device/DelanxSelfTest.nro
nxlink -s tests/device/DelanxSelfTest.nro
```

`nxlink -s` envía el NRO y deja la salida por red en la terminal. Al terminar, el test
escribe el informe completo en:

```
sdmc:/switch/Delanx/instcheck-report.txt
```

Además crea y borra ficheros de prueba dentro de `sdmc:/switch/Delanx/Temp/selfcheck/`.

## Qué se comprueba

| Bloque | Comprobaciones |
|---|---|
| SHA-256 | vectores conocidos (`""`, `"abc"`, 55/56/64 bytes, 1 MB de `a`) y `sha256_file` |
| Fechas | `parse_iso8601_utc` (Z, fracciones, `+HH:MM`, `+HHMM`), `now_unix`, `clock_hhmm_local` |
| Extracción | zip/tar generados, `.7z`, rechazo de *path traversal*, `strip_root` |
| Instalación | `find_launchable_nro` (raíz y un nivel), `write_install_metadata` (`version.txt`, `installed-build.txt`) |
| Ficheros del usuario | que `.otr`/ROMs sobrevivan a una reinstalación |
| Plataforma | `appletGetAppletType`, batería, firmware, red, `boot_origin`, `heap_usage_human` |
| Lanzador | `can_chain_nro()` (dice si el loader expone los búferes de `envSetNextLoad`) |

Cualquier `[FAIL]` indica un problema real en la consola; el informe incluye el detalle de cada
comprobación para poder corregirlo.
