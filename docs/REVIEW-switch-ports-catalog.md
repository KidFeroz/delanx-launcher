# Revisión del `switch_ports_catalog.md` (Catálogo Maestro de 110 títulos)

**Revisor:** auditoría técnica para el proyecto **Delanx** (launcher homebrew de Switch, NRO + devkitPro/libnx + SDL2)
**Fecha de la revisión:** 2026-10-11
**Documento revisado:** [`switch_ports_catalog.md`](../switch_ports_catalog.md) — 2.566 líneas, 110 filas en la tabla maestra §4 y 110 objetos en el `ports.json` de §5 (verificado: `TABLE_ROWS 110`, `JSON_OBJECTS 110`).
**Referencias usadas:** [`romfs/catalog/switch-catalog.json`](../romfs/catalog/switch-catalog.json) (46 entradas verificadas), [`docs/catalog-verification.md`](catalog-verification.md), [`docs/ARCHITECTURE.md`](ARCHITECTURE.md), [`tools/import_ports_md.py`](../tools/import_ports_md.py).

> **Alcance y honestidad metodológica.** No he usado `api.github.com` en ninguna comprobación. Todo se ha hecho con `github.com/<repo>`, `github.com/<repo>/releases`, `releases.atom` y `releases/expanded_assets/<tag>` (los mismos métodos que el informe de auditoría), más `raw.githubusercontent.com`. Donde una comprobación no llegó a buen término lo digo explícitamente como **no verificable**. Este documento es el **único fichero que he creado**; durante la auditoría generé dos temporales de trabajo (`_tmp_thumbs.tsv`, `_tmp_libretro.txt`) en la raíz del repo, que **borro al terminar** (ver §8).

---

## 1. Veredicto corto antes de los detalles

El documento es **un muy buen trabajo de investigación de campo** —la mejor parte con diferencia es la de arte— pero **como catálogo de instalación no es fiable**: mezcla 110 títulos bajo la etiqueta "compatibles con Nintendo Switch" sin distinguir lo verificado de lo supuesto, y el contraste con la realidad deja **al menos 16 repositorios inexistentes (HTTP 404)**, **13 de 47 URLs de arte rotas (28 %)** y **una API de arte (SteamGridDB) que exige token y que el documento presenta como abierta**. La parte de datos de instalación (`install_dir` + `executable`) es la más floja: aproximadamente un tercio de las entradas que he podido contrastar tienen el nombre del `.nro` inventado por convención.

---

## 2. Estructura y virtudes: qué aporta que Delanx no tenga ya

### 2.1 Esquema de datos: comparación campo a campo

El `ports.json` del maestro es **uniforme** (comprobado: los 110 objetos tienen exactamente las mismas 17 claves, sin `null`, sin campos ausentes). Frente a las 46 entradas de `switch-catalog.json`:

| Campo del maestro | ¿Lo tiene Delanx? | Valor real que aporta | Valoración |
|---|---|---|---|
| `id` estable (`oot_ship_of_harkinian`, `nxquake2`…) | **No** (Delanx usa `folderName` y `name`) | Clave estable e independiente de la carpeta final; ideal como clave de caché de arte y de miniaturas | **Adoptar.** Es la mejor idea del documento |
| `category` ("Plataformas 3D", "Acción / Aventura"…) | No (`tags` genéricos) | Categoría legible para agrupar en la UI | **Adoptar** como `tags` o campo nuevo |
| `asset_type` (`rom_extract`, `file_copy`, `pc_folder_copy`, `archive_extract_otr`…) | No | Describe *cómo* se prepara la instalación: extraer ROM, copiar ficheros, copiar carpeta de PC | **Adoptar como pista**, no como contrato: no está verificado contra ningún asset |
| `required_files` (p. ej. `["baserom.us.z64"]`, `["DUKE3D.GRP"]`) | Parcial: Delanx tiene `switch.requiredUserFiles` con **rutas absolutas** | El maestro aporta el **nombre lógico** del fichero que el usuario debe poner | **Fusionar**: usar el nombre lógico y resolverlo a `sdmc:<homeDir>/<fichero>` (que es exactamente lo que ya hace `import_ports_md.py`) |
| `rom_source_hint` ("Super Mario 64 (USA/JP) .z64") | No | Aviso claro y honesto de que hace falta ROM propia | **Adoptar**, es información de usuario excelente |
| `thumbnail_provider` + `thumbnail_url` + `cache_path` + `romfs_fallback` | Solo `appIconUrl` | La **estrategia de arte completa** (proveedor, URL, ruta de caché, respaldo) | **Adoptar** (§3), con las correcciones de §3.2 |
| `install_dir` | Equivalente a `switch.targetDir` (pero **semántica distinta**, ver §5) | — | **No adoptar tal cual** |
| `github_repo` | `repository` | — | Ya existe |
| `notes` | `notes` | Descripciones comerciales plausibles | Útil como texto de ficha |

**Conclusión de §2.1:** el maestro y Delanx son **complementarios, no alternativos**. El maestro aporta la capa de presentación (id, categoría, arte, aviso de ROM); Delanx aporta la capa de verdad (repositorio real, filtro de asset, `nroPath` dentro del zip, `stripRoot`, `verified`, `switchProof`). Lo que **falta por completo** en el maestro es precisamente lo que hace instalable una entrada: `releaseAssetFilter`, `nroPath` (ruta *dentro* del zip) y `stripRoot`. Sin esos tres campos, las 110 entradas del maestro no se pueden instalar de forma fiable, solo mostrar.

### 2.2 La estructura de SD de §1: correcta en concepto, imprecisa en detalle

El árbol de §1 (`roms/` centralizado, `switch/<app>/`, `QuiverNX/cache/`) es una convención sensata y coincide con la práctica real de la escena (los `.nro` van bajo `/switch/<app>/`, con excepciones documentadas como hbmenu en la raíz). Dos matices:

- **`roms/` centralizado no lo respetan todos los ports.** Varios de los verificados esperan los datos **junto al `.nro`**: SoH (`/switch/soh/oot.otr`), Starship (`/switch/Starship/sf64.o2r`), Ghostship (`sm64.o2r`), nxquake2 (`baseq2/`), Xash3D (`valve/`). Centralizar ROMs exige *symlinks* o copias; el documento no aborda ese conflicto.
- El propio §1 se contradice con §4 en varias entradas: §1 pone `shipofharkinian/` y `Ghostship/`, mientras que Delanx tiene verificado `/switch/soh/` (documentado en el `readme.txt` **dentro** del zip Switch de SoH y en el README de `Shipwright-Switch`).

