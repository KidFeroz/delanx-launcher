# Verificación del catálogo Switch de Delanx

Fecha de verificación: **2026-10-11**
Ficheros generados: `romfs/catalog/switch-catalog.json` (46 entradas) y `romfs/catalog/harbourmasters.json` (5 entradas + 3 no disponibles).

---

## 1. Método de verificación

1. **API de GitHub** — `https://api.github.com/repos/<owner>/<repo>/releases?per_page=30` para los repos críticos (Harbour Masters, ports grandes). Sin token el límite es de 60 peticiones/hora y **se agotó** (quedaban 2 al final), por lo que el resto se hizo por HTML.
2. **Sin límite de peticiones** — feed `https://github.com/<repo>/releases.atom` para enumerar tags y `https://github.com/<repo>/releases/expanded_assets/<tag>` para obtener los **nombres exactos de los assets**. Para repos con muchos tags se paginó `https://github.com/<repo>/releases?page=N`.
3. **Verificación fuerte (la que decide)** — descarga del ZIP real y **listado de su contenido** con `System.IO.Compression.ZipFile` en PowerShell. Cuando en este informe se dice "verificado", significa que se vio el fichero `.nro` con su nombre y tamaño dentro del archivo descargado.
4. **Documentación de layouts SD** — `README.md` (raw.githubusercontent.com), `docs/installing.md` de DevilutionX, `readme.txt` *dentro* del zip Switch de SoH, y fichas de GameBrew para Golden Balloon NX.
5. **Límite conocido** — los assets `.7z` **no** se pudieron listar (no hay `7z` ni `py7zr` en el entorno y el header va comprimido). Afecta a `mGBA-*-switch.7z` y a `xash3d-fwgs-nswitch.7z`: en el catálogo, mGBA queda con `verified: false` y todos sus campos `switch.*` en `null`.

Convención de `switch` en el JSON: `targetDir` es la carpeta de la SD donde se deja el contenido del zip (`/` = raíz de la SD, usado cuando el zip ya trae `switch/...`), `nroPath` es la ruta **dentro del zip** hasta el `.nro`, y `stripRoot` indica que el zip envuelve todo en una carpeta raíz única que conviene eliminar al instalar.

---

## 2. Tabla de los 46 proyectos catalogados