---

## 3. La estrategia de miniaturas: lo mejor del documento, con dos fallos gordos

### 3.1 Libretro Thumbnails — la parte buena (y su letra pequeña)

**Lo que dice el maestro:** "Enlaces directos a `raw.githubusercontent.com`, sin API key, sin límites de peticiones y con nombres de carátulas estandarizados de No-Intro".

**Lo que he comprobado, URL a URL (HEAD sobre las 47 entradas de Libretro):**

| Resultado | Nº | Detalle |
|---|---|---|
| HTTP 200 `image/png` | **34 / 47** | Descarga directa real, sin `Authorization`, sin clave. Confirmado con `Content-Type: image/png` y tamaños de 200 KB–785 KB |
| HTTP 404 | **13 / 47** | Nombre de fichero o colección inexistente |

**Por tanto: la afirmación "sin API key" es CIERTA** (verificado con peticiones HEAD sin cabecera de autenticación: 200 en 34 casos). **La afirmación "sin límites de peticiones" es una simplificación**: `raw.githubusercontent.com` es un CDN y no aplica el límite de 60 peticiones/hora de la API REST, pero GitHub se reserva limitar el tráfico abusivo; lo honesto es decir "no hay cuota de API documentada" y **cachear en la SD**, que es justo lo que el documento propone más abajo. No he provocado ninguna limitación con ~150 peticiones seguidas, así que en la práctica es válido.

**El fallo gordo es el 28 % de URLs rotas.** Las 13 que devuelven 404 y lo que he comprobado de cada una:

| Entrada | URL del maestro | Resultado | Qué he verificado |
|---|---|---|---|
| `goldeneye_007_nx` | `Nintendo_-_Nintendo_64/.../007%20-%20GoldenEye%20%28USA%29.png` | **404** | El nombre correcto existe: `GoldenEye%20007%20%28USA%29.png` → **200** (255.512 B). También `GoldenEye%20007%20%28Europe%29.png` → 200 |
| `wipeout_rewrite` | `Sony_-_PlayStation/.../WipEout%20%28USA%29.png` | **404 (mayúsculas)** | `Wipeout%20%28USA%29.png` (minúscula) → **200** (516.040 B) |
| `pokemon_gen2_recomp` | `Nintendo_-_Game_Boy_Color/.../Pokemon%20-%20Crystal%20Version%20%28USA%2C%20Europe%29.png` | **404** | `Pokemon%20-%20Crystal%20Version%20%28USA%29.png` → **200** (427.867 B) |
| `jak_and_daxter_opengoal` | `Sony_-_PlayStation_2/.../Jak%20and%20Daxter%20-%20The%20Precursor%20Legacy%20%28USA%29.png` | **404** | Probadas 2 variantes más, también 404. La colección/fichero no es ese: **no localizado** |
| `pokemon_gen1_recomp` | `Nintendo_-_Game_Boy/.../Pokemon%20-%20Red%20Version%20%28USA%2C%20Europe%29.png` | **404** | Variante sin región (`%28USA%29`) también 404. **No localizado** |
| `zelda_oracles_nx` | `Nintendo_-_Game_Boy_Color/.../Oracle%20of%20Seasons%20%28USA%2C%20Europe%29.png` | **404** | Sin región también 404. **No localizado** |
| `zelda_links_awakening_dx_hd` | `Nintendo_-_Game_Boy_Color/.../Link%27s%20Awakening%20DX%20%28USA%2C%20Europe%29.png` | **404** | Sin región también 404. **No localizado** |
| `daytona_usa_nx` | `FBNeo_-_Arcade_Games/Named_Titles/daytona.png` | **404** | Probado también `Named_Boxarts/daytona.png` → 404. **Colección inexistente con ese nombre** (ver más abajo) |
| `virtua_fighter_2_nx` | `FBNeo_-_Arcade_Games/Named_Titles/vf2.png` | **404** | idem |
| `virtual_on_nx` | `FBNeo_-_Arcade_Games/Named_Titles/von.png` | **404** | idem |
| `killer_instinct_nx` | `FBNeo_-_Arcade_Games/Named_Titles/kinst.png` | **404** | idem |
| `sonic_the_fighters_nx` | `FBNeo_-_Arcade_Games/Named_Titles/sfight.png` | **404** | idem |
| `sonic_cd_rsdk` | `Sega_-_CD/.../Sonic%20CD%20%28USA%29.png` | **404** | Probadas `%28USA%2C%20Europe%29` y `%28Europe%29` → 404. **No localizado** |

**Sobre las 5 de arcade:** el `.gitmodules` oficial de `libretro-thumbnails/libretro-thumbnails` (124 colecciones, consultado en `raw.githubusercontent.com/.../master/.gitmodules`) **no lista ninguna colección llamada `FBNeo_-_Arcade_Games`**; aparece `FBNeo` y `MAME`. Es decir, las 5 entradas de arcade del maestro apuntan a un **repositorio de colección que con ese nombre no existe** (el 404 del PNG es coherente con un 404 del repo entero). Dos de ellas (`kinst`, `sfight`) además usan el nombre de ROM de MAME en lugar del nombre del juego.

**Consecuencia práctica:** 13 de 110 miniaturas (12 % del catálogo total) fallan **siempre**, no solo en modo avión, y el flujo de decisión de §2 no tiene rama para "el proveedor devolvió 404" — solo para "hay red / no hay red". Un fallo de descarga debe caer al `romfs:/icons/<id>.png`, y el documento no lo especifica.

### 3.2 SteamGridDB — aquí el documento se equivoca de forma verificable

**Lo que dice el maestro:** "mediante su API abierta gratuita", con endpoints del tipo `https://www.steamgriddb.com/api/v2/search/autocomplete/<juego>` usados como **URL de miniatura** en las 63 entradas del proveedor.

**Lo que he comprobado:**

1. `GET https://www.steamgriddb.com/api/v2/search/autocomplete/Quake%20II` → **HTTP 401** con cuerpo `{"success":false,"errors":["Authentication Required"]}`.
2. `GET https://www.steamgriddb.com/api/v2` → HTTP 200 pero el cuerpo es literalmente el título `SteamGridDB API v2` (portada de documentación, sin contenido útil por fetch directo).
3. `GET https://www.steamgriddb.com/api/v2/docs` → **HTTP 404**.