| # | Nombre | Repositorio | Filtro de asset | .nro verificado | Ruta del .nro | Requisitos del usuario | Evidencia |
|---|---|---|---|---|---|---|---|
| 1 | Ship of Harkinian (OoT) | HarbourMasters/Shipwright | `SoH-*-Switch.zip` | Sí | `soh.nro` | ROM propia de OoT + `oot.otr`/`oot-mq.otr` generados en PC; CFW | Zip 8.0.6 listado: `soh.nro` 21.174.023 B. Releases 9.x sin Switch |
| 2 | Ship of Harkinian Switch Rev1 | HarbourMasters/Shipwright-Switch | `*-Switch-Rev1.zip` | Sí | `soh.nro` | ROM propia + `oot.otr`; CFW | Zip 8.0.6-1 listado; README documenta `sdcard/switch/soh/` |
| 3 | Starship (Star Fox 64) | HarbourMasters/Starship | `Starship-*-Switch.zip` | Sí | `Starship.nro` | ROM propia (US 1.0/1.1) + `sf64.o2r`; CFW | Zip v2.0.0 listado: `Starship.nro` 14.216.176 B + `starship.o2r` |
| 4 | Ghostship (Super Mario 64) | HarbourMasters/Ghostship | `*-Switch.zip` | Sí | `Ghostship.nro` | ROM propia (US/JP) + `sm64.o2r`; CFW | Zip 2.0.0 listado: `Ghostship.nro` + `ghostship.o2r` (846 entradas) |
| 5 | SpaghettiKart (Mario Kart 64) | HarbourMasters/SpaghettiKart | `*Switch.zip` | Sí | `Spaghettify-Switch/Spaghettify.nro` | ROM propia (US) + `mk64.o2r`; CFW | Prerelease 0.9.9.1 listado: `Spaghettify.nro` + `spaghetti.o2r` |
| 6 | Sphaira | NaGaa95/sphaira | `sphaira.zip` | Sí | `switch/sphaira/sphaira.nro` | CFW + hbmenu | Zip 1.0.8 listado (una sola entrada) |
| 7 | nx-hbmenu | switchbrew/nx-hbmenu | `nx-hbmenu_v*.zip` | Sí | `hbmenu.nro` | CFW; `.nro` en la raíz de la SD | Zip v3.6.1 listado: `hbmenu.nro` 2.455.539 B |
| 8 | Homebrew App Store | fortheusers/hb-appstore | `appstore.nro` | Sí | `appstore.nro` | CFW + red | Asset `.nro` directo; `switch-extracttosd.zip` listado → `switch/appstore/appstore.nro` |
| 9 | Goldleaf | XorTroll/Goldleaf | `Goldleaf.nro` | Sí | `Goldleaf.nro` | CFW; Quark/USB para NSP | Asset `.nro` directo; README: `sd:/switch/Goldleaf/settings.json` |
| 10 | NX-Shell | joel16/NX-Shell | `NX-Shell.nro` | Sí | `NX-Shell.nro` | CFW | Asset `.nro` directo (4.01) |
| 11 | JKSV | J-D-K/JKSV | `JKSV.nro` | Sí | `JKSV.nro` | CFW | Asset `.nro` directo; tags con `/` → URL-encoding |
| 12 | Awoo Installer (NSAInstaller) | Team-Neptune/Awoo-Installer | `NSAInstaller.zip` | Sí | `switch/NSAInstaller/NSAInstaller.nro` | CFW; red o USB | Zip v1.0.1 listado: `NSAInstaller.nro` 11.070.399 B |
| 13 | nxdumptool | DarkMatterCore/nxdumptool | `nxdumptool.nro` | Sí | `nxdumptool.nro` | CFW | Asset `.nro` directo (v1.1.15) |
| 14 | Checkpoint | FlagBrew/Checkpoint | `Checkpoint.nro` | Sí | `Checkpoint.nro` | CFW | Asset `.nro` directo (v5.2.0) |
| 15 | EdiZon | WerWolv/EdiZon | `EdiZon.nro` | Sí | `EdiZon.nro` | CFW; overlay opcional | Asset `.nro` directo + `ovlEdiZon.ovl` |
| 16 | NXThemes Installer | exelix11/SwitchThemeInjector | `NXThemesInstaller.nro` | Sí | `NXThemesInstaller.nro` | CFW | Asset `.nro` directo (tag `nxt-3.0.2`) |
| 17 | ftpd | mtheall/ftpd | `ftpd.nro` | Sí | `ftpd.nro` | CFW; misma red | Asset `.nro` directo (v3.2.1) |
| 18 | sys-clk | retronx-team/sys-clk | `sys-clk-*.zip` | Sí | `switch/sys-clk-manager.nro` | CFW (boot2) + Tesla-Menu | Zip 2.0.1 listado: `sys-clk-manager.nro` + `sys-clk-overlay.ovl` |
| 19 | SysDVR | exelix11/SysDVR | `SysDVR.zip` | Sí | `switch/SysDVR-conf.nro` | CFW + cliente PC/Android | Zip v6.3 listado: `SysDVR-conf.nro` 7.725.760 B |
| 20 | Moonlight-Switch | XITRIX/Moonlight-Switch | `Moonlight-Switch.nro` | Sí | `Moonlight-Switch.nro` | CFW + PC con Sunshine/GFE | Asset `.nro` directo; README: `sdcard:/switch/Moonlight-Switch` |
| 21 | SwitchWave | averne/SwitchWave | `SwitchWave-*.zip` | Sí | `switch/SwitchWave/SwitchWave.nro` | CFW | Zip 1.1.2 listado; README: extraer en la raíz de la SD |
| 22 | nxmp | proconsule/nxmp | `nxmp-*.zip` | Sí | `nxmp/nxmp.nro` | CFW | Zip 0.9.3 listado; README: copiar carpeta `nxmp` a `/switch/nxmp` |
| 23 | Chiaki (PS Remote Play) | hikigayairoha/chiaki-switch | `chiaki.nro` | Sí | `chiaki.nro` | CFW + PS4/PS5 en red | Asset `.nro` directo; README: carpeta `SD Card/switch/` |
| 24 | pNES | Cpasjuste/pemu | `pnes.nro` | Sí | `pnes.nro` | CFW + ROMs NES | Asset `.nro` directo (v7.2) |
| 25 | pSNES | Cpasjuste/pemu | `psnes.nro` | Sí | `psnes.nro` | CFW + ROMs SNES | Asset `.nro` directo (v7.2) |
| 26 | pGBA | Cpasjuste/pemu | `pgba.nro` | Sí | `pgba.nro` | CFW + ROMs GBA | Asset `.nro` directo (v7.2) |
| 27 | pGEN | Cpasjuste/pemu | `pgen.nro` | Sí | `pgen.nro` | CFW + ROMs Mega Drive | Asset `.nro` directo (v7.2) |
| 28 | pFBNeo | Cpasjuste/pemu | `pfbneo.nro` | Sí | `pfbneo.nro` | CFW + ROMsets FBNeo | Asset `.nro` directo (v7.2) |
| 29 | PPSSPP Switch Community Build | SirSamael/ppsspp-switch-community-build | `PPSSPP-Switch*.zip` | Sí | `switch/ppsspp/PPSSPP.nro` | CFW + ISOs PSP | Zip v0.6.5 (40,6 MB) listado: `PPSSPP.nro` 25.997.096 B |
| 30 | mGBA | mgba-emu/mgba | `mGBA-*-switch.7z` | **No (`.7z` no listable)** | `null` | CFW + ROMs GBA | Solo nombre de asset (`mGBA-0.10.5-switch.7z`); sin extractor 7z |
| 31 | OpenBOR | Cpasjuste/openbor | `OpenBOR-*_switch.zip` | Sí | `OpenBOR/OpenBOR.nro` | CFW + PAKs | Zip v3.0.7087-2 listado: `OpenBOR.nro` 9.458.561 B |
| 32 | NXEngine-Evo (Cave Story) | nxengine/nxengine-evo | `NXEngine-Evo-*-Switch.nro` | Sí | `NXEngine-Evo-v2.6.5-Switch.nro` | CFW + datos de Cave Story | Asset `.nro` directo (v2.6.5); v2.6.5-1 ya no trae Switch |
| 33 | DevilutionX (Diablo) | diasurgical/devilutionX | `devilutionx-switch.nro` | Sí | `devilutionx-switch.nro` | CFW + `DIABDAT.MPQ` | `docs/installing.md`: `/switch/devilutionx` |
| 34 | GZDoom | fgsfdsfgs/gzdoom | `gzdoom_nx_*.zip` | Sí | `gzdoom/gzdoom.nro` | CFW + WADs | Zip `nx4.11pre-2` listado: `gzdoom.nro` 19.431.859 B |
| 35 | NXQuake | fgsfdsfgs/nxquake | `nxquake_*.zip` | Sí | `switch/nxquake/nxquake.nro` | CFW + paks de Quake | Zip v0.2.0 listado: `nxquake.nro` 9.471.235 B + `id1/` |
| 36 | QuakeSpasmNX | fgsfdsfgs/QuakespasmNX | `quakespasmnx_v*.zip` | Sí | `switch/quakespasm/quakespasmnx.nro` | CFW + paks de Quake | Zip v040 listado: `quakespasmnx.nro` 8.207.887 B + `id1/` |
| 37 | nxquake2 | fgsfdsfgs/nxquake2 | `nxquake2_*.zip` | Sí | `switch/nxquake2/nxquake2.nro` | CFW + paks de Quake II | Zip r6 listado: `nxquake2.nro` 8.833.366 B + `baseq2/`, `rogue/`, `xatrix/` |
| 38 | ioquake3nx | fgsfdsfgs/ioq3 | `ioquake3nx_v*.zip` | Sí | `switch/ioquake3/ioquake3nx.nro` | CFW + pk3 de Quake III | Zip v030 listado: `ioquake3nx.nro` 10.395.517 B + `baseq3/vm/` |
| 39 | Xash3D-FWGS (Half-Life) | fgsfdsfgs/xash3d-fwgs | `xash3d-nswitch-*.zip` | Sí | `xash3d/xash3d.nro` | CFW + datos de Half-Life | Zip v0.20-nx2 (37 entradas) listado: `xash3d.nro` 9.128.587 B + `valve/` |
| 40 | Perfect Dark | fgsfdsfgs/perfect_dark | `pd-arm64-nswitch.zip` | Sí | `pd-arm64-nswitch/perfectdark/pd.arm64.nro` | CFW + ROM propia | Zip CI listado: 3 variantes `.nro` + `data/put_your_rom_here.txt` |
| 41 | Arx Libertatis | fgsfdsfgs/ArxLibertatis | `arx_v1.2-nx*.zip` | Sí | `arx.nro` | CFW + datos de Arx Fatalis | Zip nx1 listado: `arx.nro` 12.072.479 B + `data/` |
| 42 | OpenMW (Morrowind) | fgsfdsfgs/openmw | `openmw_nx_v*.zip` | Sí | `openmw.nro` | CFW + datos de Morrowind | Zip nx4 (120 entradas) listado: `openmw.nro` 35.763.617 B |
| 43 | Unreal | fgsfdsfgs/unreal_nx | `unreal_nx_r*.zip` | Sí | `unreal.nro` | CFW + datos de Unreal | Zip r2 listado: `unreal.nro` 14.238.819 B + `SystemARM/` |
| 44 | Serious Sam TFE | fgsfdsfgs/Serious-Engine | `ssam_tfe_nx1.zip` | Sí | `ssam_tfe/ssam_tfe.nro` | CFW + datos del juego | Zip nx1 listado: `ssam_tfe.nro` 11.709.544 B + `SE1_minimum.gro` |
| 45 | Serious Sam TSE | fgsfdsfgs/Serious-Engine | `ssam_tse_nx1.zip` | Sí | `ssam_tse/ssam_tse.nro` | CFW + datos del juego | Zip nx1 listado: `ssam_tse.nro` 12.320.711 B |
| 46 | Golden Balloon NX (DKR) | boraeskicioglu/goldenballoon_nx | `goldenballoon.nro` | Sí | `goldenballoon.nro` | CFW + ROM DKR US 1.1 | Asset `.nro` directo; layout SD documentado en la ficha de GameBrew |

Resumen: **46 entradas**, de las cuales **45 tienen el `.nro` verificado** listando el asset descargado y **1 (mGBA) queda marcada `verified: false`** por ser un `.7z` no listable en este entorno.

---

## 3. Harbour Masters: qué versiones traen Switch y cuáles lo perdieron

La organización tiene 35 repos; solo los ports de juego publican assets, y **Switch aparece y desaparece según la versión**:

| Proyecto | Última versión con asset Switch | Versiones posteriores sin Switch |
|---|---|---|
| Shipwright (OoT) | **8.0.6** (2024-08-13) `SoH-MacReady-Golf-Switch.zip` | 9.0.0 → 9.3.0 (2026-10-06) solo Mac/Win64/AppImage |
| Shipwright-Switch (repo solo-Switch) | **8.0.6-1** (2025-11-29) `SoH-MacReady-Golf-Switch-Rev1.zip` | — (canal oficial Switch vigente) |
| Starship (SF64) | **v2.0.0** (2025-05-25) `Starship-Barnard-Alfa-Switch.zip` | v1.0.0 anterior no tenía Switch |
| Ghostship (SM64) | **2.0.0** (2026-05-29) `Mary-Celeste-Alfa-Switch.zip` | 3.0.0 (2026-09-07) ya no lo incluye |
| SpaghettiKart (MK64) | **0.9.9.1** prerelease (2025-11-15) `Spaghettify-Alfredo-Alfa-1-Switch.zip` | 1.0.0 estable (2026-02-25) no lo incluye |