**Conclusión:** la API v2 de SteamGridDB **requiere token** (`Authorization: Bearer <api-key>`), que se solicita por formulario; no es "abierta" en el sentido de anónima. Y hay un segundo error, más grave para el diseño: **esas 63 URLs no son miniaturas**. `api/v2/search/autocomplete/...` es un endpoint de **búsqueda que devuelve JSON** (id, nombre, plataforma), no un PNG. Para obtener arte hay que encadenar mínimo dos llamadas más (`/grids/game/<id>`, `/heroes/game/<id>`, `/logos/game/<id>`) y luego descargar la URL del CDN que devuelva. El `cache_path` del maestro (`sdmc:/switch/QuiverNX/cache/<id>.png`) sugiere que se espera un PNG directo: **con SteamGridDB eso nunca ocurrirá sin token y sin dos peticiones previas**.

**Implicación para Delanx:** de las 110 entradas, **63 (57 %) tienen una fuente de arte que hoy no funciona tal como está escrita**. La capa de arte actual de Delanx (`Cache/artwork/<folderName>.png` a partir de `appIconUrl`) **no debería** adoptar las URLs de SteamGridDB del maestro. Si en el futuro se quiere SteamGridDB, hay que (a) registrar clave, (b) guardarla en `settings.json`, (c) implementar el flujo de dos pasos y (d) asumir que la clave del usuario viaja en la SD. Coste alto, beneficio solo para títulos de PC.

### 3.3 Fallback RomFS `romfs:/icons/<id>.png` — buena idea, adoptar

El respaldo empaquetado en el propio `.nro` es **la mejor decisión de diseño del documento** y encaja exactamente con un launcher de Switch:

- Funciona **sin red** (modo avión, el caso normal en consola portátil) y **sin depender de terceros** (ni GitHub ni SteamGridDB).
- El coste de ~2,5 MB para 110 iconos es realista **si se reescalan** a la resolución de la rejilla (los PNG de Libretro que he descargado pesan 200–785 KB cada uno: 110 × 0,5 MB ≈ 55 MB **sin reescalar**, inviable dentro del NRO de 11,3 MB actual de Delanx; a 256×256 y PNG optimizado, 110 iconos ≈ 2–3 MB, que es la cifra del documento).
- Requiere una decisión que el documento no toma: **la clave debe ser el `id` estable**, no el `folderName`, porque el `folderName` de Delanx cambia (p. ej. `DiddyKongRacing-GoldenBalloonNX`) y los `id` del maestro no.

**Recomendación:** adoptar el fallback, generar los iconos desde las **34 URLs de Libretro que responden 200** (más las 3 con nombre corregido = 37), y dejar el resto con un icono genérico por categoría. No empaquetar arte descargado de SteamGridDB sin resolver antes la licencia.

---

## 4. Errores de datos comprobables

### 4.1 Discrepancias de `executable` / `install_dir` contra lo verificado

La afirmación del documento ("110 títulos compatibles con Switch") no distingue verificado de supuesto. Tabla de discrepancias **con evidencia**; "realidad verificada" = lo que hay en `docs/catalog-verification.md` + `switch-catalog.json` (`.nro` visto listando el asset descargado) o lo que he comprobado yo en `releases/expanded_assets`:

| Entrada (id) | Dice el maestro | Realidad verificada | Evidencia |
|---|---|---|---|
| `oot_ship_of_harkinian` | `/switch/shipofharkinian/` + `shipofharkinian.nro` | `/switch/soh/` + `soh.nro` | `readme.txt` **dentro** del zip Switch de SoH y README de `HarbourMasters/Shipwright-Switch` documentan `sdcard/switch/soh/soh.nro`; catálogo Delanx |
| `mm_2ship2harkinian` | `/switch/2ship2harkinian/` + `2ship2harkinian.nro` | **No existe build de Switch** | `docs/catalog-verification.md` §3: 16 tags escaneados, cero assets Switch; repo `HarbourMasters/2ship2harkinian` sí existe (200) pero sin asset Switch |
| `paper_mario_paperboat` | `/switch/PaperBoat/` + `PaperBoat.nro` | **No existe build de Switch** | `docs/catalog-verification.md` §3: 4 tags, solo Linux/Mac/Win64 |
| `banjo_kazooie_lighthouse` | `/switch/Lighthouse/` + `Lighthouse.nro` | **No existe build de Switch** | `docs/catalog-verification.md` §3: 5 tags, solo Linux/Mac/Win64 |
| `dkr_golden_balloon` | `/switch/goldenballoon_nx/` + `goldenballoon_nx.nro` | `/switch/goldenballoon/` + `goldenballoon.nro` | Informe §2 fila 46: asset `.nro` directo `goldenballoon.nro`; ficha GameBrew. El `--check` lo marca como conflicto con Delanx |
| `starfox_64_nx` | repo `Zelda64Recomp/StarFox64Recomp` + `/switch/sf64/` + `sf64.nro` | repo **404**; el port real es `HarbourMasters/Starship` con `Starship.nro` | `HEAD https://github.com/Zelda64Recomp/StarFox64Recomp` → **404**; informe §2 fila 3: `Starship.nro` 14.216.176 B |
| `mario_kart_64_nx` | repo `n64decomp/mk64` + `/switch/mk64/` + `mk64.nro` | repositorio existe (200) pero **sin asset Switch en releases**; el build Switch real es `Spaghettify-Switch/Spaghettify.nro` | Página de releases de `n64decomp/mk64` sin ningún `.nro`; informe §2 fila 5: prerelease 0.9.9.1 con `Spaghettify.nro` |
| `smash_64_recomp` | repo `N64Recomp/Smash64Recomp` + `/switch/smash64recomp/` + `smash64recomp.nro` | repo **404**; el recomp real conocido es `zestydevy/smash64r` (**PC Windows/Linux**) | `HEAD` → 404; `zestydevy/smash64r` existe (200) y su página de releases no publica ningún `.nro` |
| `bomberman_64_recomp` | repo `N64Recomp/Bomberman64Recomp` | repo **404** | `HEAD` → 404 (la org `N64Recomp` existe con otro contenido) |
| `quest_64_recomp` | repo `N64Recomp/Quest64Recomp` | repo **404** | `HEAD` → 404 |
| `pokemon_stadium_ss_anne` | repo `HarbourMasters/SSAnne` | repo **404** | `HEAD` → 404 |
| `goldeneye_007_nx` | repo `n64decomp/ge007` | repo **404** | `HEAD` → 404 |
| `castlevania_lod_nx` | repo `fliperama86/CastlevaniaLegacyOfDarkness` | repo **404** | `HEAD` → 404 |
| `doom_64_ex_nx` | repo `fgsfdsfgs/doom64ex` + `doom64ex.nro` | repo existe pero **no figura en la auditoría** (`catalog-verification.md` §4 lista `fgsfdsfgs/dsda-doom`, `OpenJKDF2`, `alephone`… como "sin releases") | No verificable en esta revisión (no lo he comprobado tag a tag) |
| `super_mario_galaxy_astrocore` | repo `SMGCommunity/AstroCore` + `/switch/AstroCore/` + `AstroCore.nro` | repo **404** en `SMGCommunity/AstroCore`; el proyecto de descompilación de SMG existe (prensa: [Polygon](https://www.polygon.com/super-mario-galaxy-nintendo-fan-port-pc/)) pero **no he localizado repo con build Switch** | `HEAD` → 404; búsquedas web sin repo Switch verificable |
| `zelda_twilight_princess_dusklight` | repo `souldbminerr/dusklight-nx` + `/switch/dusklight/` + `dusklight.nro` | **existe** y **sí publica `.nro` de Switch**: `dusklight.nro` | `releases/expanded_assets/v2.0.0-nx2` → `dusklight.nro`; README: "Nintendo Switch port". Ojo: el upstream `TwilitRealm/dusklight` (tags v2.0.3) publica **solo macOS/Windows**, el port de Switch vive en el fork `souldbminerr` |
| `smash_melee_nx` | `/switch/Melee-NX/` + `Melee-NX.nro` | repo **existe** y **sí publica `.nro`**; el nombre real es `melee.nro` (tags `0.1.0-Beta`, `0.2.0-beta`) y `melee-nx.nro` (tag `0.2.5-beta`) | `releases/expanded_assets` de los 3 tags |
| `silent_hill_1_nx` | `/switch/silenthill/` + `silenthill.nro` | repo **existe** y **sí publica `.nro`**; el nombre real es `SilentHillPC.nro` | `expanded_assets/1.1.0` y `/v1.0.0` → `SilentHillPC.nro` |
| `jak_and_daxter_opengoal` | repo `fildicio/jak-project-switch` + `/switch/opengoal/jak1/` + `jak1.nro` | repo **existe** y publica `.nro`; el README dice que la carpeta final es **`sdmc:/switch/jak1/`**, no `/switch/opengoal/jak1/`, y el binario se empaqueta como `gk.nro`→`jak1.nro` | README (raw) `main`: "Copy the generated files from build-switch/sd-card to your Switch SD card (into `sdmc:/switch/jak1/`)" |
| `nfs_most_wanted_2005` | repo `StevensND/nfsmw_nx` + `nfsmw.nro` | repo **404**; el real es `StevensND/nfsmw-nx`, con 10 assets regionales (`nfsmw-nx-usa.nro`, `nfsmw-nx-pal-en.nro`, …) | `HEAD` → 404 en `nfsmw_nx`; `expanded_assets/v1.0.0` de `nfsmw-nx` lista los 10 `.nro` |
| `simpsons_hit_and_run` | repo `q4a/simpsons` + `simpsons.nro` | repo **existe**; el README **remite a otro repo** para los releases (`ZenoArrows/The-Simpsons-Hit-and-Run`, assets `Simpsons.zip`, `SimpsonsNTSC.zip`, `SimpsonsPAL.zip`); la carpeta `/switch/simpsons` **sí** está documentada | README de `q4a/simpsons` (`master`): "copy the contents … to `sdmc:/switch/simpsons`" + enlace al release ajeno |
| `sonic_1_rsdk` | `/switch/sonic1/` + `sonic1.nro` | **Correcto en el maestro** (tabla y objeto coinciden) | Falso positivo del importador: Delanx tiene un error propio (`targetDir /switch/sonic2` + `nro sonic2.nro`). Ver §6.3 |
| `duke_nukem_3d_eduke32` | `/switch/eduke32/` + `eduke32.nro` | **Correcto en el maestro** (tabla y objeto coinciden) | Falso positivo del importador: el error (`fury` en `/switch/eduke32`) está en la entrada de Delanx. Ver §6.3 |
| `ion_fury_nx` | `/switch/fury/` + `fury.nro` (mismo repo `fgsfdsfgs/eduke32`) | **No verificable**: no he podido listar el asset de `fgsfdsfgs/eduke32` | No comprobado en esta revisión |
| `cave_story_nxengine` | `/switch/nxengine/` + `nxengine.nro` | `/switch/` + `NXEngine-Evo-v2.6.5-Switch.nro` | Informe §2 fila 32: el asset del release **es** el `.nro` con ese nombre; `--check` lo marca |
| `zelda_wind_waker_switchwaker` | `/switch/SwitchWaker/` + `SwitchWaker.nro` | repo **existe** y **sí hay `.nro`**; el nombre real es **minúscula**: `switchwaker.nro` | Página de releases de `centollOS/SwitchWaker` → `switchwaker.nro` |
| `zelda_minish_cap_tmc` | `/switch/minishcap/` + `minishcap.nro` | repo **existe** y **sí hay `.nro`**; los nombres reales son `tmc_switch.nro`, `TMC-Switch-USA-v0.2.5.nro`, `TMC-Switch-BR-PTBR-v0.2.5.nro` | Página de releases de `HayatoG/tmc` |
| `sonic_advance_2_nx` | `/switch/sonicadvance2/` + `sonicadvance2.nro` | repo **existe** y **sí hay `.nro`**; nombres reales versionados: `sonic_advance2_nx_v0.1.nro` … `v0.3.nro` | Página de releases de `boraeskicioglu/sonic_advance2_nx` |
| `daytona_usa_nx` | `/switch/daytona/` + `daytona.nro` | repo **existe** y **sí hay `.nro`**: `daytona.nro` | Página de releases de `Thorhax/Daytona-NX-Modern` (nota: el repo es `Thorhax`, no `thorhax`; GitHub es insensible a mayúsculas) |
| `sega_rally_port0r` | `/switch/segarally/` + `segarally.nro` | repo **existe** y **sí hay `.nro`**: `segarally.nro` | Página de releases de `jacquesdupontd/port0r` |
| `super_mario_strikers_nx` | `/switch/strikers/` + `strikers.nro` | repo **existe** y **sí hay `.nro`**: `strikers.nro` | Página de releases de `new-coke/strikers` |
| `zelda_oracles_nx` | `/switch/oracles/` + `oracles.nro` | repo **existe** y **sí hay `.nro`**: `oracles.nro` | Página de releases de `toniisound/oracles-nx` |
| `jackie_chan_rechan` | `/switch/rechan/` + `rechan.nro` | **No verificable**: la página de releases de `CostelaCNX/ReChan-Switch` no muestra `.nro` | No comprobado tag a tag |
| `nfs`… (resto de 63 entradas de PC) | URLs de SteamGridDB | **Fuente de arte no operativa** | Ver §3.2 |

**Recuento de esta tabla:** 16 discrepancias de repositorio inexistente (404) + 5 nombres de `.nro` demostrablemente distintos (`goldenballoon`, `melee-nx`, `SilentHillPC`, `switchwaker`, `tmc_switch`/`TMC-Switch-USA-v0.2.5`, `sonic_advance2_nx_v0.3`) + 3 discrepancias de directorio final (`soh`, `jak1`, `goldenballoon`) + 3 entradas de Harbour Masters sin build + 1 fuente de arte rota en 63 entradas.

### 4.2 Repositorios del maestro que **no existen** (HTTP 404 verificado)

Esta es la lista de comprobaciones limpias, con `HEAD https://github.com/<repo>` sin redirecciones raras:

```
404  N64Recomp/Smash64Recomp          N64Recomp/Bomberman64Recomp      N64Recomp/Quest64Recomp
404  Zelda64Recomp/StarFox64Recomp    SMGCommunity/AstroCore           StevensND/nfsmw_nx
404  HarbourMasters/SSAnne            carstene1ns/switch-srb2          switch-ports/ringracers-nx
404  AM2R-Community-Developers/AM2R-Autopatcher   tx00100xt/Serious-Engine-Switch
404  Mupen64Plus/celeste64-switch     AquariaOS/Aquaria                Twin-E/Twin-E
404  zwaetschge/ACGC-NX               mariob0y/reVC-nx                 n64decomp/ge007
404  fliperama86/CastlevaniaLegacyOfDarkness
200  N64Recomp/N64Recomp   Zelda64Recomp/Zelda64Recomp   StevensND/nfsmw-nx   TwilitRealm/dusklight
200  zestydevy/smash64r   ZenoArrows/The-Simpsons-Hit-and-Run   fgsfdsfgs/Serious-Engine
```

Es decir: **al menos 19 repositorios citados en el maestro no existen** (dos de ellos, `tx00100xt/Serious-Engine-Switch` y `StevensND/nfsmw_nx`, existen con **otro nombre**: `fgsfdsfgs/Serious-Engine` y `StevensND/nfsmw-nx`). Las organizaciones `N64Recomp` y `Zelda64Recomp` sí existen (200) pero no contienen los repos que el documento les atribuye. **Una entrada cuyo repositorio da 404 no es instalable bajo ninguna circunstancia**: debería llevar `verified: false` y, en rigor, no debería estar en un catálogo de lanzamiento.

### 4.3 Títulos sin build de Switch (contraste con la auditoría + comprobación propia)

El documento afirma (§portada y §3) que los 110 son "compatibles con Nintendo Switch (formato ejecutable `.nro`)". Clasificación de los casos que puedo sostener:

| Título (id del maestro) | Clasificación | Motivo y evidencia |
|---|---|---|
| `mm_2ship2harkinian` | **Confirmado sin Switch** | `catalog-verification.md` §3: 16 tags escaneados, cero assets Switch; README sin mención a Switch |
| `banjo_kazooie_lighthouse` | **Confirmado sin Switch** | `catalog-verification.md` §3: 5 tags, solo Linux/Mac/Win64 |
| `paper_mario_paperboat` | **Confirmado sin Switch** | `catalog-verification.md` §3: 4 tags, solo Linux/Mac/Win64 |
| `smash_64_recomp` | **Confirmado sin Switch** (y repo del maestro inexistente) | `N64Recomp/Smash64Recomp` → 404. El recomp real es `zestydevy/smash64r`, descrito como "Static recompilation of Super Smash Bros. **for PC (Windows/Linux)**" ([repo](https://github.com/zestydevy/smash64r)); su página de releases no publica `.nro` |
| `melee_nx` (`smash_melee_nx`) | **Confirmado con Switch** (nombre de ejecutable erróneo) | `KawaiiBunga/Melee-NX` con 3 tags y assets `melee.nro` / `melee-nx.nro` en `releases/expanded_assets` |
| `dusklight` (`zelda_twilight_princess_dusklight`) | **Confirmado con Switch** (repo correcto pero distinto del upstream) | `souldbminerr/dusklight-nx` → `dusklight.nro` en `v2.0.0-nx2`; README "Nintendo Switch port". El upstream `TwilitRealm/dusklight` (hasta v2.0.3) **no** publica Switch |
| `astrocore` (`super_mario_galaxy_astrocore`) | **Confirmado sin Switch / repo inexistente** | `SMGCommunity/AstroCore` → 404. No he localizado ningún repo público con build `.nro` de la descompilación de SMG (el proyecto existe según prensa, sin release Switch) |
| `silent_hill_1_nx` | **Confirmado con Switch** (nombre de ejecutable erróneo) | `ChaykaDed/silent-hill-decomp-nx`, assets `SilentHillPC.nro` en `v1.0.0` y `1.1.0` |
| `opengoal` (`jak_and_daxter_opengoal`) | **Confirmado con Switch** (carpeta final errónea) | `fildicio/jak-project-switch` publica `.nro`; README fija `sdmc:/switch/jak1/` |
| `nfsmw` (`nfs_most_wanted_2005`) | **Confirmado con Switch** (repo y nombre erróneos) | `StevensND/nfsmw-nx` v1.0.0 con 10 `.nro` regionales; `nfsmw_nx` → 404 |
| `simpsons` (`simpsons_hit_and_run`) | **Confirmado con Switch** (releases en otro repo) | `q4a/simpsons` (README) remite a `ZenoArrows/The-Simpsons-Hit-and-Run` (assets `SimpsonsNTSC.zip`/`SimpsonsPAL.zip`); `/switch/simpsons` documentado |
| `goldeneye_007_nx` | **No verificable** | El repo del maestro da 404; no he podido determinar el repo real del port de GoldenEye en esta revisión |
| `doom_64_ex_nx`, `fzero_x_nx_diffuser`, `smash`… (resto) | **No verificable** | No he podido confirmar release a release los 110 títulos en el tiempo de esta revisión; **no afirmo nada sobre ellos** |

**Además (de la auditoría previa, ya verificada):** `Zelda64Recomp/Zelda64Recomp`, `BanjoRecomp/BanjoRecomp`, `N64Recomp/N64Recomp`, `coop-deluxe/sm64coopdx`, `STJr/SRB2` y `opentyrian/opentyrian` **no publican asset de Switch**. El maestro incluye `srb2_nx` apuntando a `carstene1ns/switch-srb2` (**404**) y `tyrian_2000_opentyrian` apuntando a `opentyrian/opentyrian` (existe, sin asset Switch). Son dos entradas más con problemas.

---

## 5. Conflictos con Delanx: `install_dir` vs `targetDir` (semántica, no ortografía)

### 5.1 La diferencia, explicada

| Concepto | Campo | Responde a la pregunta | Ejemplo (NXQuake) |
|---|---|---|---|
| **Dónde vive la app** | `install_dir` (maestro) / `switch.homeDir` (Delanx) | "¿en qué carpeta de la SD acaba el `.nro`?" | `/switch/nxquake` |
| **Dónde se extrae el archivo** | `switch.targetDir` (Delanx, `ARCHITECTURE.md` §5) | "¿en qué raíz descomprimo el asset?" | **`/`** |
| **Ruta del `.nro` dentro del zip** | `switch.nroPath` | "¿cómo sé cuál es el ejecutable al descomprimir?" | `switch/nxquake/nxquake.nro` |

El zip de NXQuake v0.2.0 **ya contiene la carpeta `switch/nxquake/`** (verificado listando el asset: `nxquake.nro` 9.471.235 B + `id1/`). Por tanto se extrae en la **raíz de la SD** (`targetDir: "/"`), y el resultado final *es* `/switch/nxquake/nxquake.nro`. Si se hiciera caso al `install_dir` del maestro y se extrajera en `/switch/nxquake`, el fichero acabaría en `/switch/nxquake/switch/nxquake/nxquake.nro`: **roto**.

El caso inverso es **GZDoom**: su zip `gzdoom_nx_*.zip` contiene `gzdoom/gzdoom.nro` (sin prefijo `switch/`), así que Delanx usa `targetDir: "/switch"` y la app acaba en `/switch/gzdoom/gzdoom.nro`. Aquí el `install_dir` del maestro (`/switch/gzdoom`) **coincide por casualidad** con el resultado, pero **no** es la raíz de extracción.

**Consecuencia para el importador:** como el contenido del asset no está verificado, `import_ports_md.py` usa la carpeta final como `targetDir` y lo **avisa en `notes`** (`PATH_NOTE`). Es la decisión correcta en ausencia de verificación, y está bien documentada en la cabecera del script. Pero significa que **las 104 entradas importadas son instalables solo si su zip NO trae el prefijo `switch/`** — y las entradas del maestro no dan esa información. Es el hueco de datos más importante del documento.

### 5.2 Los conflictos que detectó `tools/import_ports_md.py --check`

Resultado **reproducible** (3 ejecuciones consecutivas idénticas, más una cuarta de confirmación):

```
entradas del catálogo maestro          : 110
  con github_repo                     : 110
  sin github_repo (no instalables)     : 0
ya presentes en Delanx (no se duplican) : 6
nuevas en ports-master.json             : 104
conflictos de ruta/ejecutable           : 7
total resultante (verificadas + nuevas) : 150
```

Las 6 ya presentes (emparejadas **solo por `repository`**): Ghostship, Ship of Harkinian, Golden Balloon NX, nxquake2, DevilutionX, NXEngine-Evo.

Los 7 conflictos, clasificados:

| # | Entrada | Campo | Delanx (verificado) | Maestro | Clasificación |
|---|---|---|---|---|---|
| 1 | Ship of Harkinian | `targetDir` | `/switch/soh` | `/switch/shipofharkinian` | **Error del maestro** (el `readme.txt` del propio zip documenta `switch/soh/`) |
| 2 | Ship of Harkinian | ejecutable | `soh.nro` | `shipofharkinian.nro` | **Error del maestro** (`soh.nro` está listado dentro del asset, 21.174.023 B) |
| 3 | Golden Balloon NX | `targetDir` | `/switch/goldenballoon` | `/switch/goldenballoon_nx` | **Error del maestro** |
| 4 | Golden Balloon NX | ejecutable | `goldenballoon.nro` | `goldenballoon_nx.nro` | **Error del maestro** |
| 5 | nxquake2 | `targetDir` | `/` | `/switch/nxquake2` | **Semántica distinta**, no error: el maestro dice dónde vive la app, Delanx dice dónde extraer (el zip trae `switch/nxquake2/`) |
| 6 | NXEngine-Evo | `targetDir` | `/switch` | `/switch/nxengine` | **Semántica distinta**: el asset del release **es** un `.nro` suelto (`NXEngine-Evo-v2.6.5-Switch.nro`), no un zip con carpeta; `/switch/nxengine` es una convención que el autor no documenta |
| 7 | NXEngine-Evo | ejecutable | `NXEngine-Evo-v2.6.5-Switch.nro` | `nxengine.nro` | **Error del maestro**: el nombre real del asset es el versionado |

**Resumen: 5 son errores del maestro y 2 son solo semántica distinta** (y en el caso de NXEngine-Evo el `targetDir` es "semántica distinta" solo en apariencia: el maestro presupone una estructura de carpeta que el asset no tiene).

### 5.3 Dos avisos importantes sobre esa lista de 7 (y sobre los "11" del enunciado)

**(a) Los conflictos que aparecen y desaparecen no son un fallo del maestro: son dos puntos ciegos del importador.**

Al principio de mi sesión, `--check` me devolvió **8 ya presentes / 11 conflictos / 148 total**, con dos conflictos extra:

```
! Duke Nukem 3D (EDuke32-NX): targetDir Delanx=/switch/eduke32 <> catálogo maestro=/switch/fury
! Duke Nukem 3D (EDuke32-NX): ejecutable real=eduke32.nro <> catálogo maestro=fury.nro
! Sonic the Hedgehog 1 (Retro Engine): targetDir Delanx=/switch/sonic1 <> catálogo maestro=/switch/sonic2
! Sonic the Hedgehog 1 (Retro Engine): ejecutable real=sonic1.nro <> catálogo maestro=sonic2.nro
```

Repetido después, el resultado estable es 6/7/150, **sin** esos cuatro. La causa está en el script, no en el documento:

- `by_repo` es un **diccionario de cubos mutado en tiempo de ejecución**: al emparejar la primera entrada de un repo se hace `bucket.pop(0)` y, tras añadir una entrada nueva, `by_repo[key] = []` **machacando el cubo compartido** (línea 241). Resultado: **por cada repositorio solo se compara/empareja una entrada**, y el orden de procesamiento decide cuáles. Con `fgsfdsfgs/eduke32` (Duke Nukem 3D **e** Ion Fury) y `Rubberduckycooly/Sonic-1-2-2013-Decompilation` (Sonic 1 **y** Sonic 2) el emparejamiento cruza títulos distintos: la entrada de Ion Fury se compara contra **Duke Nukem 3D** de Delanx y "descubre" que Delanx tiene `fury` en `/switch/eduke32`.
- El "11" del enunciado corresponde a esa ejecución; el estado reproducible actual es **7**. Mi recomendación es no citar ninguno de los dos como cifra fija hasta arreglar el script (ver §7, recomendación 9).

**(b) Dos conflictos adicionales que el `--check` NO detecta, porque el maestro tiene el repositorio mal** (el emparejamiento es solo por `repository`, así que una entrada con el repo equivocado nunca se compara):

| Entrada | Delanx (verificado) | Maestro | Por qué no se detecta |
|---|---|---|---|
| SpaghettiKart (Mario Kart 64) | `/switch/Spaghettify` + `Spaghettify.nro`, `nroPath: Spaghettify-Switch/Spaghettify.nro`, `stripRoot: true` | `HarbourMasters/SpaghettiKart` no está en el maestro; la entrada equivalente es `n64decomp/mk64` con `/switch/mk64` + `mk64.nro` | Repo distinto ⇒ no hay pareja ⇒ no hay conflicto |
| StarFox 64 | `HarbourMasters/Starship` + `Starship.nro` | `Zelda64Recomp/StarFox64Recomp` + `sf64.nro` (repo 404) | idem |

**Los "conflictos" que reporta el importador son solo la punta del iceberg**: hay más discrepancias reales en entradas que el script no puede emparejar.

---

## 6. Recomendaciones accionables

### 6.1 Copiar tal cual del maestro (coste bajo, valor alto)

1. **Adoptar el esquema de `id` estable del maestro.** Es el mayor acierto del documento. Añadir `id` a las 46 entradas verificadas (y al importador) y usarlo como clave de la caché de arte (`Cache/artwork/<id>.png`) y de `romfs:/icons/<id>.png`. Coste: ~1 h de script + renombrar caché. Beneficio: el arte y los iconos dejan de depender del `folderName`, que cambia cuando cambia el catálogo.
2. **Adoptar `category` y `rom_source_hint`.** `category` mejora la navegación de la UI; `rom_source_hint` es un aviso honesto al usuario ("necesitas tu propia ROM USA/JP"). Coste: dos campos pasivos, cero lógica.
3. **Adoptar los `required_files` como nombres lógicos** y seguir resolviéndolos a `sdmc:<homeDir>/<nombre>` (es lo que ya hace `import_ports_md.py`). No adoptar `asset_type` como contrato de instalación: úsalo como pista textual en `notes`.
4. **Adoptar el fallback RomFS `romfs:/icons/<id>.png`.** Es la mejor idea de diseño para modo avión/offline. Reglas: (a) clave = `id` estable; (b) reescalar a 256×256 y optimizar PNG para no inflar el NRO actual de 11,3 MB (los PNG crudos de Libretro pesan 200–785 KB cada uno ⇒ 110 iconos sin tocar serían ~55 MB); (c) el flujo de arte debe caer al icono RomFS **tanto** si no hay red **como** si la descarga devuelve 404/timeout — hoy el maestro solo contempla "sin red".
5. **Adoptar el arte de Libretro Thumbnails tal cual, pero solo el verificado**: hay **34 PNG que responden 200 sin clave** (más 3 reparables con los nombres de §3.1). Copiar esos enlaces a `appIconUrl` es seguro y gratis.
6. **Mantener las notas de procedencia y el estilo de `notes`** del maestro en el catálogo: en una entrada `verified: false` la nota es la única defensa del usuario.

### 6.2 Corregir en el maestro (antes de usar cualquier cosa)

7. **Reparar los 19 repositorios inexistentes** (§4.2) o **eliminar esas entradas**. Un catálogo con un 17 % de repositorios 404 no es un catálogo, es una lista de deseos. Los correcciones conocidas: `StevensND/nfsmw_nx` → `StevensND/nfsmw-nx`; `tx00100xt/Serious-Engine-Switch` → `fgsfdsfgs/Serious-Engine`; `srb2_nx` → abandonar (no hay asset Switch de SRB2); `starfox_64_nx` → `HarbourMasters/Starship`; `mario_kart_64_nx` → `HarbourMasters/SpaghettiKart`.
8. **Reparar los 13 enlaces de arte y asumir que SteamGridDB no es "abierta"**: (a) sustituir los 13 PNG roto por los nombres verificados que doy en §3.1; (b) reescribir el texto de §2 para decir la verdad — la API v2 de SteamGridDB **exige token** (`401 Authentication Required` verificado), y las URLs `/search/autocomplete/...` **son endpoints de búsqueda JSON, no imágenes**; (c) quitar de las 63 entradas de PC la columna de miniatura "directa" o marcarla como "requiere clave y dos peticiones".
9. **Arreglar el emparejamiento de `import_ports_md.py`** (no es el maestro, pero contamina la revisión del maestro): usar `by_repo` como **lista sin mutar** (`bucket.pop(0)` sin el `by_repo[key] = []` final) y **emparejar por `(repository, ejecutable/homeDir)`** o exigir coincidencia de repo **y** de carpeta final. Sin eso, las cifras de "ya presentes"/"conflictos" oscilan entre 8/11 y 6/7 según el orden, y se cruzan Duke Nukem 3D con Ion Fury y Sonic 1 con Sonic 2. Añadir además un test de regresión que fije el valor esperado.
10. **Añadir a cada entrada del maestro los tres campos que la hacen instalable**: `release_asset_filter` (patrón del asset), `nro_path` (ruta del `.nro` **dentro** del zip) y `strip_root` (si el zip envuelve en carpeta raíz). Sin ellos, el importador solo puede *adivinar* la raíz de extracción (es exactamente lo que hace hoy, y lo dice en las notas).

### 6.3 Qué NO adoptar (y el coste de adoptarlo)

11. **NO mezclar las 102–104 entradas sin verificar en `switch-catalog.json`.** El importador ya lo hace bien (fichero aparte `ports-master.json`, `verified: false` en todo, `switchProof: []`, entradas verificadas intactas). Mezclar coste: (a) el instalador de Delanx trataría como instalables ~19 repos inexistentes y ~13+ títulos sin build de Switch ⇒ **descargas fallidas en la consola**, que es el peor sitio posible para fallar; (b) se diluye la semántica de `verified`, que `ARCHITECTURE.md` §4 usa para decidir si pide confirmación al usuario; (c) se pierde la trazabilidad `switchProof` que hoy respalda las 46 entradas. **Mantener dos catálogos.**
12. **NO adoptar `nlohmann/json`.** `ARCHITECTURE.md` §2 especifica `core/json (jansson)` y el árbol de dependencias está construido sobre eso. nlohmann es *header-only* C++17 y aumenta el tiempo de compilación y el tamaño; no aporta nada que jansson no haga aquí. Coste de cambiarlo: tocar `core/json`, `model/*`, `services/*` y el `Makefile`, y re-testear en hardware. **Beneficio: cero.**
13. **NO renombrar la app a `QuiverNX` ni `ports.json` a `apps.json`.** Este punto es el más caro de los tres: (a) `apps.json` es el **contrato de interoperabilidad con Quiver** (`ARCHITECTURE.md` §3.1: mismos campos, `repositorySource`, `catalogEntryId`, `preferredVersion`, `mods`); renombrarlo rompe la lectura de bibliotecas existentes y el intercambio con el launcher de escritorio; (b) `Delanx.nro`/`Delanx.nro` aparece en el `Makefile`, el `.nacp`, el NRO de 11,3 MB ya construido, `Delanx-0.1.0-sdcard.zip`, el README, los docs y probablemente `envSetNextLoad`/rutas de staging (`sdmc:/switch/Delanx/Temp/`); (c) el documento maestro **no es la especificación del proyecto**: es una propuesta que incluye un nombre distinto, y adoptarlo costaría un renombrado completo para ganar únicamente coherencia nominal con la carpeta `roms/`. Lo único que sí vale la pena rescatar de ese punto es la **idea** de un `cache/` bajo la carpeta de la app, que Delanx ya cubre con `Cache/artwork/`.
14. **NO adoptar la estructura `roms/` centralizada como obligatoria** (§2.2): varios de los ports verificados leen sus datos junto al `.nro`. Si se quiere ofrecer, debe ser una opción que **copie o enlace** los datos a la carpeta de la app, con la advertencia de espacio en SD.

---

## 7. Cómo se ha verificado (reproducible)

| Comprobación | Método | Resultado |
|---|---|---|
| Integridad del documento | Parseo del bloque ```` ```json ```` y de las filas de tabla | 110 filas, 110 objetos, **17 claves idénticas en los 110** |
| Arte Libretro | 47 × `HEAD` a `raw.githubusercontent.com` (12 en paralelo, repetido 12×) | **34 × HTTP 200 `image/png`** (200–785 KB), **13 × 404** |
| Arte SteamGridDB | `GET /api/v2/search/autocomplete/Quake%20II`, `GET /api/v2`, `GET /api/v2/docs` | **401 `Authentication Required`**, 200 (portada), **404** |
| Repositorios | `HEAD https://github.com/<repo>` | **19 × 404**, resto 200 (orgs `N64Recomp` y `Zelda64Recomp` existen) |
| Assets de Switch | `releases.atom` + `releases/expanded_assets/<tag>` | `.nro` confirmados en Melee-NX, dusklight-nx (fork), silent-hill-decomp-nx, nfsmw-nx, SwitchWaker, tmc, sonic_advance2_nx, Daytona, port0r, strikers, oracles-nx |
| Colecciones Libretro | `raw.githubusercontent.com/.../master/.gitmodules` | 124 colecciones; **no existe** `FBNeo_-_Arcade_Games` |
| Conflictos con Delanx | `python tools/import_ports_md.py --check` × 4 | Reproducible **6 / 104 / 7 / 150**; una ejecución previa dio **8 / 102 / 11 / 148** (ver §5.3a) |
| Ficheros de Delanx | Lectura de `switch-catalog.json` (46 entradas) y `ARCHITECTURE.md` §3.1/§5 | `targetDir` = raíz de extracción, `nroPath` = ruta en el zip |

**Python usado:** `C:\Users\night\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe` (el del harness). **No se ha ejecutado `make`** y **no se ha modificado ningún otro fichero**.

---

## 8. Limpieza

Durante la auditoría generé dos ficheros temporales de trabajo en la raíz del repositorio (`_tmp_thumbs.tsv`, `_tmp_libretro.txt`) y **los elimino**; este informe es el único fichero nuevo que queda.

---

## 9. Veredicto final (5 líneas)

1. **La estrategia de arte es lo mejor del documento y a la vez su mayor deuda**: acertar con Libretro Thumbnails (34/47 URLs funcionan **sin clave**, verificado) es un hallazgo real y el fallback `romfs:/icons/<id>.png` es la decisión de diseño más valiosa; pero 13 enlaces están rotos y las 63 entradas de SteamGridDB **no funcionan como están escritas** (401 sin token + son búsquedas JSON, no imágenes).
2. **Como catálogo de instalación, el maestro no es utilizable tal cual**: 19 repositorios citados dan HTTP 404, 3 títulos (2ship2Harkinian, Lighthouse, PaperBoat) no tienen build de Switch —y otros como `smash_64_recomp`/`astrocore` tampoco, con repos inexistentes— y decenas de nombres de `.nro` son convenciones inventadas (`soh.nro`, `goldenballoon.nro`, `Melee-NX.nro`, `SilentHillPC.nro`, `nfsmw-nx-usa.nro`).
3. **El esquema de datos sí merece ser adoptado**: `id` estable, `category`, `required_files`, `rom_source_hint` y el flujo de arte son aportaciones genuinas que Delanx no tiene, y encajan sin romper nada; el fallback RomFS es directamente necesario para modo avión.
4. **Los "11 conflictos" deben tratarse con pinzas**: el estado reproducible es **6 ya presentes / 104 nuevas / 7 conflictos / 150 totales**, y de esos 7, **5 son errores del maestro y 2 son semántica distinta `install_dir` vs `targetDir`**; los conflictos Duke Nukem 3D↔Ion Fury y Sonic 1↔Sonic 2 que aparecieron primero eran artefactos del emparejamiento por repositorio de `import_ports_md.py`, que hay que arreglar (y hay más discrepancias reales que el script no ve, como SpaghettiKart y StarFox 64).
5. **Recomendación operativa:** conservar los dos catálogos (`switch-catalog.json` verificado como fuente de instalación, `ports-master.json` como capa de presentación con `verified: false`), importar del maestro **solo** ids, categorías, avisos de ROM y los 34 enlaces de arte válidos, corregir los 13 PNG y los 19 repos en el maestro, y **no** tocar `nlohmann/json` (jansson ya está integrado) ni renombrar `Delanx`/`apps.json` (rompe el contrato con Quiver y todo el árbol de build) — el coste de ambos cambios es alto y el beneficio, nulo.