**Lo importante:** aunque el asset desaparezca de los releases estables, los README siguen publicando **builds Switch de CI** vía `nightly.link`, que son la vía recomendada para versiones nuevas:

- SoH: `https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-switch.zip`
- Starship: `https://nightly.link/HarbourMasters/Starship/workflows/main/main/Starship-switch.zip`
- SpaghettiKart: `https://nightly.link/HarbourMasters/SpaghettiKart/workflows/main/main/Spaghettify-switch.zip`
- Ghostship: el README mantiene sección Nintendo Switch (nightly), aunque el asset del release 3.0.0 desapareció

**Requisito común de todos los ports HM:** no incluyen assets con copyright. El usuario debe aportar su **ROM** (verificable en `https://ship.equipment/`) y, en el caso de Switch, **generar el `.otr`/`.o2r` ejecutando antes un release de PC**. El `readme.txt` incluido en el zip Switch de SoH lo dice literalmente y documenta el árbol de la SD:

```
sdcard
└── switch
    └── soh
        ├── oot-mq.otr
        ├── oot.otr
        ├── soh.nro
        └── soh.otr
```

**Sobre la web `harbourmasters.org/downloads`:** es una SPA (el fetch directo solo devuelve el `<title>`). Su código fuente está en `HarbourMasters/harbourmasters-hub`: `src/data/games.ts` declara **7 ports** (`ORG_STATS.totalPorts = 7`: Ship of Harkinian, 2ship2Harkinian, Ghostship, Spaghetti Kart, Starship, Lighthouse, Paperboat) y existe `public/icons/platforms/nintendo-switch.svg` junto a `wii-u.svg`, es decir **Switch sí es una plataforma contemplada**; pero la lista de descargas se trae en vivo de la API de GitHub, así que la web **no garantiza** que cada juego tenga asset Switch.

### Harbour Masters sin Switch (`unavailable` en `harbourmasters.json`)

| Proyecto | Motivo | Workaround |
|---|---|---|
| 2ship2harkinian (Majora's Mask) | 16 tags escaneados, cero assets Switch; README sin mención a Switch | `null` (no se localizó nightly Switch) |
| Lighthouse (Banjo-Kazooie) | 5 tags, solo Linux/Mac/Win64; README sin Switch | `null` |
| PaperBoat (Paper Mario 64) | 4 tags, solo Linux/Mac/Win64; README sin Switch | `null` |

---

## 4. No catalogables / no verificados (y por qué)

**Sin ningún asset Switch (comprobado tag a tag):** `HarbourMasters/2ship2harkinian`, `HarbourMasters/Lighthouse`, `HarbourMasters/PaperBoat`, `Zelda64Recomp/Zelda64Recomp` (solo Win/Linux/macOS), `BanjoRecomp/BanjoRecomp` (solo Win/Linux/macOS/Flatpak), `N64Recomp/N64Recomp` (solo `RecompModTool`), `coop-deluxe/sm64coopdx`, `STJr/SRB2`, `opentyrian/opentyrian`.

**Sin releases (habría que compilar con devkitPro):** `fgsfdsfgs/sm64ex` y `fgsfdsfgs/sm64-port` (puertos Switch de sm64ex), `fgsfdsfgs/dsda-doom`, `fgsfdsfgs/OpenJKDF2`, `fgsfdsfgs/alephone`, `fgsfdsfgs/silent-hill-decomp`, `fgsfdsfgs/cs16-client`, `m4xw/ppsspp`.

**Assets de otra plataforma (fáciles de confundir):** `fgsfdsfgs/chocolate-doom` y `fgsfdsfgs/prboom-plus` (solo PSVita `.vpk`), `spacemeowx2/switch-lan-play` (solo clientes de PC), `HarbourMasters/Shipwright-WiiU` y `2ship2harkinian-WiiU` (Wii U, no Switch).

**Repo bloqueado o inexistente:** `shchmue/Lockpick_RCM` → GitHub responde **HTTP 451** (no verificable); `proferabg/EdiZon-SE` → 404; `proconsule/SwitchWave` → 404 (el correcto es `averne/SwitchWave`).

**Verificados pero SIN `.nro` en el release** (son overlays, sysmodules o payloads: no se lanzan desde el Homebrew Menu, por eso no entran en el catálogo):
`XorTroll/emuiibo` (zip con `SdOut/switch/.overlays/emuiibo.ovl` + sysmodule; sin `.nro`), `masagrator/Status-Monitor-Overlay` (`.ovl`), `WerWolv/Tesla-Menu` (`ovlmenu.zip` → `.ovl`), `ppkantorski/Fizeau` (`Fizeau.ovl`), `HookedBehemoth/sys-tune` (sysmodule + `.ovl`), `suchmememanyskill/TegraExplorer` (`TegraExplorer.bin`, payload).

**Descartados por no ser homebrew instalable:** AM2R Switch (no se localizó repo ni release en GitHub; solo hilos de GBAtemp, no verificable), Portal 64 (sin release público), SonicMania y VVVVVV (comerciales, sin port), yuzu (emulador *de* Switch, no corre *en* Switch), DBI y Tinfoil (sin repositorio en GitHub), Citro2D/Citro3D (librerías), RetroArch (los releases de GitHub son solo `retroarch-sourceonly-*.tar.xz`; los builds Switch se distribuyen desde el buildbot de libretro, no desde releases de GitHub).

**Advertencias para el launcher:**
- HM cambia de assets entre versiones: conviene fijar tag/versión en `releaseAssetFilter` (SoH → 8.0.6 o el repo Shipwright-Switch; Ghostship → 2.0.0; SpaghettiKart → 0.9.9.1) o resolver los `nightly.link`.
- `JKSV` publica tags con `/`, que deben ir URL-encoded al construir la URL de descarga.
- `mGBA`, y el tag `continuous-switch_newer` de Xash3D, solo ofrecen `.7z`: el instalador necesita soporte 7z o debe usar la release estable de Xash3D (`v0.20-nx2`, que sí es ZIP).
- `targetDir` marcado como deducido (no documentado por el autor): Starship, Ghostship, SpaghettiKart, Arx Libertatis, OpenMW, Unreal, NXEngine-Evo y las tools cuyo README no fija subcarpeta. Todos los casos quedan explicados en el campo `notes` de cada entrada.
