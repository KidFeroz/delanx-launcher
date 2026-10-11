# Catálogo Maestro de Ports, Decomps y Recomps para Nintendo Switch
## Con Arquitectura Híbrida de Miniaturas: Libretro Thumbnails & SteamGridDB

Este documento recopila la base de datos completa de proyectos de descompilación (*decomps*), recompilación estática (*recomps*) y adaptaciones nativas (*source ports*) compatibles con **Nintendo Switch** (formato ejecutable `.nro`).
Integra el estándar de directorios en la tarjeta MicroSD y la **estrategia de obtención de miniaturas a costo cero ($0)** combinando los CDNs públicos de **Libretro Thumbnails** (consolas clásicas) y **SteamGridDB** (PC, indies y arcades).

## 1. Estructura General de Directorios en la MicroSD (`sdmc:/`)

```text
sdmc:/
├── roms/                               # Depósito central de ROMs/ISOs sin procesar
│   ├── n64/                            # ROMs .z64 (OoT, MM, SM64, DKR, F-Zero X)
│   ├── snes/                           # ROMs .sfc (A Link to the Past, Super Metroid)
│   ├── gba/                            # ROMs .gba (Minish Cap, Sonic Advance)
│   ├── gbc/                            # ROMs .gbc (Link's Awakening DX, Oracles)
│   ├── ps1/                            # Imágenes de disco .bin/.cue (Driver 2, Silent Hill)
│   ├── gc/                             # ISOs de GameCube (Wind Waker, Twilight Princess, Melee)
│   └── arcade/                         # ROMsets MAME / Model 2 (Daytona USA, Virtua Fighter 2)
│
├── switch/                             # Directorio estándar de Homebrew de Nintendo Switch
│   ├── QuiverNX/                       # Directorio del Launcher autónomo
│   │   ├── QuiverNX.nro                # Ejecutable del Launcher
│   │   ├── ports.json                  # Manifiesto local de juegos y metadatos con URLs de arte
│   │   └── cache/                      # Miniaturas descargadas de Libretro / SteamGridDB (<id>.png)
│   │
│   ├── shipofharkinian/                # Zelda: Ocarina of Time (shipofharkinian.nro + oot.otr)
│   ├── 2ship2harkinian/                # Zelda: Majora's Mask (2ship2harkinian.nro + mm.o2r)
│   ├── Ghostship/                      # Super Mario 64 HarbourMasters (Ghostship.nro + sm64.otr)
│   ├── PaperBoat/                      # Paper Mario 64 (PaperBoat.nro + pm64.o2r)
│   ├── Lighthouse/                     # Banjo-Kazooie (Lighthouse.nro + banjo.otr)
│   ├── goldenballoon_nx/               # Diddy Kong Racing (goldenballoon_nx.nro + dkr.z64)
│   ├── nx-diffuser/                    # F-Zero X (nx-diffuser.nro + assets/)
│   ├── opengoal/                       # Jak and Daxter (jak1.nro + data/iso_data/)
│   ├── simpsons/                       # The Simpsons: Hit & Run (simpsons.nro + *.rcf)
│   └── [resto_de_ports]/               # Cada juego en su subdirectorio aislado
```

## 2. Estrategia Híbrida de Miniaturas ($0 Costo)

- **Libretro Thumbnails (47 títulos):** Aplicado a consolas retro (N64, SNES, GBA, GBC, PS1, GameCube, Saturn, Arcade). Enlaces directos a `raw.githubusercontent.com`, sin API key, sin límites de peticiones documentados y con nombres de carátulas estandarizados de No-Intro. Los enlaces se descargan directamente en formato PNG sin cabeceras de autorización.
- **SteamGridDB (63 títulos):** Aplicado a ports de PC, juegos indie modernos, FPS clásicos de MS-DOS/Windows y fangames independientes. Permite obtener pósters verticales (600×900) y banners panorámicos (1920×620). **Nota técnica sobre autenticación:** la API v2 de SteamGridDB requiere token de autenticación (`Authorization: Bearer <token>`). Los endpoints `/api/v2/search/autocomplete/...` son llamadas de búsqueda que devuelven respuestas JSON (no son imágenes directas). La descarga de imágenes requiere consultar los grids del juego (`/grids/game/<id>`) con la clave de API configurada en `settings.json`.
- **Caché en MicroSD:** Todo arte descargado exitosamente se guarda persistentemente en `sdmc:/switch/QuiverNX/cache/<id>.png`.
- **Fallback local RomFS:** En modo avión, sin red, o si la descarga de miniaturas falla por error de red o HTTP 404, la app recurre automáticamente a `romfs:/icons/<id>.png` empaquetado dentro del propio `.nro` (~2.5 MB para todos los títulos reescalados a 256×256).

### Flujo de Decisión en C++ (`libnx`)

```text
[ Solicitud de Carátula en Launcher ]
                  │
                  ▼
        ¿Existe en MicroSD?
    sdmc:/switch/QuiverNX/cache/<id>.png
              /             \
          [SÍ]               [NO]
           │                  │
   Cargar textura             ▼
   directa en GPU     ¿Conexión a Internet?
                         /             \
                     [SÍ]               [NO]
                      │                  │
        ¿Proveedor en ports.json?         │
             /              \            │
       [Libretro]        [SteamGridDB]    │
           │                   │          │
     Descarga directa      Consulta API   │
     GitHub Raw (PNG)      con API Key    │
           │                   │          │
      ¿Éxito (200)?       ¿Éxito (200)?   │
        /        \          /        \     │
      [SÍ]       [NO]     [SÍ]       [NO]  │
       │           \       │          /    │
  Guardar en SD     └───► Fallback ◄────────┘
  y cargar en GPU       romfs:/icons/<id>.png
```

## 3. Resumen Estadístico del Catálogo

- **Total de títulos documentados:** 110
- **Fuentes de miniaturas:** Libretro Thumbnails: 47 | SteamGridDB: 63

## 4. Tabla Maestra de Puertos, Directorios y Proveedores de Miniaturas

| ID | Título | Plataforma | Tipo | Directorio en SD | Ejecutable | Proveedor Arte | Fuente / Endpoint Miniatura |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `sm64_ghostship` | Super Mario 64 (Ghostship / sm64-port) | Nintendo 64 | `decomp` | `sdmc:/switch/Ghostship/` | `Ghostship.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Super%20Mario%2064%20%28USA%29.png` |
| `oot_ship_of_harkinian` | The Legend of Zelda: Ocarina of Time (Ship of Harkinian) | Nintendo 64 | `decomp` | `sdmc:/switch/soh/` | `soh.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Ocarina%20of%20Time%20%28USA%29.png` |
| `mm_2ship2harkinian` | The Legend of Zelda: Majora's Mask (2Ship2Harkinian) | Nintendo 64 | `decomp` | `sdmc:/switch/2ship2harkinian/` | `2ship2harkinian.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Majora%27s%20Mask%20%28USA%29.png` |
| `paper_mario_paperboat` | Paper Mario 64 (PaperBoat) | Nintendo 64 | `decomp` | `sdmc:/switch/PaperBoat/` | `PaperBoat.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Paper%20Mario%20%28USA%29.png` |
| `banjo_kazooie_lighthouse` | Banjo-Kazooie (Lighthouse / BanjoRecomp) | Nintendo 64 | `decomp_recomp` | `sdmc:/switch/Lighthouse/` | `Lighthouse.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Banjo-Kazooie%20%28USA%29.png` |
| `dkr_golden_balloon` | Diddy Kong Racing (Golden Balloon NX) | Nintendo 64 | `decomp` | `sdmc:/switch/goldenballoon/` | `goldenballoon.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Diddy%20Kong%20Racing%20%28USA%29%20%28En%2CFr%29.png` |
| `fzero_x_nx_diffuser` | F-Zero X (NX-Diffuser / G-Diffuser) | Nintendo 64 | `decomp` | `sdmc:/switch/nx-diffuser/` | `nx-diffuser.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/F-Zero%20X%20%28USA%29.png` |
| `perfect_dark_nx` | Perfect Dark | Nintendo 64 | `decomp` | `sdmc:/switch/perfect_dark/` | `pd.arm64.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Perfect%20Dark%20%28USA%29.png` |
| `starfox_64_nx` | Star Fox 64 (Starship / SF64 Recompiled) | Nintendo 64 | `recomp` | `sdmc:/switch/Starship/` | `Starship.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Star%20Fox%2064%20%28USA%29.png` |
| `mario_kart_64_nx` | Mario Kart 64 (mk64-port / SpaghettiKart) | Nintendo 64 | `decomp` | `sdmc:/switch/Spaghettify/` | `Spaghettify.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Mario%20Kart%2064%20%28USA%29.png` |
| `doom_64_ex_nx` | Doom 64 (Doom64EX-NX) | Nintendo 64 | `recreation` | `sdmc:/switch/doom64ex/` | `doom64ex.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Doom%2064%20%28USA%29.png` |
| `smash_64_recomp` | Super Smash Bros. 64 (Smash 64 Recomp) | Nintendo 64 | `recomp` | `sdmc:/switch/smash64recomp/` | `smash64recomp.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Super%20Smash%20Bros.%20%28USA%29.png` |
| `pokemon_stadium_ss_anne` | Pokemon Stadium (SS Anne) | Nintendo 64 | `decomp` | `sdmc:/switch/ssanne/` | `ssanne.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Pokemon%20Stadium%20%28USA%29.png` |
| `bomberman_64_recomp` | Bomberman 64 (Bomberman 64: Recompiled) | Nintendo 64 | `recomp` | `sdmc:/switch/bomberman64/` | `bomberman64.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Bomberman%2064%20%28USA%29.png` |
| `castlevania_lod_nx` | Castlevania: Legacy of Darkness | Nintendo 64 | `decomp` | `sdmc:/switch/castlevania_lod/` | `castlevania_lod.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Castlevania%20-%20Legacy%20of%20Darkness%20%28USA%29.png` |
| `quest_64_recomp` | Quest 64 (Quest 64 Recomp) | Nintendo 64 | `recomp` | `sdmc:/switch/quest64/` | `quest64.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Quest%2064%20%28USA%29.png` |
| `goldeneye_007_nx` | GoldenEye 007 (ge007-nx) | Nintendo 64 | `decomp` | `sdmc:/switch/ge007/` | `ge007.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/GoldenEye%20007%20%28USA%29.png` |
| `jak_and_daxter_opengoal` | Jak and Daxter: The Precursor Legacy (OpenGOAL) | PlayStation 2 | `recomp_native` | `sdmc:/switch/jak1/` | `jak1.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation_2/master/Named_Boxarts/Jak%20and%20Daxter%20-%20The%20Precursor%20Legacy%20%28USA%29%20%28En%2CFr%2CDe%2CEs%2CIt%29.png` |
| `simpsons_hit_and_run` | The Simpsons: Hit & Run (simpsons-nx) | PC / PS2 / GameCube | `source_port` | `sdmc:/switch/simpsons/` | `simpsons.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/The%20Simpsons%3A%20Hit%20%26%20Run` |
| `zelda_wind_waker_switchwaker` | The Legend of Zelda: The Wind Waker (SwitchWaker) | GameCube | `decomp` | `sdmc:/switch/SwitchWaker/` | `switchwaker.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20The%20Wind%20Waker%20%28USA%29.png` |
| `zelda_twilight_princess_dusklight` | The Legend of Zelda: Twilight Princess (Dusklight-NX) | GameCube | `decomp` | `sdmc:/switch/dusklight/` | `dusklight.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Twilight%20Princess%20%28USA%29.png` |
| `smash_melee_nx` | Super Smash Bros. Melee (Melee-NX) | GameCube | `decomp` | `sdmc:/switch/Melee-NX/` | `melee-nx.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Super%20Smash%20Bros.%20Melee%20%28USA%29.png` |
| `super_mario_strikers_nx` | Super Mario Strikers (strikers-nx) | GameCube | `decomp` | `sdmc:/switch/strikers/` | `strikers.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Super%20Mario%20Strikers%20%28USA%29.png` |
| `super_mario_galaxy_astrocore` | Super Mario Galaxy (AstroCore) | Nintendo Wii | `decomp` | `sdmc:/switch/AstroCore/` | `AstroCore.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Wii/master/Named_Boxarts/Super%20Mario%20Galaxy%20%28USA%29.png` |
| `animal_crossing_acgc_nx` | Animal Crossing GameCube (ACGC-NX) | GameCube | `decomp` | `sdmc:/switch/acgc/` | `acgc.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Animal%20Crossing%20%28USA%29.png` |
| `nfs_most_wanted_2005` | Need For Speed: Most Wanted 2005 | PC / PS2 | `source_port` | `sdmc:/switch/nfsmw/` | `nfsmw-nx-usa.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Need%20For%20Speed%3A%20Most%20Wanted%202005` |
| `driver_2_redriver2` | Driver 2 (REDRIVER2) | PlayStation | `decomp` | `sdmc:/switch/redriver2/` | `redriver2.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Driver%202%20%28USA%29%20%28Disc%201%29.png` |
| `wipeout_rewrite` | WipEout (wipeout-rewrite) | PlayStation | `decomp` | `sdmc:/switch/wipeout/` | `wipeout.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Wipeout%20%28USA%29.png` |
| `silent_hill_1_nx` | Silent Hill 1 (silent-hill-decomp-nx) | PlayStation | `decomp` | `sdmc:/switch/silenthill/` | `SilentHillPC.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Silent%20Hill%20%28USA%29.png` |
| `jackie_chan_rechan` | Jackie Chan Stuntmaster (ReChan-Switch) | PlayStation | `decomp` | `sdmc:/switch/rechan/` | `rechan.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Jackie%20Chan%20Stuntmaster%20%28USA%29.png` |
| `tomb_raider_openlara` | Tomb Raider I & II (OpenLara) | PlayStation / PC | `recreation` | `sdmc:/switch/openlara/` | `openlara.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Tomb%20Raider%20I%20%26%20II` |
| `pokemon_gen1_recomp` | Pokemon Rojo / Azul / Amarillo (gen1recomp) | Game Boy | `recomp` | `sdmc:/switch/gen1recomp/` | `gen1recomp.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy/master/Named_Boxarts/Pokemon%20-%20Red%20Version%20%28USA%2C%20Europe%29%20%28SGB%20Enhanced%29.png` |
| `pokemon_gen2_recomp` | Pokemon Oro / Plata / Cristal (Gen2Recomped) | Game Boy Color | `recomp` | `sdmc:/switch/gen2recomp/` | `gen2recomp.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/master/Named_Boxarts/Pokemon%20-%20Crystal%20Version%20%28USA%29.png` |
| `zelda_links_awakening_dx_hd` | The Legend of Zelda: Link's Awakening DX HD | Game Boy Color | `recreation` | `sdmc:/switch/zeldaladx/` | `zeldaladx.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Link%27s%20Awakening%20DX%20%28USA%2C%20Europe%29%20%28SGB%20Enhanced%29%20%28GB%20Compatible%29.png` |
| `zelda_oracles_nx` | The Legend of Zelda: Oracle of Seasons & Ages (oracles-nx) | Game Boy Color | `decomp` | `sdmc:/switch/oracles/` | `oracles.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Oracle%20of%20Seasons%20%28USA%2C%20Australia%29.png` |
| `zelda_minish_cap_tmc` | The Legend of Zelda: The Minish Cap (tmc-nx) | Game Boy Advance | `decomp` | `sdmc:/switch/minishcap/` | `tmc_switch.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20The%20Minish%20Cap%20%28USA%29.png` |
| `sonic_advance_2_nx` | Sonic Advance 2 | Game Boy Advance | `decomp` | `sdmc:/switch/sonicadvance2/` | `sonicadvance2.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/master/Named_Boxarts/Sonic%20Advance%202%20%28USA%29%20%28En%2CJa%2CFr%2CDe%2CEs%2CIt%29.png` |
| `zelda_alttp_zelda3` | The Legend of Zelda: A Link to the Past (zelda3) | Super Nintendo | `decomp` | `sdmc:/switch/zelda3/` | `zelda3.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20A%20Link%20to%20the%20Past%20%28USA%29.png` |
| `super_metroid_sm_decomp` | Super Metroid (sm-decomp) | Super Nintendo | `decomp` | `sdmc:/switch/supermetroid/` | `sm.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Super%20Metroid%20%28Japan%2C%20USA%29%20%28En%2CJa%29.png` |
| `super_mario_world_smw_decomp` | Super Mario World (smw-decomp) | Super Nintendo | `decomp` | `sdmc:/switch/smw/` | `smw.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Super%20Mario%20World%20%28USA%29.png` |
| `daytona_usa_nx` | Daytona USA (Daytona-NX-Modern) | Arcade (Sega Model 2) | `recomp` | `sdmc:/switch/daytona/` | `daytona.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Saturn/master/Named_Boxarts/Daytona%20USA%20%28USA%29.png` |
| `virtua_fighter_2_nx` | Virtua Fighter 2 (VF2-NX-Modern) | Arcade (Sega Model 2) | `recomp` | `sdmc:/switch/vf2/` | `vf2.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Saturn/master/Named_Boxarts/Virtua%20Fighter%202%20%28USA%29.png` |
| `virtual_on_nx` | Cyber Troopers Virtual-On (Virtual-On-NX) | Arcade (Sega Model 2) | `recomp` | `sdmc:/switch/virtualon/` | `virtualon.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/FBNeo_-_Arcade_Games/master/Named_Titles/von.png` |
| `killer_instinct_nx` | Killer Instinct 1 & 2 (Killer-Instinct-NX-Modern) | Arcade (Midway) | `recomp` | `sdmc:/switch/killerinstinct/` | `ki.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Killer%20Instinct%20%28USA%29.png` |
| `sega_rally_port0r` | Sega Rally Championship (port0r) | Arcade / Saturn | `source_port` | `sdmc:/switch/segarally/` | `segarally.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Saturn/master/Named_Boxarts/Sega%20Rally%20Championship%20%28USA%29.png` |
| `sonic_the_fighters_nx` | Sonic the Fighters | Arcade (Sega Model 2) | `recomp` | `sdmc:/switch/sonicthefighters/` | `sonicthefighters.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/FBNeo_-_Arcade_Games/master/Named_Titles/sfight.png` |
| `doom_3_dhewm3` | Doom 3 & RoE (dhewm3-nx) | PC | `source_port` | `sdmc:/switch/dhewm3/` | `dhewm3.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Doom%203%20%26%20RoE` |
| `quake_2_nxquake2` | Quake II (nxquake2 / Yamagi Quake II) | PC | `source_port` | `sdmc:/switch/nxquake2/` | `nxquake2.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Quake%20II` |
| `quake_3_arena_nx` | Quake III Arena (ioquake3-nx) | PC | `source_port` | `sdmc:/switch/ioquake3/` | `ioquake3.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Quake%20III%20Arena` |
| `duke_nukem_3d_eduke32` | Duke Nukem 3D (EDuke32-NX) | PC | `source_port` | `sdmc:/switch/eduke32/` | `eduke32.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Duke%20Nukem%203D` |
| `blood_nblood_nx` | Blood (NBlood-NX) | PC | `source_port` | `sdmc:/switch/nblood/` | `nblood.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Blood` |
| `shadow_warrior_voidsw` | Shadow Warrior (VoidSW-NX) | PC | `source_port` | `sdmc:/switch/voidsw/` | `voidsw.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Shadow%20Warrior` |
| `powerslave_pcexhumed` | Powerslave / Exhumed (PCExhumed-NX) | PC | `source_port` | `sdmc:/switch/pcexhumed/` | `pcexhumed.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Powerslave%20/%20Exhumed` |
| `redneck_rampage_nx` | Redneck Rampage (Redneck-NX) | PC | `source_port` | `sdmc:/switch/redneck/` | `redneck.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Redneck%20Rampage` |
| `ion_fury_nx` | Ion Fury (Fury-NX) | PC | `source_port` | `sdmc:/switch/fury/` | `fury.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Ion%20Fury` |
| `wolfenstein_3d_ecwolf` | Wolfenstein 3D & Spear of Destiny (ECWolf-NX) | PC | `source_port` | `sdmc:/switch/ecwolf/` | `ecwolf.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Wolfenstein%203D%20%26%20Spear%20of%20Destiny` |
| `rtc_wolfenstein_iortcw` | Return to Castle Wolfenstein (iortcw-nx) | PC | `source_port` | `sdmc:/switch/iortcw/` | `iortcw.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Return%20to%20Castle%20Wolfenstein` |
| `descent_dxx_rebirth` | Descent 1 & 2 (DXX-Rebirth-NX) | PC | `source_port` | `sdmc:/switch/dxx-rebirth/` | `d1x-rebirth.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Descent%201%20%26%202` |
| `rise_of_the_triad_rott` | Rise of the Triad (rott-nx) | PC | `source_port` | `sdmc:/switch/rott/` | `rott.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Rise%20of%20the%20Triad` |
| `serious_sam_nx` | Serious Sam: The First & Second Encounter | PC | `source_port` | `sdmc:/switch/serioussam/` | `serioussam.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Serious%20Sam%3A%20The%20First%20%26%20Second%20Encounter` |
| `alien_vs_predator_avp` | Alien vs Predator Classic (AvP-NX) | PC | `source_port` | `sdmc:/switch/avp/` | `avp.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Alien%20vs%20Predator%20Classic` |
| `postal_1_nx` | Postal 1 (Postal-NX) | PC | `source_port` | `sdmc:/switch/postal/` | `postal.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Postal%201` |
| `half_life_1_xash3d` | Half-Life 1, Opposing Force, Blue Shift (Xash3D-FWGS) | PC | `source_port` | `sdmc:/switch/xash3d/` | `xash3d.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Half-Life%201%2C%20Opposing%20Force%2C%20Blue%20Shift` |
| `half_life_2_portal_hl2_nx` | Half-Life 2 & Portal (hl2_nx) | PC | `source_port` | `sdmc:/switch/hl2/` | `hl2.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Half-Life%202%20%26%20Portal` |
| `max_payne_1_nx` | Max Payne 1 (max_nx) | PC | `source_port` | `sdmc:/switch/maxpayne/` | `maxpayne.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Max%20Payne%201` |
| `star_wars_dark_forces_tfe` | Star Wars: Dark Forces (The Force Engine - TFE) | PC | `source_port` | `sdmc:/switch/tfe/` | `tfe.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Star%20Wars%3A%20Dark%20Forces` |
| `star_wars_jedi_knight_openjk` | Star Wars Jedi Knight: Jedi Outcast & Jedi Academy (OpenJK) | PC | `source_port` | `sdmc:/switch/openjk/` | `openjk.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Star%20Wars%20Jedi%20Knight%3A%20Jedi%20Outcast%20%26%20Jedi%20Academy` |
| `fallout_1_ce` | Fallout 1 (Fallout-CE Switch) | PC | `source_port` | `sdmc:/switch/fallout1/` | `fallout-ce.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Fallout%201` |
| `fallout_2_ce` | Fallout 2 (Fallout 2-CE Switch) | PC | `source_port` | `sdmc:/switch/fallout2/` | `fallout2-ce.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Fallout%202` |
| `diablo_1_devilutionx` | Diablo 1 & Hellfire (DevilutionX) | PC | `source_port` | `sdmc:/switch/devilutionx/` | `devilutionx.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Diablo%201%20%26%20Hellfire` |
| `tes_daggerfall_unity` | The Elder Scrolls II: Daggerfall (Daggerfall Unity) | PC | `source_port` | `sdmc:/switch/daggerfall/` | `daggerfall.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/The%20Elder%20Scrolls%20II%3A%20Daggerfall` |
| `tes_morrowind_openmw` | The Elder Scrolls III: Morrowind (OpenMW-Switch) | PC | `source_port` | `sdmc:/switch/openmw/` | `openmw.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/The%20Elder%20Scrolls%20III%3A%20Morrowind` |
| `rct2_openrct2` | RollerCoaster Tycoon 1 & 2 (OpenRCT2) | PC | `source_port` | `sdmc:/switch/openrct2/` | `openrct2.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/RollerCoaster%20Tycoon%201%20%26%202` |
| `transport_tycoon_openttd` | Transport Tycoon Deluxe (OpenTTD) | PC | `source_port` | `sdmc:/switch/openttd/` | `openttd.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Transport%20Tycoon%20Deluxe` |
| `caesar_3_julius_augustus` | Caesar III (Julius / Augustus) | PC | `source_port` | `sdmc:/switch/julius/` | `julius.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Caesar%20III` |
| `theme_hospital_corsixth` | Theme Hospital (CorsixTH) | PC | `source_port` | `sdmc:/switch/corsixth/` | `corsixth.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Theme%20Hospital` |
| `command_and_conquer_vanilla` | Command & Conquer: Tiberian Dawn & Red Alert (Vanilla-Conquer) | PC | `source_port` | `sdmc:/switch/vanillaconquer/` | `vanillaconquer.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Command%20%26%20Conquer%3A%20Tiberian%20Dawn%20%26%20Red%20Alert` |
| `dune_2_opendune` | Dune II: Battle for Arrakis (OpenDUNE) | PC | `source_port` | `sdmc:/switch/opendune/` | `opendune.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Dune%20II%3A%20Battle%20for%20Arrakis` |
| `heroes_of_might_magic_2_fheroes2` | Heroes of Might and Magic II (fheroes2) | PC | `source_port` | `sdmc:/switch/fheroes2/` | `fheroes2.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Heroes%20of%20Might%20and%20Magic%20II` |
| `heroes_of_might_magic_3_vcmi` | Heroes of Might and Magic III (VCMI) | PC | `source_port` | `sdmc:/switch/vcmi/` | `vcmi.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Heroes%20of%20Might%20and%20Magic%20III` |
| `chris_sawyers_locomotion_openloco` | Chris Sawyer's Locomotion (OpenLoco) | PC | `source_port` | `sdmc:/switch/openloco/` | `openloco.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Chris%20Sawyer%27s%20Locomotion` |
| `revolt_rvgl` | Re-Volt (RVGL-NX) | PC | `source_port` | `sdmc:/switch/rvgl/` | `rvgl.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Re-Volt` |
| `oddworld_relive` | Oddworld: Abe's Oddysee & Exoddus (R.E.L.I.V.E.) | PC / PS1 | `decomp` | `sdmc:/switch/relive/` | `relive.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Oddworld%3A%20Abe%27s%20Oddysee%20%26%20Exoddus` |
| `gta_3_re3` | Grand Theft Auto III (re3) | PC / PS2 | `decomp` | `sdmc:/switch/re3/` | `re3.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Grand%20Theft%20Auto%20III` |
| `gta_vc_revc` | Grand Theft Auto: Vice City (reVC) | PC / PS2 | `decomp` | `sdmc:/switch/reVC/` | `reVC.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Grand%20Theft%20Auto%3A%20Vice%20City` |
| `gta_sa_nx` | Grand Theft Auto: San Andreas (gtasa_nx) | Android / PC | `native_wrapper` | `sdmc:/switch/gtasa/` | `gtasa.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Grand%20Theft%20Auto%3A%20San%20Andreas` |
| `sonic_1_rsdk` | Sonic the Hedgehog 1 (Retro Engine) | Sega Mega Drive / Móvil | `decomp` | `sdmc:/switch/sonic1/` | `sonic1.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/master/Named_Boxarts/Sonic%20The%20Hedgehog%20%28USA%2C%20Europe%29.png` |
| `sonic_2_rsdk` | Sonic the Hedgehog 2 (Retro Engine) | Sega Mega Drive / Móvil | `decomp` | `sdmc:/switch/sonic2/` | `sonic2.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/master/Named_Boxarts/Sonic%20The%20Hedgehog%202%20%28World%29.png` |
| `sonic_cd_rsdk` | Sonic CD (Retro Engine) | Sega CD / PC | `decomp` | `sdmc:/switch/soniccd/` | `soniccd.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_CD/master/Named_Boxarts/Sonic%20CD%20%28USA%29.png` |
| `sonic_mania_rsdk` | Sonic Mania (RSDKv5-Decompilation) | PC | `decomp` | `sdmc:/switch/sonicmania/` | `sonicmania.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Sonic%20Mania` |
| `sonic_3_air` | Sonic 3 & Knuckles (Sonic 3 A.I.R.) | Sega Mega Drive | `remaster_native` | `sdmc:/switch/sonic3air/` | `sonic3air.nro` | **Libretro Thumbnails** | `https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/master/Named_Boxarts/Sonic%20The%20Hedgehog%203%20%28USA%29.png` |
| `srb2_nx` | Sonic Robo Blast 2 (SRB2-NX) | PC (Motor Doom) | `standalone_engine` | `sdmc:/switch/srb2/` | `srb2.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Sonic%20Robo%20Blast%202` |
| `dr_robotnik_ring_racers` | Dr. Robotnik's Ring Racers (RingRacers-NX) | PC (Motor Doom) | `standalone_engine` | `sdmc:/switch/ringracers/` | `ringracers.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Dr.%20Robotnik%27s%20Ring%20Racers` |
| `another_world_raw` | Another World / Out of This World (Raw-NX) | Amiga / DOS | `recreation` | `sdmc:/switch/raw/` | `raw.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Another%20World%20/%20Out%20of%20This%20World` |
| `flashback_reminiscence` | Flashback: The Quest for Identity (REminiscence-NX) | Amiga / DOS | `recreation` | `sdmc:/switch/reminiscence/` | `reminiscence.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Flashback%3A%20The%20Quest%20for%20Identity` |
| `heart_of_darkness_hode` | Heart of Darkness (Hode-NX) | PlayStation / PC | `recreation` | `sdmc:/switch/hode/` | `hode.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Heart%20of%20Darkness` |
| `bermuda_syndrome_bs` | Bermuda Syndrome (Bermuda-NX) | PC | `recreation` | `sdmc:/switch/bermuda/` | `bermuda.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Bermuda%20Syndrome` |
| `fade_to_black_f2b` | Fade to Black (f2b-nx) | PC / PS1 | `recreation` | `sdmc:/switch/f2b/` | `f2b.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Fade%20to%20Black` |
| `little_big_adventure_twine` | Little Big Adventure 1 & 2 (Twin-E NX) | PC | `source_port` | `sdmc:/switch/twine/` | `twine.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Little%20Big%20Adventure%201%20%26%202` |
| `jazz_jackrabbit_openjazz` | Jazz Jackrabbit (OpenJazzNX) | PC (DOS) | `recreation` | `sdmc:/switch/openjazz/` | `openjazz.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Jazz%20Jackrabbit` |
| `captain_claw_openclaw` | Captain Claw (OpenClaw-NX) | PC | `recreation` | `sdmc:/switch/openclaw/` | `openclaw.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Captain%20Claw` |
| `abuse_nx` | Abuse (abuse-nx) | PC | `source_port` | `sdmc:/switch/abuse/` | `abuse.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Abuse` |
| `commander_keen_cgenius` | Commander Keen Episodios 1-6 (Commander Genius) | PC (DOS) | `recreation` | `sdmc:/switch/cgenius/` | `cgenius.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Commander%20Keen%20Episodios%201-6` |
| `tyrian_2000_opentyrian` | Tyrian 2000 (OpenTyrian-NX) | PC (DOS) | `source_port` | `sdmc:/switch/opentyrian/` | `opentyrian.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Tyrian%202000` |
| `aquaria_nx` | Aquaria (Aquaria-NX) | PC | `source_port` | `sdmc:/switch/aquaria/` | `aquaria.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Aquaria` |
| `celeste_64_nx` | Celeste 64: Fragments of the Mountain | PC | `source_port` | `sdmc:/switch/celeste64/` | `celeste64.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Celeste%2064%3A%20Fragments%20of%20the%20Mountain` |
| `cave_story_nxengine` | Cave Story (NXEngine-evo) | PC | `recreation` | `sdmc:/switch/nxengine/` | `nxengine.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Cave%20Story` |
| `am2r_switch` | Another Metroid 2 Remake (AM2R) | PC | `native_wrapper` | `sdmc:/switch/am2r/` | `am2r.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/Another%20Metroid%202%20Remake` |
| `space_cadet_pinball_nx` | 3D Pinball for Windows - Space Cadet | Windows NT/95 | `decomp` | `sdmc:/switch/pinball/` | `pinball.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/3D%20Pinball%20for%20Windows%20-%20Space%20Cadet` |
| `vvvvvv_nx` | VVVVVV (Make and Play) | PC | `decomp_source` | `sdmc:/switch/vvvvvv/` | `vvvvvv.nro` | **SteamGridDB** | `https://www.steamgriddb.com/api/v2/search/autocomplete/VVVVVV` |

## 5. Archivo de Metadatos Completo Enriquecido (`ports.json`)

A continuación se presenta el bloque JSON integral que contiene todos los metadatos estructurados para ser consumidos directamente por la aplicación en C++ (vía `nlohmann/json` o `jansson`), incluyendo las rutas de instalación, requerimientos de ROMs y las fuentes exactas de miniaturas:

```json
[
  {
    "id": "sm64_ghostship",
    "title": "Super Mario 64 (Ghostship / sm64-port)",
    "original_system": "Nintendo 64",
    "category": "Plataformas 3D",
    "type": "decomp",
    "project": "HarbourMasters / sm64-port",
    "github_repo": "HarbourMasters/Ghostship",
    "install_dir": "sdmc:/switch/Ghostship/",
    "executable": "Ghostship.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "baserom.us.z64"
    ],
    "rom_source_hint": "Super Mario 64 (USA/JP) .z64",
    "notes": "Renderizado a 60 FPS, widescreen 16:9 nativo, soporte de mods Render96 y cámara libre analógica.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Super%20Mario%2064%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sm64_ghostship.png",
    "romfs_fallback": "romfs:/icons/sm64_ghostship.png"
  },
  {
    "id": "oot_ship_of_harkinian",
    "title": "The Legend of Zelda: Ocarina of Time (Ship of Harkinian)",
    "original_system": "Nintendo 64",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "HarbourMasters",
    "github_repo": "HarbourMasters/Shipwright",
    "install_dir": "sdmc:/switch/soh/",
    "executable": "soh.nro",
    "asset_type": "archive_extract_otr",
    "required_files": [
      "oot.otr"
    ],
    "rom_source_hint": "Zelda OoT NTSC 1.0 / GameCube Debug ROM",
    "notes": "Controles giroscópicos con Joy-Con, 60 FPS sin glitches físicos, randomizer y D-Pad para botas (directorio estándar /switch/soh/).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Ocarina%20of%20Time%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/oot_ship_of_harkinian.png",
    "romfs_fallback": "romfs:/icons/oot_ship_of_harkinian.png"
  },
  {
    "id": "mm_2ship2harkinian",
    "title": "The Legend of Zelda: Majora's Mask (2Ship2Harkinian)",
    "original_system": "Nintendo 64",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "HarbourMasters",
    "github_repo": "HarbourMasters/2ship2harkinian",
    "install_dir": "sdmc:/switch/2ship2harkinian/",
    "executable": "2ship2harkinian.nro",
    "asset_type": "archive_extract_o2r",
    "required_files": [
      "mm.o2r"
    ],
    "rom_source_hint": "Zelda Majora's Mask (USA) .z64",
    "notes": "Descompilación completa por HarbourMasters (soporte Switch en desarrollo; build oficial verificado actualmente para PC).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Majora%27s%20Mask%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/mm_2ship2harkinian.png",
    "romfs_fallback": "romfs:/icons/mm_2ship2harkinian.png"
  },
  {
    "id": "paper_mario_paperboat",
    "title": "Paper Mario 64 (PaperBoat)",
    "original_system": "Nintendo 64",
    "category": "RPG",
    "type": "decomp",
    "project": "HarbourMasters",
    "github_repo": "HarbourMasters/PaperBoat",
    "install_dir": "sdmc:/switch/PaperBoat/",
    "executable": "PaperBoat.nro",
    "asset_type": "archive_extract_o2r",
    "required_files": [
      "pm64.o2r"
    ],
    "rom_source_hint": "Paper Mario (USA) .z64",
    "notes": "Descompilación completa por HarbourMasters (PaperBoat; build oficial verificado actualmente para PC).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Paper%20Mario%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/paper_mario_paperboat.png",
    "romfs_fallback": "romfs:/icons/paper_mario_paperboat.png"
  },
  {
    "id": "banjo_kazooie_lighthouse",
    "title": "Banjo-Kazooie (Lighthouse / BanjoRecomp)",
    "original_system": "Nintendo 64",
    "category": "Plataformas 3D",
    "type": "decomp_recomp",
    "project": "HarbourMasters / Wiseguy",
    "github_repo": "HarbourMasters/Lighthouse",
    "install_dir": "sdmc:/switch/Lighthouse/",
    "executable": "Lighthouse.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "banjo.otr"
    ],
    "rom_source_hint": "Banjo-Kazooie (USA v1.0) .z64",
    "notes": "Recompilación estática por HarbourMasters (Lighthouse / BanjoRecomp; build oficial verificado actualmente para PC).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Banjo-Kazooie%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/banjo_kazooie_lighthouse.png",
    "romfs_fallback": "romfs:/icons/banjo_kazooie_lighthouse.png"
  },
  {
    "id": "dkr_golden_balloon",
    "title": "Diddy Kong Racing (Golden Balloon NX)",
    "original_system": "Nintendo 64",
    "category": "Carreras",
    "type": "decomp",
    "project": "boraeskicioglu / besk",
    "github_repo": "boraeskicioglu/goldenballoon_nx",
    "install_dir": "sdmc:/switch/goldenballoon/",
    "executable": "goldenballoon.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "dkr.z64"
    ],
    "rom_source_hint": "Diddy Kong Racing (USA v1.0) .z64",
    "notes": "Descompilación de Diddy Kong Racing con tasa de cuadros desbloqueada a 60 FPS y selector de personaje.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Diddy%20Kong%20Racing%20%28USA%29%20%28En%2CFr%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/dkr_golden_balloon.png",
    "romfs_fallback": "romfs:/icons/dkr_golden_balloon.png"
  },
  {
    "id": "fzero_x_nx_diffuser",
    "title": "F-Zero X (NX-Diffuser / G-Diffuser)",
    "original_system": "Nintendo 64",
    "category": "Carreras",
    "type": "decomp",
    "project": "PalindromicBreadLoaf / inspectredc",
    "github_repo": "PalindromicBreadLoaf/NX-Diffuser",
    "install_dir": "sdmc:/switch/nx-diffuser/",
    "executable": "nx-diffuser.nro",
    "asset_type": "rom_bundle",
    "required_files": [
      "fzerox.z64",
      "64dd_bios.bin",
      "fzerox_ek.bin"
    ],
    "rom_source_hint": "F-Zero X (USA Rev 1.0) + BIOS 64DD + Expansion Kit",
    "notes": "Usa libultraship runtime. Soporta contenido de 64DD Expansion Kit nativo en Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/F-Zero%20X%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/fzero_x_nx_diffuser.png",
    "romfs_fallback": "romfs:/icons/fzero_x_nx_diffuser.png"
  },
  {
    "id": "perfect_dark_nx",
    "title": "Perfect Dark",
    "original_system": "Nintendo 64",
    "category": "FPS",
    "type": "decomp",
    "project": "Ryan Dwyer / fgsfds",
    "github_repo": "perfect-dark-pc-port/perfect_dark",
    "install_dir": "sdmc:/switch/perfect_dark/",
    "executable": "pd.arm64.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "pd.ntsc-final.z64"
    ],
    "rom_source_hint": "Perfect Dark (NTSC Final / PAL) .z64",
    "notes": "Control de doble stick analógico moderno, 60 FPS, widescreen y giroscopio.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Perfect%20Dark%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/perfect_dark_nx.png",
    "romfs_fallback": "romfs:/icons/perfect_dark_nx.png"
  },
  {
    "id": "starfox_64_nx",
    "title": "Star Fox 64 (Starship / SF64 Recompiled)",
    "original_system": "Nintendo 64",
    "category": "Shooter / Acción",
    "type": "recomp",
    "project": "HarbourMasters",
    "github_repo": "HarbourMasters/Starship",
    "install_dir": "sdmc:/switch/Starship/",
    "executable": "Starship.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "sf64.z64"
    ],
    "rom_source_hint": "Star Fox 64 (USA v1.0) .z64",
    "notes": "Port oficial de recompilación de Star Fox 64 por HarbourMasters (Starship) para Nintendo Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Star%20Fox%2064%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/starfox_64_nx.png",
    "romfs_fallback": "romfs:/icons/starfox_64_nx.png"
  },
  {
    "id": "mario_kart_64_nx",
    "title": "Mario Kart 64 (mk64-port / SpaghettiKart)",
    "original_system": "Nintendo 64",
    "category": "Carreras",
    "type": "decomp",
    "project": "HarbourMasters / n64decomp",
    "github_repo": "HarbourMasters/SpaghettiKart",
    "install_dir": "sdmc:/switch/Spaghettify/",
    "executable": "Spaghettify.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "baserom.us.z64"
    ],
    "rom_source_hint": "Mario Kart 64 (USA) .z64",
    "notes": "Port nativo de MK64 para Switch desarrollado bajo HarbourMasters (SpaghettiKart / Spaghettify).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Mario%20Kart%2064%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/mario_kart_64_nx.png",
    "romfs_fallback": "romfs:/icons/mario_kart_64_nx.png"
  },
  {
    "id": "doom_64_ex_nx",
    "title": "Doom 64 (Doom64EX-NX)",
    "original_system": "Nintendo 64",
    "category": "FPS",
    "type": "recreation",
    "project": "Kaiser / fgsfds",
    "github_repo": "fgsfdsfgs/doom64ex",
    "install_dir": "sdmc:/switch/doom64ex/",
    "executable": "doom64ex.nro",
    "asset_type": "rom_or_wad",
    "required_files": [
      "DOOM64.WAD",
      "DOORS.WAD"
    ],
    "rom_source_hint": "Doom 64 N64 ROM o WADs extraídos con Wadgen",
    "notes": "Reconstrucción exacta del motor con iluminación cromática y soporte táctil.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Doom%2064%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/doom_64_ex_nx.png",
    "romfs_fallback": "romfs:/icons/doom_64_ex_nx.png"
  },
  {
    "id": "smash_64_recomp",
    "title": "Super Smash Bros. 64 (Smash 64 Recomp)",
    "original_system": "Nintendo 64",
    "category": "Lucha",
    "type": "recomp",
    "project": "N64 Recomp Community",
    "github_repo": "zestydevy/smash64r",
    "install_dir": "sdmc:/switch/smash64recomp/",
    "executable": "smash64recomp.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "smash64.z64"
    ],
    "rom_source_hint": "Super Smash Bros. (USA) .z64",
    "notes": "Recompilación estática de SSB64 (proyecto zestydevy/smash64r para PC; pendiente publicación de release NRO para Switch).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Super%20Smash%20Bros.%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/smash_64_recomp.png",
    "romfs_fallback": "romfs:/icons/smash_64_recomp.png"
  },
  {
    "id": "pokemon_stadium_ss_anne",
    "title": "Pokemon Stadium (SS Anne)",
    "original_system": "Nintendo 64",
    "category": "Combate / Estrategia",
    "type": "decomp",
    "project": "HarbourMasters",
    "github_repo": "HarbourMasters/SSAnne",
    "install_dir": "sdmc:/switch/ssanne/",
    "executable": "ssanne.nro",
    "asset_type": "archive_extract_o2r",
    "required_files": [
      "stadium.o2r"
    ],
    "rom_source_hint": "Pokemon Stadium (USA v1.0) .z64",
    "notes": "Proyecto SS Anne de HarbourMasters; en desarrollo, pendiente de release oficial para Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Pokemon%20Stadium%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/pokemon_stadium_ss_anne.png",
    "romfs_fallback": "romfs:/icons/pokemon_stadium_ss_anne.png"
  },
  {
    "id": "bomberman_64_recomp",
    "title": "Bomberman 64 (Bomberman 64: Recompiled)",
    "original_system": "Nintendo 64",
    "category": "Acción / Plataformas",
    "type": "recomp",
    "project": "Wiseguy / Recomp Community",
    "github_repo": "N64Recomp/Bomberman64Recomp",
    "install_dir": "sdmc:/switch/bomberman64/",
    "executable": "bomberman64.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "bomberman64.z64"
    ],
    "rom_source_hint": "Bomberman 64 (USA) .z64",
    "notes": "Recompilación estática comunitaria en progreso; pendiente release de binario .nro empaquetado para Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Bomberman%2064%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/bomberman_64_recomp.png",
    "romfs_fallback": "romfs:/icons/bomberman_64_recomp.png"
  },
  {
    "id": "castlevania_lod_nx",
    "title": "Castlevania: Legacy of Darkness",
    "original_system": "Nintendo 64",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "fliperama86",
    "github_repo": "fliperama86/CastlevaniaLegacyOfDarkness",
    "install_dir": "sdmc:/switch/castlevania_lod/",
    "executable": "castlevania_lod.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "castlevania_lod.z64"
    ],
    "rom_source_hint": "Castlevania: Legacy of Darkness (USA) .z64",
    "notes": "Proyecto de ingeniería inversa en desarrollo; pendiente de release empaquetado para Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Castlevania%20-%20Legacy%20of%20Darkness%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/castlevania_lod_nx.png",
    "romfs_fallback": "romfs:/icons/castlevania_lod_nx.png"
  },
  {
    "id": "quest_64_recomp",
    "title": "Quest 64 (Quest 64 Recomp)",
    "original_system": "Nintendo 64",
    "category": "RPG",
    "type": "recomp",
    "project": "ShadowOne333 / Recomp Team",
    "github_repo": "N64Recomp/Quest64Recomp",
    "install_dir": "sdmc:/switch/quest64/",
    "executable": "quest64.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "quest64.z64"
    ],
    "rom_source_hint": "Quest 64 (USA) .z64",
    "notes": "Recompilación estática en progreso basada en N64Recomp; pendiente release de binario .nro para Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/Quest%2064%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/quest_64_recomp.png",
    "romfs_fallback": "romfs:/icons/quest_64_recomp.png"
  },
  {
    "id": "goldeneye_007_nx",
    "title": "GoldenEye 007 (ge007-nx)",
    "original_system": "Nintendo 64",
    "category": "FPS",
    "type": "decomp",
    "project": "ge007 team / Ryan Dwyer",
    "github_repo": "n64decomp/ge007",
    "install_dir": "sdmc:/switch/ge007/",
    "executable": "ge007.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "ge007.z64"
    ],
    "rom_source_hint": "GoldenEye 007 (NTSC Final) .z64",
    "notes": "Descompilación al 100% con controles modernos de doble stick y 60 FPS estables.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64/master/Named_Boxarts/GoldenEye%20007%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/goldeneye_007_nx.png",
    "romfs_fallback": "romfs:/icons/goldeneye_007_nx.png"
  },
  {
    "id": "jak_and_daxter_opengoal",
    "title": "Jak and Daxter: The Precursor Legacy (OpenGOAL)",
    "original_system": "PlayStation 2",
    "category": "Plataformas 3D",
    "type": "recomp_native",
    "project": "OpenGOAL / fildicio",
    "github_repo": "fildicio/jak-project-switch",
    "install_dir": "sdmc:/switch/jak1/",
    "executable": "jak1.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "data/iso_data/"
    ],
    "rom_source_hint": "Jak and Daxter PS2 ISO original",
    "notes": "Recompilación nativa de Jak and Daxter mediante OpenGOAL (binario gk.nro instalado como jak1.nro en /switch/jak1/).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation_2/master/Named_Boxarts/Jak%20and%20Daxter%20-%20The%20Precursor%20Legacy%20%28USA%29%20%28En%2CFr%2CDe%2CEs%2CIt%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/jak_and_daxter_opengoal.png",
    "romfs_fallback": "romfs:/icons/jak_and_daxter_opengoal.png"
  },
  {
    "id": "simpsons_hit_and_run",
    "title": "The Simpsons: Hit & Run (simpsons-nx)",
    "original_system": "PC / PS2 / GameCube",
    "category": "Acción / Sandbox",
    "type": "source_port",
    "project": "q4a / ZenoArrows",
    "github_repo": "q4a/simpsons",
    "install_dir": "sdmc:/switch/simpsons/",
    "executable": "simpsons.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "art/",
      "dialogue/",
      "scripts/",
      "sound/",
      "*.rcf"
    ],
    "rom_source_hint": "Archivos del CD o instalación de PC",
    "notes": "100% jugable, multijugador local en minijuegos, resolución completa y tiempos de carga instantáneos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/The%20Simpsons%3A%20Hit%20%26%20Run",
    "cache_path": "sdmc:/switch/QuiverNX/cache/simpsons_hit_and_run.png",
    "romfs_fallback": "romfs:/icons/simpsons_hit_and_run.png"
  },
  {
    "id": "zelda_wind_waker_switchwaker",
    "title": "The Legend of Zelda: The Wind Waker (SwitchWaker)",
    "original_system": "GameCube",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "centollOS",
    "github_repo": "centollOS/SwitchWaker",
    "install_dir": "sdmc:/switch/SwitchWaker/",
    "executable": "switchwaker.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "game.iso",
      "assets/"
    ],
    "rom_source_hint": "Zelda Wind Waker GameCube ISO",
    "notes": "Descompilación nativa de The Legend of Zelda: The Wind Waker para Nintendo Switch (ejecutable switchwaker.nro).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20The%20Wind%20Waker%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/zelda_wind_waker_switchwaker.png",
    "romfs_fallback": "romfs:/icons/zelda_wind_waker_switchwaker.png"
  },
  {
    "id": "zelda_twilight_princess_dusklight",
    "title": "The Legend of Zelda: Twilight Princess (Dusklight-NX)",
    "original_system": "GameCube",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "HayatoG / souldbminerr",
    "github_repo": "souldbminerr/dusklight-nx",
    "install_dir": "sdmc:/switch/dusklight/",
    "executable": "dusklight.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "dusklight_assets/"
    ],
    "rom_source_hint": "Twilight Princess GameCube ISO",
    "notes": "Optimizado para 720p 30 FPS en frecuencias stock de Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Twilight%20Princess%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/zelda_twilight_princess_dusklight.png",
    "romfs_fallback": "romfs:/icons/zelda_twilight_princess_dusklight.png"
  },
  {
    "id": "smash_melee_nx",
    "title": "Super Smash Bros. Melee (Melee-NX)",
    "original_system": "GameCube",
    "category": "Lucha",
    "type": "decomp",
    "project": "KawaiiBunga",
    "github_repo": "KawaiiBunga/Melee-NX",
    "install_dir": "sdmc:/switch/Melee-NX/",
    "executable": "melee-nx.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "root/"
    ],
    "rom_source_hint": "Super Smash Bros. Melee (USA v1.02) ISO",
    "notes": "Descompilación nativa de Super Smash Bros. Melee corriendo a 60 FPS fijos sin emulación (binario melee.nro / melee-nx.nro).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Super%20Smash%20Bros.%20Melee%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/smash_melee_nx.png",
    "romfs_fallback": "romfs:/icons/smash_melee_nx.png"
  },
  {
    "id": "super_mario_strikers_nx",
    "title": "Super Mario Strikers (strikers-nx)",
    "original_system": "GameCube",
    "category": "Deportes",
    "type": "decomp",
    "project": "new-coke",
    "github_repo": "new-coke/strikers",
    "install_dir": "sdmc:/switch/strikers/",
    "executable": "strikers.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "strikers_data/"
    ],
    "rom_source_hint": "Super Mario Strikers GameCube ISO",
    "notes": "Soporte de paquetes de texturas personalizados y resolución HD.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Super%20Mario%20Strikers%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/super_mario_strikers_nx.png",
    "romfs_fallback": "romfs:/icons/super_mario_strikers_nx.png"
  },
  {
    "id": "super_mario_galaxy_astrocore",
    "title": "Super Mario Galaxy (AstroCore)",
    "original_system": "Nintendo Wii",
    "category": "Plataformas 3D",
    "type": "decomp",
    "project": "SMGCommunity / Petari",
    "github_repo": "SMGCommunity/AstroCore",
    "install_dir": "sdmc:/switch/AstroCore/",
    "executable": "AstroCore.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "data/"
    ],
    "rom_source_hint": "Super Mario Galaxy Wii ISO / WBFS",
    "notes": "Descompilación AstroCore de SMG; proyecto en desarrollo para PC, pendiente release oficial de puerto Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Wii/master/Named_Boxarts/Super%20Mario%20Galaxy%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/super_mario_galaxy_astrocore.png",
    "romfs_fallback": "romfs:/icons/super_mario_galaxy_astrocore.png"
  },
  {
    "id": "animal_crossing_acgc_nx",
    "title": "Animal Crossing GameCube (ACGC-NX)",
    "original_system": "GameCube",
    "category": "Simulación",
    "type": "decomp",
    "project": "ac-decomp team / zwaetschge",
    "github_repo": "zwaetschge/ACGC-NX",
    "install_dir": "sdmc:/switch/acgc/",
    "executable": "acgc.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "acgc_assets/"
    ],
    "rom_source_hint": "Animal Crossing GameCube ISO",
    "notes": "Reloj de tiempo real integrado directamente con el reloj del sistema de Switch.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube/master/Named_Boxarts/Animal%20Crossing%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/animal_crossing_acgc_nx.png",
    "romfs_fallback": "romfs:/icons/animal_crossing_acgc_nx.png"
  },
  {
    "id": "nfs_most_wanted_2005",
    "title": "Need For Speed: Most Wanted 2005",
    "original_system": "PC / PS2",
    "category": "Carreras",
    "type": "source_port",
    "project": "StevensND",
    "github_repo": "StevensND/nfsmw-nx",
    "install_dir": "sdmc:/switch/nfsmw/",
    "executable": "nfsmw-nx-usa.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "CARS/",
      "GLOBAL/",
      "TRACKS/"
    ],
    "rom_source_hint": "Instalación de PC de Need For Speed: Most Wanted (2005)",
    "notes": "Port nativo con assets regionales (nfsmw-nx-usa.nro / nfsmw-nx-pal-en.nro); requiere datos de la versión de PC/PS2.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Need%20For%20Speed%3A%20Most%20Wanted%202005",
    "cache_path": "sdmc:/switch/QuiverNX/cache/nfs_most_wanted_2005.png",
    "romfs_fallback": "romfs:/icons/nfs_most_wanted_2005.png"
  },
  {
    "id": "driver_2_redriver2",
    "title": "Driver 2 (REDRIVER2)",
    "original_system": "PlayStation",
    "category": "Acción / Conducción",
    "type": "decomp",
    "project": "OpenDriver2 team",
    "github_repo": "OpenDriver2/REDRIVER2",
    "install_dir": "sdmc:/switch/redriver2/",
    "executable": "redriver2.nro",
    "asset_type": "disc_rip",
    "required_files": [
      "DRIVER2/"
    ],
    "rom_source_hint": "Imágenes de disco CD1 y CD2 (.bin/.cue) de PS1",
    "notes": "Corre a 60 FPS, widescreen 16:9 real y distancia de dibujado multiplicada.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Driver%202%20%28USA%29%20%28Disc%201%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/driver_2_redriver2.png",
    "romfs_fallback": "romfs:/icons/driver_2_redriver2.png"
  },
  {
    "id": "wipeout_rewrite",
    "title": "WipEout (wipeout-rewrite)",
    "original_system": "PlayStation",
    "category": "Carreras Futuristas",
    "type": "decomp",
    "project": "phoboslab",
    "github_repo": "phoboslab/wipeout-rewrite",
    "install_dir": "sdmc:/switch/wipeout/",
    "executable": "wipeout.nro",
    "asset_type": "disc_extract",
    "required_files": [
      "wipeout/data/"
    ],
    "rom_source_hint": "Archivos del CD original de PlayStation o PC",
    "notes": "Resolución 1080p nativa a 60 FPS con banda sonora completa en OGG/FLAC.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Wipeout%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/wipeout_rewrite.png",
    "romfs_fallback": "romfs:/icons/wipeout_rewrite.png"
  },
  {
    "id": "silent_hill_1_nx",
    "title": "Silent Hill 1 (silent-hill-decomp-nx)",
    "original_system": "PlayStation",
    "category": "Survival Horror",
    "type": "decomp",
    "project": "DerilDX / ChaykaDed",
    "github_repo": "ChaykaDed/silent-hill-decomp-nx",
    "install_dir": "sdmc:/switch/silenthill/",
    "executable": "SilentHillPC.nro",
    "asset_type": "disc_rip",
    "required_files": [
      "SLUS_007.07",
      "assets.bin"
    ],
    "rom_source_hint": "Silent Hill (USA) PSX .bin/.cue",
    "notes": "Port nativo C++ de Silent Hill 1 con corrección de perspectiva geométrica y widescreen (ejecutable SilentHillPC.nro).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Silent%20Hill%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/silent_hill_1_nx.png",
    "romfs_fallback": "romfs:/icons/silent_hill_1_nx.png"
  },
  {
    "id": "jackie_chan_rechan",
    "title": "Jackie Chan Stuntmaster (ReChan-Switch)",
    "original_system": "PlayStation",
    "category": "Beat 'em up",
    "type": "decomp",
    "project": "CostelaCNX",
    "github_repo": "CostelaCNX/ReChan-Switch",
    "install_dir": "sdmc:/switch/rechan/",
    "executable": "rechan.nro",
    "asset_type": "disc_rip",
    "required_files": [
      "rechan_data/"
    ],
    "rom_source_hint": "Jackie Chan Stuntmaster PS1 .bin/.cue",
    "notes": "Puerto nativo con framerate fluido y pantalla completa.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sony_-_PlayStation/master/Named_Boxarts/Jackie%20Chan%20Stuntmaster%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/jackie_chan_rechan.png",
    "romfs_fallback": "romfs:/icons/jackie_chan_rechan.png"
  },
  {
    "id": "tomb_raider_openlara",
    "title": "Tomb Raider I & II (OpenLara)",
    "original_system": "PlayStation / PC",
    "category": "Acción / Plataformas",
    "type": "recreation",
    "project": "XProger",
    "github_repo": "XProger/OpenLara",
    "install_dir": "sdmc:/switch/openlara/",
    "executable": "openlara.nro",
    "asset_type": "game_data",
    "required_files": [
      "data/",
      "audio/"
    ],
    "rom_source_hint": "Archivos de datos de Tomb Raider 1 o 2 (PC / PS1)",
    "notes": "Iluminación dinámica en tiempo real, efectos de agua, 60 FPS y opción en primera persona.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Tomb%20Raider%20I%20%26%20II",
    "cache_path": "sdmc:/switch/QuiverNX/cache/tomb_raider_openlara.png",
    "romfs_fallback": "romfs:/icons/tomb_raider_openlara.png"
  },
  {
    "id": "pokemon_gen1_recomp",
    "title": "Pokemon Rojo / Azul / Amarillo (gen1recomp)",
    "original_system": "Game Boy",
    "category": "RPG",
    "type": "recomp",
    "project": "bryanthaboi",
    "github_repo": "bryanthaboi/gen1recomp",
    "install_dir": "sdmc:/switch/gen1recomp/",
    "executable": "gen1recomp.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "pokemon_red.gb"
    ],
    "rom_source_hint": "ROM Game Boy (Red/Blue/Yellow) .gb",
    "notes": "Renderizado panorámico, interfaz táctil y paletas de color configurables.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy/master/Named_Boxarts/Pokemon%20-%20Red%20Version%20%28USA%2C%20Europe%29%20%28SGB%20Enhanced%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/pokemon_gen1_recomp.png",
    "romfs_fallback": "romfs:/icons/pokemon_gen1_recomp.png"
  },
  {
    "id": "pokemon_gen2_recomp",
    "title": "Pokemon Oro / Plata / Cristal (Gen2Recomped)",
    "original_system": "Game Boy Color",
    "category": "RPG",
    "type": "recomp",
    "project": "UNDERdecoded",
    "github_repo": "UNDERdecoded/Gen2Recomped",
    "install_dir": "sdmc:/switch/gen2recomp/",
    "executable": "gen2recomp.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "pokemon_crystal.gbc"
    ],
    "rom_source_hint": "ROM Game Boy Color (Crystal/Gold/Silver) .gbc",
    "notes": "Sincronización con reloj real y soporte panorámico nativo.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/master/Named_Boxarts/Pokemon%20-%20Crystal%20Version%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/pokemon_gen2_recomp.png",
    "romfs_fallback": "romfs:/icons/pokemon_gen2_recomp.png"
  },
  {
    "id": "zelda_links_awakening_dx_hd",
    "title": "The Legend of Zelda: Link's Awakening DX HD",
    "original_system": "Game Boy Color",
    "category": "Acción / Aventura",
    "type": "recreation",
    "project": "delsonazevedo / Alexgg1014",
    "github_repo": "delsonazevedo/Zelda-LA-DX-HD-Updated",
    "install_dir": "sdmc:/switch/zeldaladx/",
    "executable": "zeldaladx.nro",
    "asset_type": "standalone_or_rom",
    "required_files": [
      "assets/"
    ],
    "rom_source_hint": "Link's Awakening DX .gbc",
    "notes": "Gráficos HD redraw, cámara libre continua y música orquestada.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Link%27s%20Awakening%20DX%20%28USA%2C%20Europe%29%20%28SGB%20Enhanced%29%20%28GB%20Compatible%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/zelda_links_awakening_dx_hd.png",
    "romfs_fallback": "romfs:/icons/zelda_links_awakening_dx_hd.png"
  },
  {
    "id": "zelda_oracles_nx",
    "title": "The Legend of Zelda: Oracle of Seasons & Ages (oracles-nx)",
    "original_system": "Game Boy Color",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "toniisound",
    "github_repo": "toniisound/oracles-nx",
    "install_dir": "sdmc:/switch/oracles/",
    "executable": "oracles.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "seasons.gbc",
      "ages.gbc"
    ],
    "rom_source_hint": "ROMs originales de Game Boy Color",
    "notes": "Unifica ambos títulos en un único menú con soporte de vinculación de contraseñas.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20Oracle%20of%20Seasons%20%28USA%2C%20Australia%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/zelda_oracles_nx.png",
    "romfs_fallback": "romfs:/icons/zelda_oracles_nx.png"
  },
  {
    "id": "zelda_minish_cap_tmc",
    "title": "The Legend of Zelda: The Minish Cap (tmc-nx)",
    "original_system": "Game Boy Advance",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "HayatoG / Alexgg1014",
    "github_repo": "HayatoG/tmc",
    "install_dir": "sdmc:/switch/minishcap/",
    "executable": "tmc_switch.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "tmc.gba"
    ],
    "rom_source_hint": "Zelda Minish Cap (USA/EUR) .gba",
    "notes": "Port nativo de The Minish Cap con soporte panorámico 16:9 y filtros de escalado (ejecutables tmc_switch.nro / TMC-Switch-USA).",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20The%20Minish%20Cap%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/zelda_minish_cap_tmc.png",
    "romfs_fallback": "romfs:/icons/zelda_minish_cap_tmc.png"
  },
  {
    "id": "sonic_advance_2_nx",
    "title": "Sonic Advance 2",
    "original_system": "Game Boy Advance",
    "category": "Plataformas",
    "type": "decomp",
    "project": "boraeskicioglu",
    "github_repo": "boraeskicioglu/sonic_advance2_nx",
    "install_dir": "sdmc:/switch/sonicadvance2/",
    "executable": "sonicadvance2.nro",
    "asset_type": "rom_direct",
    "required_files": [
      "sonicadvance2.gba"
    ],
    "rom_source_hint": "Sonic Advance 2 (USA) .gba",
    "notes": "Ejecución nativa con tasa de cuadros desbloqueada a 60 FPS.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/master/Named_Boxarts/Sonic%20Advance%202%20%28USA%29%20%28En%2CJa%2CFr%2CDe%2CEs%2CIt%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_advance_2_nx.png",
    "romfs_fallback": "romfs:/icons/sonic_advance_2_nx.png"
  },
  {
    "id": "zelda_alttp_zelda3",
    "title": "The Legend of Zelda: A Link to the Past (zelda3)",
    "original_system": "Super Nintendo",
    "category": "Acción / Aventura",
    "type": "decomp",
    "project": "snesrev",
    "github_repo": "snesrev/zelda3",
    "install_dir": "sdmc:/switch/zelda3/",
    "executable": "zelda3.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "zelda3.sfc",
      "zelda3.ini"
    ],
    "rom_source_hint": "Zelda A Link to the Past (USA/JP) .sfc",
    "notes": "Soporte panorámico 16:9 que renderiza áreas ocultas sin scroll pop-in, audio MSU-1 y atajos de objetos.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Legend%20of%20Zelda%2C%20The%20-%20A%20Link%20to%20the%20Past%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/zelda_alttp_zelda3.png",
    "romfs_fallback": "romfs:/icons/zelda_alttp_zelda3.png"
  },
  {
    "id": "super_metroid_sm_decomp",
    "title": "Super Metroid (sm-decomp)",
    "original_system": "Super Nintendo",
    "category": "Metroidvania",
    "type": "decomp",
    "project": "snesrev",
    "github_repo": "snesrev/sm",
    "install_dir": "sdmc:/switch/supermetroid/",
    "executable": "sm.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "sm.sfc"
    ],
    "rom_source_hint": "Super Metroid (JU) .sfc",
    "notes": "Widescreen real, audio sin compresión y mapeo moderno de apuntado para los Joy-Con.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Super%20Metroid%20%28Japan%2C%20USA%29%20%28En%2CJa%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/super_metroid_sm_decomp.png",
    "romfs_fallback": "romfs:/icons/super_metroid_sm_decomp.png"
  },
  {
    "id": "super_mario_world_smw_decomp",
    "title": "Super Mario World (smw-decomp)",
    "original_system": "Super Nintendo",
    "category": "Plataformas",
    "type": "decomp",
    "project": "snesrev / delsonazevedo",
    "github_repo": "snesrev/smw",
    "install_dir": "sdmc:/switch/smw/",
    "executable": "smw.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "smw.sfc"
    ],
    "rom_source_hint": "Super Mario World (USA) .sfc",
    "notes": "Reescritura en C con soporte para pantallas panorámicas y física idéntica al original.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Super%20Mario%20World%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/super_mario_world_smw_decomp.png",
    "romfs_fallback": "romfs:/icons/super_mario_world_smw_decomp.png"
  },
  {
    "id": "daytona_usa_nx",
    "title": "Daytona USA (Daytona-NX-Modern)",
    "original_system": "Arcade (Sega Model 2)",
    "category": "Carreras Arcade",
    "type": "recomp",
    "project": "Thorhax",
    "github_repo": "Thorhax/Daytona-NX-Modern",
    "install_dir": "sdmc:/switch/daytona/",
    "executable": "daytona.nro",
    "asset_type": "arcade_roms",
    "required_files": [
      "roms/daytona/"
    ],
    "rom_source_hint": "ROMset de Daytona USA Arcade",
    "notes": "Recompilación nativa del hardware Model 2 a 60 FPS estables con audio estéreo mejorado.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Saturn/master/Named_Boxarts/Daytona%20USA%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/daytona_usa_nx.png",
    "romfs_fallback": "romfs:/icons/daytona_usa_nx.png"
  },
  {
    "id": "virtua_fighter_2_nx",
    "title": "Virtua Fighter 2 (VF2-NX-Modern)",
    "original_system": "Arcade (Sega Model 2)",
    "category": "Lucha 3D",
    "type": "recomp",
    "project": "Thorhax",
    "github_repo": "Thorhax/VF2-NX-Modern",
    "install_dir": "sdmc:/switch/vf2/",
    "executable": "vf2.nro",
    "asset_type": "arcade_roms",
    "required_files": [
      "roms/vf2/"
    ],
    "rom_source_hint": "ROMset de Virtua Fighter 2 Arcade",
    "notes": "Gráficos poligonales nítidos a 60 FPS sin los artefactos de emuladores pesados.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Saturn/master/Named_Boxarts/Virtua%20Fighter%202%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/virtua_fighter_2_nx.png",
    "romfs_fallback": "romfs:/icons/virtua_fighter_2_nx.png"
  },
  {
    "id": "virtual_on_nx",
    "title": "Cyber Troopers Virtual-On (Virtual-On-NX)",
    "original_system": "Arcade (Sega Model 2)",
    "category": "Acción Mecha",
    "type": "recomp",
    "project": "Thorhax",
    "github_repo": "Thorhax/Virtual-On-NX",
    "install_dir": "sdmc:/switch/virtualon/",
    "executable": "virtualon.nro",
    "asset_type": "arcade_roms",
    "required_files": [
      "roms/von/"
    ],
    "rom_source_hint": "ROMset de Virtual-On Arcade",
    "notes": "Mapeo optimizado para controles Dual-Stick emulando las palancas gemelas arcade.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/FBNeo_-_Arcade_Games/master/Named_Titles/von.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/virtual_on_nx.png",
    "romfs_fallback": "romfs:/icons/virtual_on_nx.png"
  },
  {
    "id": "killer_instinct_nx",
    "title": "Killer Instinct 1 & 2 (Killer-Instinct-NX-Modern)",
    "original_system": "Arcade (Midway)",
    "category": "Lucha",
    "type": "recomp",
    "project": "Thorhax",
    "github_repo": "Thorhax/Killer-Instinct-NX-Modern",
    "install_dir": "sdmc:/switch/killerinstinct/",
    "executable": "ki.nro",
    "asset_type": "arcade_chd_rom",
    "required_files": [
      "roms/kinst/",
      "kinst.chd"
    ],
    "rom_source_hint": "ROM + CHD original de Killer Instinct 1 & 2",
    "notes": "Renderizado sin pérdidas, audio original y combos a 60 FPS nativos.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/master/Named_Boxarts/Killer%20Instinct%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/killer_instinct_nx.png",
    "romfs_fallback": "romfs:/icons/killer_instinct_nx.png"
  },
  {
    "id": "sega_rally_port0r",
    "title": "Sega Rally Championship (port0r)",
    "original_system": "Arcade / Saturn",
    "category": "Carreras Arcade",
    "type": "source_port",
    "project": "jacquesdupontd",
    "github_repo": "jacquesdupontd/port0r",
    "install_dir": "sdmc:/switch/segarally/",
    "executable": "segarally.nro",
    "asset_type": "game_data",
    "required_files": [
      "data/"
    ],
    "rom_source_hint": "Assets de Sega Rally PC / Arcade",
    "notes": "Control de derrape preciso con sticks analógicos y 60 FPS.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Saturn/master/Named_Boxarts/Sega%20Rally%20Championship%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sega_rally_port0r.png",
    "romfs_fallback": "romfs:/icons/sega_rally_port0r.png"
  },
  {
    "id": "sonic_the_fighters_nx",
    "title": "Sonic the Fighters",
    "original_system": "Arcade (Sega Model 2)",
    "category": "Lucha 3D",
    "type": "recomp",
    "project": "boraeskicioglu",
    "github_repo": "boraeskicioglu/sonic_the_fighters_nx",
    "install_dir": "sdmc:/switch/sonicthefighters/",
    "executable": "sonicthefighters.nro",
    "asset_type": "arcade_roms",
    "required_files": [
      "roms/sfight/"
    ],
    "rom_source_hint": "ROMset arcade de Sonic the Fighters",
    "notes": "Puerto nativo de la entrega de lucha poligonal de Sonic Team.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/FBNeo_-_Arcade_Games/master/Named_Titles/sfight.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_the_fighters_nx.png",
    "romfs_fallback": "romfs:/icons/sonic_the_fighters_nx.png"
  },
  {
    "id": "doom_3_dhewm3",
    "title": "Doom 3 & RoE (dhewm3-nx)",
    "original_system": "PC",
    "category": "FPS / Terror",
    "type": "source_port",
    "project": "dhewm team / fgsfds",
    "github_repo": "dhewm/dhewm3",
    "install_dir": "sdmc:/switch/dhewm3/",
    "executable": "dhewm3.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "base/pak000.pk4",
      "base/pak001.pk4",
      "base/pak002.pk4"
    ],
    "rom_source_hint": "Instalación de PC de Doom 3 (Steam / GOG)",
    "notes": "Motor id Tech 4 nativo a 60 FPS con apuntado por giroscopio y soporte para linterna en arma.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Doom%203%20%26%20RoE",
    "cache_path": "sdmc:/switch/QuiverNX/cache/doom_3_dhewm3.png",
    "romfs_fallback": "romfs:/icons/doom_3_dhewm3.png"
  },
  {
    "id": "quake_2_nxquake2",
    "title": "Quake II (nxquake2 / Yamagi Quake II)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "yquake2 team / fgsfds",
    "github_repo": "fgsfdsfgs/nxquake2",
    "install_dir": "sdmc:/switch/nxquake2/",
    "executable": "nxquake2.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "baseq2/pak0.pak"
    ],
    "rom_source_hint": "Instalación de PC de Quake II",
    "notes": "Giroscopio de Joy-Con implementado, música en OGG estéreo y resolución 1080p.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Quake%20II",
    "cache_path": "sdmc:/switch/QuiverNX/cache/quake_2_nxquake2.png",
    "romfs_fallback": "romfs:/icons/quake_2_nxquake2.png"
  },
  {
    "id": "quake_3_arena_nx",
    "title": "Quake III Arena (ioquake3-nx)",
    "original_system": "PC",
    "category": "FPS Arena",
    "type": "source_port",
    "project": "ioquake3 team",
    "github_repo": "fgsfdsfgs/ioquake3",
    "install_dir": "sdmc:/switch/ioquake3/",
    "executable": "ioquake3.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "baseq3/pak0.pk3"
    ],
    "rom_source_hint": "Instalación de PC de Quake III Arena",
    "notes": "Multijugador local con bots o partidas online en servidores de la comunidad.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Quake%20III%20Arena",
    "cache_path": "sdmc:/switch/QuiverNX/cache/quake_3_arena_nx.png",
    "romfs_fallback": "romfs:/icons/quake_3_arena_nx.png"
  },
  {
    "id": "duke_nukem_3d_eduke32",
    "title": "Duke Nukem 3D (EDuke32-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "EDuke32 Team",
    "github_repo": "fgsfdsfgs/eduke32",
    "install_dir": "sdmc:/switch/eduke32/",
    "executable": "eduke32.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "DUKE3D.GRP"
    ],
    "rom_source_hint": "Atomic Edition / 20th Anniversary PC GRP",
    "notes": "Soporta renderizado Polymer, paquetes de texturas en alta resolución y mods HRP.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Duke%20Nukem%203D",
    "cache_path": "sdmc:/switch/QuiverNX/cache/duke_nukem_3d_eduke32.png",
    "romfs_fallback": "romfs:/icons/duke_nukem_3d_eduke32.png"
  },
  {
    "id": "blood_nblood_nx",
    "title": "Blood (NBlood-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "Nuke.YKT",
    "github_repo": "nukeykt/nblood",
    "install_dir": "sdmc:/switch/nblood/",
    "executable": "nblood.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "BLOOD.RFF",
      "SOUNDS.RFF"
    ],
    "rom_source_hint": "Blood: One Unit Whole Blood (GOG / Steam)",
    "notes": "Reconstrucción perfecta del código fuente original de Monolith a 60 FPS estables.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Blood",
    "cache_path": "sdmc:/switch/QuiverNX/cache/blood_nblood_nx.png",
    "romfs_fallback": "romfs:/icons/blood_nblood_nx.png"
  },
  {
    "id": "shadow_warrior_voidsw",
    "title": "Shadow Warrior (VoidSW-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "EDuke32 Team",
    "github_repo": "fgsfdsfgs/voidsw",
    "install_dir": "sdmc:/switch/voidsw/",
    "executable": "voidsw.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "SW.GRP"
    ],
    "rom_source_hint": "Shadow Warrior Classic (PC)",
    "notes": "Motor Build nativo con controles modernos y giroscopio.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Shadow%20Warrior",
    "cache_path": "sdmc:/switch/QuiverNX/cache/shadow_warrior_voidsw.png",
    "romfs_fallback": "romfs:/icons/shadow_warrior_voidsw.png"
  },
  {
    "id": "powerslave_pcexhumed",
    "title": "Powerslave / Exhumed (PCExhumed-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "Nuke.YKT",
    "github_repo": "nukeykt/pcexhumed",
    "install_dir": "sdmc:/switch/pcexhumed/",
    "executable": "pcexhumed.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "STUFF.DAT",
      "BOOK.MOV"
    ],
    "rom_source_hint": "Powerslave versión DOS de PC",
    "notes": "Puerto exacto del motor Build con corrección de bugs y soporte analógico.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Powerslave%20/%20Exhumed",
    "cache_path": "sdmc:/switch/QuiverNX/cache/powerslave_pcexhumed.png",
    "romfs_fallback": "romfs:/icons/powerslave_pcexhumed.png"
  },
  {
    "id": "redneck_rampage_nx",
    "title": "Redneck Rampage (Redneck-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "Nuke.YKT",
    "github_repo": "nukeykt/redneck",
    "install_dir": "sdmc:/switch/redneck/",
    "executable": "redneck.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "REDNECK.GRP"
    ],
    "rom_source_hint": "Redneck Rampage (GOG / Steam)",
    "notes": "Soporta también las expansiones Suckin' Grits on Route 66.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Redneck%20Rampage",
    "cache_path": "sdmc:/switch/QuiverNX/cache/redneck_rampage_nx.png",
    "romfs_fallback": "romfs:/icons/redneck_rampage_nx.png"
  },
  {
    "id": "ion_fury_nx",
    "title": "Ion Fury (Fury-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "Voidpoint / fgsfds",
    "github_repo": "fgsfdsfgs/eduke32",
    "install_dir": "sdmc:/switch/fury/",
    "executable": "fury.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "fury.grp"
    ],
    "rom_source_hint": "Instalación de PC de Ion Fury (Steam / GOG)",
    "notes": "Rendimiento óptimo a 60 FPS sin los límites de memoria de la versión comercial.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Ion%20Fury",
    "cache_path": "sdmc:/switch/QuiverNX/cache/ion_fury_nx.png",
    "romfs_fallback": "romfs:/icons/ion_fury_nx.png"
  },
  {
    "id": "wolfenstein_3d_ecwolf",
    "title": "Wolfenstein 3D & Spear of Destiny (ECWolf-NX)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "Blzut3",
    "github_repo": "fgsfdsfgs/ecwolf",
    "install_dir": "sdmc:/switch/ecwolf/",
    "executable": "ecwolf.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "*.WL6",
      "*.SOD"
    ],
    "rom_source_hint": "Archivos originales de Wolfenstein 3D / Spear of Destiny",
    "notes": "Soporte panorámico nativo, automapa en pantalla y ranuras de guardado ilimitadas.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Wolfenstein%203D%20%26%20Spear%20of%20Destiny",
    "cache_path": "sdmc:/switch/QuiverNX/cache/wolfenstein_3d_ecwolf.png",
    "romfs_fallback": "romfs:/icons/wolfenstein_3d_ecwolf.png"
  },
  {
    "id": "rtc_wolfenstein_iortcw",
    "title": "Return to Castle Wolfenstein (iortcw-nx)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "iortcw team / fgsfds",
    "github_repo": "fgsfdsfgs/iortcw",
    "install_dir": "sdmc:/switch/iortcw/",
    "executable": "iortcw.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "main/pak0.pk3",
      "main/mp_pak0.pk3"
    ],
    "rom_source_hint": "Return to Castle Wolfenstein (Steam / GOG)",
    "notes": "Aceleración por hardware completa, 60 FPS estables y giroscopio.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Return%20to%20Castle%20Wolfenstein",
    "cache_path": "sdmc:/switch/QuiverNX/cache/rtc_wolfenstein_iortcw.png",
    "romfs_fallback": "romfs:/icons/rtc_wolfenstein_iortcw.png"
  },
  {
    "id": "descent_dxx_rebirth",
    "title": "Descent 1 & 2 (DXX-Rebirth-NX)",
    "original_system": "PC",
    "category": "FPS 6DOF",
    "type": "source_port",
    "project": "dxx-rebirth team",
    "github_repo": "fgsfdsfgs/dxx-rebirth",
    "install_dir": "sdmc:/switch/dxx-rebirth/",
    "executable": "d1x-rebirth.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "descent.hog",
      "descent.pig"
    ],
    "rom_source_hint": "Descent 1 / 2 versión PC",
    "notes": "Mapeo completo de 6 grados de libertad en sticks analógicos y giroscopio.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Descent%201%20%26%202",
    "cache_path": "sdmc:/switch/QuiverNX/cache/descent_dxx_rebirth.png",
    "romfs_fallback": "romfs:/icons/descent_dxx_rebirth.png"
  },
  {
    "id": "rise_of_the_triad_rott",
    "title": "Rise of the Triad (rott-nx)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "fgsfds",
    "github_repo": "fgsfdsfgs/rott",
    "install_dir": "sdmc:/switch/rott/",
    "executable": "rott.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "DARKWAR.WAD"
    ],
    "rom_source_hint": "Rise of the Triad: Dark War (PC)",
    "notes": "Soporta versión shareware y versión completa registrada con música MIDI emulada.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Rise%20of%20the%20Triad",
    "cache_path": "sdmc:/switch/QuiverNX/cache/rise_of_the_triad_rott.png",
    "romfs_fallback": "romfs:/icons/rise_of_the_triad_rott.png"
  },
  {
    "id": "serious_sam_nx",
    "title": "Serious Sam: The First & Second Encounter",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "Croteam / Serious-Engine Team",
    "github_repo": "fgsfdsfgs/Serious-Engine",
    "install_dir": "sdmc:/switch/serioussam/",
    "executable": "serioussam.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "SE1_00.gro",
      "Classes/",
      "Levels/"
    ],
    "rom_source_hint": "Serious Sam Classic: TFE / TSE (Steam / GOG)",
    "notes": "Serious Engine port para Switch por fgsfdsfgs; incluye First y Second Encounter a 60 FPS nativos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Serious%20Sam%3A%20The%20First%20%26%20Second%20Encounter",
    "cache_path": "sdmc:/switch/QuiverNX/cache/serious_sam_nx.png",
    "romfs_fallback": "romfs:/icons/serious_sam_nx.png"
  },
  {
    "id": "alien_vs_predator_avp",
    "title": "Alien vs Predator Classic (AvP-NX)",
    "original_system": "PC",
    "category": "FPS / Terror",
    "type": "source_port",
    "project": "Rebellion / icculus",
    "github_repo": "fgsfdsfgs/avp",
    "install_dir": "sdmc:/switch/avp/",
    "executable": "avp.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "fastfile/",
      "sound/"
    ],
    "rom_source_hint": "Aliens versus Predator Classic 2000 (Steam / GOG)",
    "notes": "Campañas completas de Marine, Alien y Depredador a resolución nativa.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Alien%20vs%20Predator%20Classic",
    "cache_path": "sdmc:/switch/QuiverNX/cache/alien_vs_predator_avp.png",
    "romfs_fallback": "romfs:/icons/alien_vs_predator_avp.png"
  },
  {
    "id": "postal_1_nx",
    "title": "Postal 1 (Postal-NX)",
    "original_system": "PC",
    "category": "Shooter Isométrico",
    "type": "source_port",
    "project": "Running With Scissors / fgsfds",
    "github_repo": "fgsfdsfgs/postal",
    "install_dir": "sdmc:/switch/postal/",
    "executable": "postal.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "res/"
    ],
    "rom_source_hint": "Postal 1 (GOG / Steam)",
    "notes": "Controles tipo twin-stick shooter con los Joy-Con.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Postal%201",
    "cache_path": "sdmc:/switch/QuiverNX/cache/postal_1_nx.png",
    "romfs_fallback": "romfs:/icons/postal_1_nx.png"
  },
  {
    "id": "half_life_1_xash3d",
    "title": "Half-Life 1, Opposing Force, Blue Shift (Xash3D-FWGS)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "FWGS Team",
    "github_repo": "FWGS/xash3d-fwgs",
    "install_dir": "sdmc:/switch/xash3d/",
    "executable": "xash3d.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "valve/pak0.pak",
      "valve/halflife.wad"
    ],
    "rom_source_hint": "Half-Life 1 original de Steam / CD de PC",
    "notes": "Motor GoldSrc independiente; soporta mods comunitarios como Counter-Strike 1.6.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Half-Life%201%2C%20Opposing%20Force%2C%20Blue%20Shift",
    "cache_path": "sdmc:/switch/QuiverNX/cache/half_life_1_xash3d.png",
    "romfs_fallback": "romfs:/icons/half_life_1_xash3d.png"
  },
  {
    "id": "half_life_2_portal_hl2_nx",
    "title": "Half-Life 2 & Portal (hl2_nx)",
    "original_system": "PC",
    "category": "FPS / Puzzles",
    "type": "source_port",
    "project": "NaGaa95 / Source Engine NX Team",
    "github_repo": "NaGaa95/hl2_nx",
    "install_dir": "sdmc:/switch/hl2/",
    "executable": "hl2.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "hl2/",
      "portal/"
    ],
    "rom_source_hint": "Archivos VPK y carpetas de Half-Life 2 / Portal de Steam",
    "notes": "Motor Source con físicas Havok nativas aceleradas por GPU.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Half-Life%202%20%26%20Portal",
    "cache_path": "sdmc:/switch/QuiverNX/cache/half_life_2_portal_hl2_nx.png",
    "romfs_fallback": "romfs:/icons/half_life_2_portal_hl2_nx.png"
  },
  {
    "id": "max_payne_1_nx",
    "title": "Max Payne 1 (max_nx)",
    "original_system": "PC",
    "category": "Acción en 3ª Persona",
    "type": "source_port",
    "project": "NaGaa95",
    "github_repo": "NaGaa95/max_nx_v2.1.131",
    "install_dir": "sdmc:/switch/maxpayne/",
    "executable": "maxpayne.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "x_data.ras",
      "x_music.ras",
      "x_english.ras"
    ],
    "rom_source_hint": "Instalación de PC de Max Payne 1 (Steam / GOG)",
    "notes": "Bullet Time a 60 FPS estables con apuntado fino por giroscopio.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Max%20Payne%201",
    "cache_path": "sdmc:/switch/QuiverNX/cache/max_payne_1_nx.png",
    "romfs_fallback": "romfs:/icons/max_payne_1_nx.png"
  },
  {
    "id": "star_wars_dark_forces_tfe",
    "title": "Star Wars: Dark Forces (The Force Engine - TFE)",
    "original_system": "PC",
    "category": "FPS",
    "type": "source_port",
    "project": "LuciusDXL / fgsfds",
    "github_repo": "LuciusDXL/TheForceEngine",
    "install_dir": "sdmc:/switch/tfe/",
    "executable": "tfe.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "DARK.GOB",
      "SOUNDS.GOB"
    ],
    "rom_source_hint": "Archivos de Star Wars: Dark Forces (GOG / Steam)",
    "notes": "Reemplazo moderno del motor Jedi con renderizado 3D real y resolución panorámica.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Star%20Wars%3A%20Dark%20Forces",
    "cache_path": "sdmc:/switch/QuiverNX/cache/star_wars_dark_forces_tfe.png",
    "romfs_fallback": "romfs:/icons/star_wars_dark_forces_tfe.png"
  },
  {
    "id": "star_wars_jedi_knight_openjk",
    "title": "Star Wars Jedi Knight: Jedi Outcast & Jedi Academy (OpenJK)",
    "original_system": "PC",
    "category": "Acción / Sable de Luz",
    "type": "source_port",
    "project": "JACoders / fgsfds",
    "github_repo": "JACoders/OpenJK",
    "install_dir": "sdmc:/switch/openjk/",
    "executable": "openjk.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "base/assets0.pk3",
      "base/assets1.pk3"
    ],
    "rom_source_hint": "Instalación de PC de Jedi Outcast / Jedi Academy",
    "notes": "Combates con sable de luz a 60 FPS con soporte completo de mods de la comunidad.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Star%20Wars%20Jedi%20Knight%3A%20Jedi%20Outcast%20%26%20Jedi%20Academy",
    "cache_path": "sdmc:/switch/QuiverNX/cache/star_wars_jedi_knight_openjk.png",
    "romfs_fallback": "romfs:/icons/star_wars_jedi_knight_openjk.png"
  },
  {
    "id": "fallout_1_ce",
    "title": "Fallout 1 (Fallout-CE Switch)",
    "original_system": "PC",
    "category": "RPG Isométrico",
    "type": "source_port",
    "project": "alexbatalov / ryandeering",
    "github_repo": "ryandeering/fallout-ce-switch",
    "install_dir": "sdmc:/switch/fallout1/",
    "executable": "fallout-ce.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "master.dat",
      "critter.dat",
      "data/"
    ],
    "rom_source_hint": "Fallout 1 (GOG / Steam)",
    "notes": "Soporte táctil nativo completo y compatibilidad de partidas guardadas con PC.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Fallout%201",
    "cache_path": "sdmc:/switch/QuiverNX/cache/fallout_1_ce.png",
    "romfs_fallback": "romfs:/icons/fallout_1_ce.png"
  },
  {
    "id": "fallout_2_ce",
    "title": "Fallout 2 (Fallout 2-CE Switch)",
    "original_system": "PC",
    "category": "RPG Isométrico",
    "type": "source_port",
    "project": "alexbatalov / ryandeering",
    "github_repo": "ryandeering/fallout2-ce-switch",
    "install_dir": "sdmc:/switch/fallout2/",
    "executable": "fallout2-ce.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "master.dat",
      "critter.dat",
      "data/"
    ],
    "rom_source_hint": "Fallout 2 (GOG / Steam)",
    "notes": "Misma arquitectura Community Edition que Fallout 1, optimizado para Joy-Con.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Fallout%202",
    "cache_path": "sdmc:/switch/QuiverNX/cache/fallout_2_ce.png",
    "romfs_fallback": "romfs:/icons/fallout_2_ce.png"
  },
  {
    "id": "diablo_1_devilutionx",
    "title": "Diablo 1 & Hellfire (DevilutionX)",
    "original_system": "PC",
    "category": "Action RPG",
    "type": "source_port",
    "project": "diasurgical",
    "github_repo": "diasurgical/devilutionX",
    "install_dir": "sdmc:/switch/devilutionx/",
    "executable": "devilutionx.nro",
    "asset_type": "pc_file_copy",
    "required_files": [
      "DIABDAT.MPQ"
    ],
    "rom_source_hint": "CD original o versión GOG de Diablo 1 (+ hellfire.mpq opcional)",
    "notes": "Multijugador local/online, soporte táctil, traducción al español y 60 FPS fluidos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Diablo%201%20%26%20Hellfire",
    "cache_path": "sdmc:/switch/QuiverNX/cache/diablo_1_devilutionx.png",
    "romfs_fallback": "romfs:/icons/diablo_1_devilutionx.png"
  },
  {
    "id": "tes_daggerfall_unity",
    "title": "The Elder Scrolls II: Daggerfall (Daggerfall Unity)",
    "original_system": "PC",
    "category": "RPG en 1ª Persona",
    "type": "source_port",
    "project": "Interkarma / ChanseyIsTheBest",
    "github_repo": "ChanseyIsTheBest/daggerfall_nx",
    "install_dir": "sdmc:/switch/daggerfall/",
    "executable": "daggerfall.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "arena2/"
    ],
    "rom_source_hint": "Daggerfall gratis de Bethesda o GOG",
    "notes": "Recreación del motor en Unity con controles analógicos modernos y distancia visual épica.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/The%20Elder%20Scrolls%20II%3A%20Daggerfall",
    "cache_path": "sdmc:/switch/QuiverNX/cache/tes_daggerfall_unity.png",
    "romfs_fallback": "romfs:/icons/tes_daggerfall_unity.png"
  },
  {
    "id": "tes_morrowind_openmw",
    "title": "The Elder Scrolls III: Morrowind (OpenMW-Switch)",
    "original_system": "PC",
    "category": "RPG en 1ª Persona",
    "type": "source_port",
    "project": "OpenMW Team",
    "github_repo": "OpenMW/openmw",
    "install_dir": "sdmc:/switch/openmw/",
    "executable": "openmw.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "Data Files/Morrowind.esm",
      "Data Files/Morrowind.bsa"
    ],
    "rom_source_hint": "Morrowind Game of the Year Edition (Steam / GOG)",
    "notes": "Motor C++ libre que reemplaza el motor de Bethesda. Soporta mods y expansiones Bloodmoon/Tribunal.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/The%20Elder%20Scrolls%20III%3A%20Morrowind",
    "cache_path": "sdmc:/switch/QuiverNX/cache/tes_morrowind_openmw.png",
    "romfs_fallback": "romfs:/icons/tes_morrowind_openmw.png"
  },
  {
    "id": "rct2_openrct2",
    "title": "RollerCoaster Tycoon 1 & 2 (OpenRCT2)",
    "original_system": "PC",
    "category": "Simulación / Gestión",
    "type": "source_port",
    "project": "OpenRCT2 Team",
    "github_repo": "OpenRCT2/OpenRCT2",
    "install_dir": "sdmc:/switch/openrct2/",
    "executable": "openrct2.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "Data/",
      "ObjData/",
      "Scenarios/"
    ],
    "rom_source_hint": "RollerCoaster Tycoon 2 Triple Thrill Pack (GOG / Steam)",
    "notes": "Soporte táctil preciso y cursor analógico virtual; añade multijugador online.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/RollerCoaster%20Tycoon%201%20%26%202",
    "cache_path": "sdmc:/switch/QuiverNX/cache/rct2_openrct2.png",
    "romfs_fallback": "romfs:/icons/rct2_openrct2.png"
  },
  {
    "id": "transport_tycoon_openttd",
    "title": "Transport Tycoon Deluxe (OpenTTD)",
    "original_system": "PC",
    "category": "Simulación / Gestión",
    "type": "source_port",
    "project": "OpenTTD Team",
    "github_repo": "OpenTTD/OpenTTD",
    "install_dir": "sdmc:/switch/openttd/",
    "executable": "openttd.nro",
    "asset_type": "standalone_or_assets",
    "required_files": [
      "baseset/"
    ],
    "rom_source_hint": "Assets libres OpenGFX o archivos originales de TTD",
    "notes": "Funciona con assets abiertos incluidos o con los clásicos de DOS/Windows.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Transport%20Tycoon%20Deluxe",
    "cache_path": "sdmc:/switch/QuiverNX/cache/transport_tycoon_openttd.png",
    "romfs_fallback": "romfs:/icons/transport_tycoon_openttd.png"
  },
  {
    "id": "caesar_3_julius_augustus",
    "title": "Caesar III (Julius / Augustus)",
    "original_system": "PC",
    "category": "Estrategia / Ciudad",
    "type": "source_port",
    "project": "bvschaik",
    "github_repo": "bvschaik/julius",
    "install_dir": "sdmc:/switch/julius/",
    "executable": "julius.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "c3.dat",
      "c3.map"
    ],
    "rom_source_hint": "Instalación de Caesar III (GOG / Steam)",
    "notes": "Soporte panorámico para resoluciones de Switch, controles táctiles y atajos de construcción.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Caesar%20III",
    "cache_path": "sdmc:/switch/QuiverNX/cache/caesar_3_julius_augustus.png",
    "romfs_fallback": "romfs:/icons/caesar_3_julius_augustus.png"
  },
  {
    "id": "theme_hospital_corsixth",
    "title": "Theme Hospital (CorsixTH)",
    "original_system": "PC",
    "category": "Simulación / Gestión",
    "type": "source_port",
    "project": "CorsixTH Team",
    "github_repo": "CorsixTH/CorsixTH",
    "install_dir": "sdmc:/switch/corsixth/",
    "executable": "corsixth.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "HOSP/"
    ],
    "rom_source_hint": "Theme Hospital (GOG / Origin)",
    "notes": "Reimplementación moderna en C++/Lua con pantalla en alta resolución y soporte de ratón/táctil.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Theme%20Hospital",
    "cache_path": "sdmc:/switch/QuiverNX/cache/theme_hospital_corsixth.png",
    "romfs_fallback": "romfs:/icons/theme_hospital_corsixth.png"
  },
  {
    "id": "command_and_conquer_vanilla",
    "title": "Command & Conquer: Tiberian Dawn & Red Alert (Vanilla-Conquer)",
    "original_system": "PC",
    "category": "Estrategia en Tiempo Real (RTS)",
    "type": "source_port",
    "project": "TheAssemblyProject / capsterx-switch",
    "github_repo": "capsterx-switch/Vanilla-Conquer",
    "install_dir": "sdmc:/switch/vanillaconquer/",
    "executable": "vanillaconquer.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "*.MIX"
    ],
    "rom_source_hint": "Discos originales o C&C Remastered Collection",
    "notes": "Motor multiplataforma limpio; soporte para pantalla táctil y ratón USB.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Command%20%26%20Conquer%3A%20Tiberian%20Dawn%20%26%20Red%20Alert",
    "cache_path": "sdmc:/switch/QuiverNX/cache/command_and_conquer_vanilla.png",
    "romfs_fallback": "romfs:/icons/command_and_conquer_vanilla.png"
  },
  {
    "id": "dune_2_opendune",
    "title": "Dune II: Battle for Arrakis (OpenDUNE)",
    "original_system": "PC",
    "category": "Estrategia en Tiempo Real (RTS)",
    "type": "source_port",
    "project": "OpenDUNE Team",
    "github_repo": "OpenDUNE/OpenDUNE",
    "install_dir": "sdmc:/switch/opendune/",
    "executable": "opendune.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "DUNE.PAK",
      "SCENARIO.PAK"
    ],
    "rom_source_hint": "Archivos originales de Dune II de DOS",
    "notes": "Recreación exacta del primer RTS moderno con interfaz adaptada.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Dune%20II%3A%20Battle%20for%20Arrakis",
    "cache_path": "sdmc:/switch/QuiverNX/cache/dune_2_opendune.png",
    "romfs_fallback": "romfs:/icons/dune_2_opendune.png"
  },
  {
    "id": "heroes_of_might_magic_2_fheroes2",
    "title": "Heroes of Might and Magic II (fheroes2)",
    "original_system": "PC",
    "category": "Estrategia por Turnos",
    "type": "source_port",
    "project": "ihhub",
    "github_repo": "ihhub/fheroes2",
    "install_dir": "sdmc:/switch/fheroes2/",
    "executable": "fheroes2.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "DATA/",
      "MAPS/"
    ],
    "rom_source_hint": "Heroes of Might and Magic II (GOG)",
    "notes": "Motor C++ reescrito con mejoras de calidad de vida épicas, resoluciones HD y controles Joy-Con.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Heroes%20of%20Might%20and%20Magic%20II",
    "cache_path": "sdmc:/switch/QuiverNX/cache/heroes_of_might_magic_2_fheroes2.png",
    "romfs_fallback": "romfs:/icons/heroes_of_might_magic_2_fheroes2.png"
  },
  {
    "id": "heroes_of_might_magic_3_vcmi",
    "title": "Heroes of Might and Magic III (VCMI)",
    "original_system": "PC",
    "category": "Estrategia por Turnos",
    "type": "source_port",
    "project": "vcmi team",
    "github_repo": "vcmi/vcmi",
    "install_dir": "sdmc:/switch/vcmi/",
    "executable": "vcmi.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "Data/",
      "Maps/",
      "Mp3/"
    ],
    "rom_source_hint": "HoMM 3 Complete (GOG)",
    "notes": "Soporta pantallas panorámicas, la expansión Horn of the Abyss y mods de comunidad.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Heroes%20of%20Might%20and%20Magic%20III",
    "cache_path": "sdmc:/switch/QuiverNX/cache/heroes_of_might_magic_3_vcmi.png",
    "romfs_fallback": "romfs:/icons/heroes_of_might_magic_3_vcmi.png"
  },
  {
    "id": "chris_sawyers_locomotion_openloco",
    "title": "Chris Sawyer's Locomotion (OpenLoco)",
    "original_system": "PC",
    "category": "Simulación / Transporte",
    "type": "source_port",
    "project": "OpenLoco Team",
    "github_repo": "OpenLoco/OpenLoco",
    "install_dir": "sdmc:/switch/openloco/",
    "executable": "openloco.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "Data/",
      "ObjData/"
    ],
    "rom_source_hint": "Chris Sawyer's Locomotion (Steam / GOG)",
    "notes": "Descompilación inversa del sucesor espiritual de Transport Tycoon.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Chris%20Sawyer%27s%20Locomotion",
    "cache_path": "sdmc:/switch/QuiverNX/cache/chris_sawyers_locomotion_openloco.png",
    "romfs_fallback": "romfs:/icons/chris_sawyers_locomotion_openloco.png"
  },
  {
    "id": "revolt_rvgl",
    "title": "Re-Volt (RVGL-NX)",
    "original_system": "PC",
    "category": "Carreras RC",
    "type": "source_port",
    "project": "Huki / ChanseyIsTheBest",
    "github_repo": "ChanseyIsTheBest/revolt_nx",
    "install_dir": "sdmc:/switch/rvgl/",
    "executable": "rvgl.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "cars/",
      "levels/",
      "sound/"
    ],
    "rom_source_hint": "Instalación de PC de Re-Volt",
    "notes": "Motor moderno multiplataforma con físicas pulidas, 60 FPS y shaders modernos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Re-Volt",
    "cache_path": "sdmc:/switch/QuiverNX/cache/revolt_rvgl.png",
    "romfs_fallback": "romfs:/icons/revolt_rvgl.png"
  },
  {
    "id": "oddworld_relive",
    "title": "Oddworld: Abe's Oddysee & Exoddus (R.E.L.I.V.E.)",
    "original_system": "PC / PS1",
    "category": "Plataformas Cinemático",
    "type": "decomp",
    "project": "AliveTeam",
    "github_repo": "AliveTeam/alive_reversing",
    "install_dir": "sdmc:/switch/relive/",
    "executable": "relive.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "*.lvl",
      "*.cam"
    ],
    "rom_source_hint": "Oddworld: Abe's Oddysee / Exoddus (Steam / GOG)",
    "notes": "Motor C++ reescrito que elimina los bugs del ejecutable antiguo de Windows y escala perfecto a 720p/1080p.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Oddworld%3A%20Abe%27s%20Oddysee%20%26%20Exoddus",
    "cache_path": "sdmc:/switch/QuiverNX/cache/oddworld_relive.png",
    "romfs_fallback": "romfs:/icons/oddworld_relive.png"
  },
  {
    "id": "gta_3_re3",
    "title": "Grand Theft Auto III (re3)",
    "original_system": "PC / PS2",
    "category": "Acción / Sandbox",
    "type": "decomp",
    "project": "GTAmodding / mariob0y",
    "github_repo": "mariob0y/gta3-nx",
    "install_dir": "sdmc:/switch/re3/",
    "executable": "re3.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "audio/",
      "models/",
      "data/",
      "txd/"
    ],
    "rom_source_hint": "Instalación de PC de GTA III",
    "notes": "Ingeniería inversa de RenderWare. 60 FPS, widescreen y tiempos de carga nulos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Grand%20Theft%20Auto%20III",
    "cache_path": "sdmc:/switch/QuiverNX/cache/gta_3_re3.png",
    "romfs_fallback": "romfs:/icons/gta_3_re3.png"
  },
  {
    "id": "gta_vc_revc",
    "title": "Grand Theft Auto: Vice City (reVC)",
    "original_system": "PC / PS2",
    "category": "Acción / Sandbox",
    "type": "decomp",
    "project": "GTAmodding / mariob0y",
    "github_repo": "mariob0y/reVC-nx",
    "install_dir": "sdmc:/switch/reVC/",
    "executable": "reVC.nro",
    "asset_type": "pc_folder_copy",
    "required_files": [
      "audio/",
      "models/",
      "data/"
    ],
    "rom_source_hint": "Instalación de PC de GTA Vice City",
    "notes": "Misma tecnología re3, con soporte para todas las estaciones de radio y mapeo moderno.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Grand%20Theft%20Auto%3A%20Vice%20City",
    "cache_path": "sdmc:/switch/QuiverNX/cache/gta_vc_revc.png",
    "romfs_fallback": "romfs:/icons/gta_vc_revc.png"
  },
  {
    "id": "gta_sa_nx",
    "title": "Grand Theft Auto: San Andreas (gtasa_nx)",
    "original_system": "Android / PC",
    "category": "Acción / Sandbox",
    "type": "native_wrapper",
    "project": "NaGaa95 / Graber",
    "github_repo": "NaGaa95/gtasa_nx",
    "install_dir": "sdmc:/switch/gtasa/",
    "executable": "gtasa.nro",
    "asset_type": "apk_extract",
    "required_files": [
      "libGTASA.so",
      "data/"
    ],
    "rom_source_hint": "APK y datos OBB oficiales de GTA San Andreas Android",
    "notes": "Traducción ARM64 directa a Horizon OS con mejor rendimiento que la Trilogy oficial.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Grand%20Theft%20Auto%3A%20San%20Andreas",
    "cache_path": "sdmc:/switch/QuiverNX/cache/gta_sa_nx.png",
    "romfs_fallback": "romfs:/icons/gta_sa_nx.png"
  },
  {
    "id": "sonic_1_rsdk",
    "title": "Sonic the Hedgehog 1 (Retro Engine)",
    "original_system": "Sega Mega Drive / Móvil",
    "category": "Plataformas",
    "type": "decomp",
    "project": "Rubberduckycooly",
    "github_repo": "Rubberduckycooly/Sonic-1-2-2013-Decompilation",
    "install_dir": "sdmc:/switch/sonic1/",
    "executable": "sonic1.nro",
    "asset_type": "file_copy",
    "required_files": [
      "Data.rsdk"
    ],
    "rom_source_hint": "Archivo Data.rsdk de la versión móvil o remake",
    "notes": "60 FPS a 16:9 con Spin Dash y Tails/Knuckles jugables.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/master/Named_Boxarts/Sonic%20The%20Hedgehog%20%28USA%2C%20Europe%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_1_rsdk.png",
    "romfs_fallback": "romfs:/icons/sonic_1_rsdk.png"
  },
  {
    "id": "sonic_2_rsdk",
    "title": "Sonic the Hedgehog 2 (Retro Engine)",
    "original_system": "Sega Mega Drive / Móvil",
    "category": "Plataformas",
    "type": "decomp",
    "project": "Rubberduckycooly",
    "github_repo": "Rubberduckycooly/Sonic-1-2-2013-Decompilation",
    "install_dir": "sdmc:/switch/sonic2/",
    "executable": "sonic2.nro",
    "asset_type": "file_copy",
    "required_files": [
      "Data.rsdk"
    ],
    "rom_source_hint": "Archivo Data.rsdk de Sonic 2 (2013)",
    "notes": "Incluye la zona oculta Hidden Palace Zone restaurada.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/master/Named_Boxarts/Sonic%20The%20Hedgehog%202%20%28World%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_2_rsdk.png",
    "romfs_fallback": "romfs:/icons/sonic_2_rsdk.png"
  },
  {
    "id": "sonic_cd_rsdk",
    "title": "Sonic CD (Retro Engine)",
    "original_system": "Sega CD / PC",
    "category": "Plataformas",
    "type": "decomp",
    "project": "Rubberduckycooly",
    "github_repo": "Rubberduckycooly/Sonic-CD-11-Decompilation",
    "install_dir": "sdmc:/switch/soniccd/",
    "executable": "soniccd.nro",
    "asset_type": "file_copy",
    "required_files": [
      "Data.rsdk"
    ],
    "rom_source_hint": "Archivo Data.rsdk de Sonic CD en Steam",
    "notes": "Elección entre bandas sonoras japonesa y americana, 60 FPS panorámico.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_CD/master/Named_Boxarts/Sonic%20CD%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_cd_rsdk.png",
    "romfs_fallback": "romfs:/icons/sonic_cd_rsdk.png"
  },
  {
    "id": "sonic_mania_rsdk",
    "title": "Sonic Mania (RSDKv5-Decompilation)",
    "original_system": "PC",
    "category": "Plataformas",
    "type": "decomp",
    "project": "Rubberduckycooly",
    "github_repo": "Rubberduckycooly/Sonic-Mania-Decompilation",
    "install_dir": "sdmc:/switch/sonicmania/",
    "executable": "sonicmania.nro",
    "asset_type": "file_copy",
    "required_files": [
      "Data.rsdk"
    ],
    "rom_source_hint": "Data.rsdk de Sonic Mania en Steam / Epic",
    "notes": "Permite cargar mods de GameBanana directamente en Switch sin alterar el juego base.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Sonic%20Mania",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_mania_rsdk.png",
    "romfs_fallback": "romfs:/icons/sonic_mania_rsdk.png"
  },
  {
    "id": "sonic_3_air",
    "title": "Sonic 3 & Knuckles (Sonic 3 A.I.R.)",
    "original_system": "Sega Mega Drive",
    "category": "Plataformas",
    "type": "remaster_native",
    "project": "Eukaryot / ultra-azu",
    "github_repo": "Eukaryot/sonic3air",
    "install_dir": "sdmc:/switch/sonic3air/",
    "executable": "sonic3air.nro",
    "asset_type": "rom_extract",
    "required_files": [
      "Sonic_Knuckles_wSonic3.bin"
    ],
    "rom_source_hint": "ROM pura de Sonic 3 & Knuckles de Mega Drive (Steam)",
    "notes": "Panorámico, animaciones fluidas, menús modernos y soporte masivo de mods.",
    "thumbnail_provider": "Libretro Thumbnails",
    "thumbnail_url": "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/master/Named_Boxarts/Sonic%20The%20Hedgehog%203%20%28USA%29.png",
    "cache_path": "sdmc:/switch/QuiverNX/cache/sonic_3_air.png",
    "romfs_fallback": "romfs:/icons/sonic_3_air.png"
  },
  {
    "id": "srb2_nx",
    "title": "Sonic Robo Blast 2 (SRB2-NX)",
    "original_system": "PC (Motor Doom)",
    "category": "Plataformas 3D",
    "type": "standalone_engine",
    "project": "Sonic Team Junior / switch port team",
    "github_repo": "carstene1ns/switch-srb2",
    "install_dir": "sdmc:/switch/srb2/",
    "executable": "srb2.nro",
    "asset_type": "included_freeware",
    "required_files": [
      "srb2.pk3"
    ],
    "rom_source_hint": "Descarga gratuita oficial de SRB2",
    "notes": "Sonic Robo Blast 2; motor Doom modificado. Pendiente release de paquete NRO actualizado compatible con firmware reciente.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Sonic%20Robo%20Blast%202",
    "cache_path": "sdmc:/switch/QuiverNX/cache/srb2_nx.png",
    "romfs_fallback": "romfs:/icons/srb2_nx.png"
  },
  {
    "id": "dr_robotnik_ring_racers",
    "title": "Dr. Robotnik's Ring Racers (RingRacers-NX)",
    "original_system": "PC (Motor Doom)",
    "category": "Carreras Kart",
    "type": "standalone_engine",
    "project": "Kart Krew / switch porter",
    "github_repo": "switch-ports/ringracers-nx",
    "install_dir": "sdmc:/switch/ringracers/",
    "executable": "ringracers.nro",
    "asset_type": "included_freeware",
    "required_files": [
      "ringracers.pk3"
    ],
    "rom_source_hint": "Descarga gratuita oficial de Ring Racers",
    "notes": "Secuela masiva de SRB2Kart con cientos de pistas y juego multijugador local.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Dr.%20Robotnik%27s%20Ring%20Racers",
    "cache_path": "sdmc:/switch/QuiverNX/cache/dr_robotnik_ring_racers.png",
    "romfs_fallback": "romfs:/icons/dr_robotnik_ring_racers.png"
  },
  {
    "id": "another_world_raw",
    "title": "Another World / Out of This World (Raw-NX)",
    "original_system": "Amiga / DOS",
    "category": "Aventura Cinemática",
    "type": "recreation",
    "project": "cyxx / fgsfds",
    "github_repo": "fgsfdsfgs/raw",
    "install_dir": "sdmc:/switch/raw/",
    "executable": "raw.nro",
    "asset_type": "game_data",
    "required_files": [
      "banks/"
    ],
    "rom_source_hint": "Versión DOS del clásico de Éric Chahi de 1991",
    "notes": "Intérprete perfecto de los vectores poligonales en pantalla completa.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Another%20World%20/%20Out%20of%20This%20World",
    "cache_path": "sdmc:/switch/QuiverNX/cache/another_world_raw.png",
    "romfs_fallback": "romfs:/icons/another_world_raw.png"
  },
  {
    "id": "flashback_reminiscence",
    "title": "Flashback: The Quest for Identity (REminiscence-NX)",
    "original_system": "Amiga / DOS",
    "category": "Aventura Cinemática",
    "type": "recreation",
    "project": "cyxx",
    "github_repo": "cyxx/REminiscence",
    "install_dir": "sdmc:/switch/reminiscence/",
    "executable": "reminiscence.nro",
    "asset_type": "game_data",
    "required_files": [
      "DATA/"
    ],
    "rom_source_hint": "Archivos originales de Flashback para DOS",
    "notes": "Soporta pistas de audio mejoradas y cinemáticas originales a pantalla completa.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Flashback%3A%20The%20Quest%20for%20Identity",
    "cache_path": "sdmc:/switch/QuiverNX/cache/flashback_reminiscence.png",
    "romfs_fallback": "romfs:/icons/flashback_reminiscence.png"
  },
  {
    "id": "heart_of_darkness_hode",
    "title": "Heart of Darkness (Hode-NX)",
    "original_system": "PlayStation / PC",
    "category": "Aventura Cinemática",
    "type": "recreation",
    "project": "cyxx",
    "github_repo": "cyxx/hode",
    "install_dir": "sdmc:/switch/hode/",
    "executable": "hode.nro",
    "asset_type": "game_data",
    "required_files": [
      "hod.pck",
      "*.lvl"
    ],
    "rom_source_hint": "Archivos de datos de Heart of Darkness de PC",
    "notes": "Reproducción fluida de cinemáticas y guardado automático por puntos de control.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Heart%20of%20Darkness",
    "cache_path": "sdmc:/switch/QuiverNX/cache/heart_of_darkness_hode.png",
    "romfs_fallback": "romfs:/icons/heart_of_darkness_hode.png"
  },
  {
    "id": "bermuda_syndrome_bs",
    "title": "Bermuda Syndrome (Bermuda-NX)",
    "original_system": "PC",
    "category": "Aventura Cinemática",
    "type": "recreation",
    "project": "cyxx",
    "github_repo": "cyxx/bstone",
    "install_dir": "sdmc:/switch/bermuda/",
    "executable": "bermuda.nro",
    "asset_type": "game_data",
    "required_files": [
      "BERMUDA/"
    ],
    "rom_source_hint": "Bermuda Syndrome de PC",
    "notes": "Aventura clásica estilo Flashback ambientada en la era jurásica.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Bermuda%20Syndrome",
    "cache_path": "sdmc:/switch/QuiverNX/cache/bermuda_syndrome_bs.png",
    "romfs_fallback": "romfs:/icons/bermuda_syndrome_bs.png"
  },
  {
    "id": "fade_to_black_f2b",
    "title": "Fade to Black (f2b-nx)",
    "original_system": "PC / PS1",
    "category": "Acción 3D",
    "type": "recreation",
    "project": "cyxx",
    "github_repo": "cyxx/f2b",
    "install_dir": "sdmc:/switch/f2b/",
    "executable": "f2b.nro",
    "asset_type": "game_data",
    "required_files": [
      "DATA/"
    ],
    "rom_source_hint": "Secuela 3D de Flashback para DOS",
    "notes": "Controles modernos con stick analógico para el movimiento en tercera persona.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Fade%20to%20Black",
    "cache_path": "sdmc:/switch/QuiverNX/cache/fade_to_black_f2b.png",
    "romfs_fallback": "romfs:/icons/fade_to_black_f2b.png"
  },
  {
    "id": "little_big_adventure_twine",
    "title": "Little Big Adventure 1 & 2 (Twin-E NX)",
    "original_system": "PC",
    "category": "Aventura Isométrica",
    "type": "source_port",
    "project": "Twin-E Team",
    "github_repo": "Twin-E/Twin-E",
    "install_dir": "sdmc:/switch/twine/",
    "executable": "twine.nro",
    "asset_type": "game_data",
    "required_files": [
      "LBA/"
    ],
    "rom_source_hint": "LBA 1 / LBA 2 (Twinsen's Odyssey) de GOG/Steam",
    "notes": "Motor C++ que unifica ambas aventuras de Twinsen con soporte para mandos modernos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Little%20Big%20Adventure%201%20%26%202",
    "cache_path": "sdmc:/switch/QuiverNX/cache/little_big_adventure_twine.png",
    "romfs_fallback": "romfs:/icons/little_big_adventure_twine.png"
  },
  {
    "id": "jazz_jackrabbit_openjazz",
    "title": "Jazz Jackrabbit (OpenJazzNX)",
    "original_system": "PC (DOS)",
    "category": "Plataformas",
    "type": "recreation",
    "project": "AlisterT / KranKRival",
    "github_repo": "KranKRival/OpenJazzNX",
    "install_dir": "sdmc:/switch/openjazz/",
    "executable": "openjazz.nro",
    "asset_type": "game_data",
    "required_files": [
      "*.000",
      "*.001",
      "*.002"
    ],
    "rom_source_hint": "Jazz Jackrabbit 1 (Shareware / Completo)",
    "notes": "Pantalla completa panorámica, 60 FPS estables y soporte para todos los episodios.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Jazz%20Jackrabbit",
    "cache_path": "sdmc:/switch/QuiverNX/cache/jazz_jackrabbit_openjazz.png",
    "romfs_fallback": "romfs:/icons/jazz_jackrabbit_openjazz.png"
  },
  {
    "id": "captain_claw_openclaw",
    "title": "Captain Claw (OpenClaw-NX)",
    "original_system": "PC",
    "category": "Plataformas",
    "type": "recreation",
    "project": "pjasicek / switch porter",
    "github_repo": "pjasicek/OpenClaw",
    "install_dir": "sdmc:/switch/openclaw/",
    "executable": "openclaw.nro",
    "asset_type": "game_data",
    "required_files": [
      "CLAW.REZ"
    ],
    "rom_source_hint": "Captain Claw de PC (1997)",
    "notes": "Resolución nativa HD y atajos para las armas secundarias en botones L/R.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Captain%20Claw",
    "cache_path": "sdmc:/switch/QuiverNX/cache/captain_claw_openclaw.png",
    "romfs_fallback": "romfs:/icons/captain_claw_openclaw.png"
  },
  {
    "id": "abuse_nx",
    "title": "Abuse (abuse-nx)",
    "original_system": "PC",
    "category": "Acción / Run & Gun",
    "type": "source_port",
    "project": "fgsfds",
    "github_repo": "fgsfdsfgs/abuse",
    "install_dir": "sdmc:/switch/abuse/",
    "executable": "abuse.nro",
    "asset_type": "game_data",
    "required_files": [
      "abuse.dat"
    ],
    "rom_source_hint": "Abuse de PC (Shareware o Registrado)",
    "notes": "Apuntado de 360 grados asignado al stick analógico derecho o pantalla táctil.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Abuse",
    "cache_path": "sdmc:/switch/QuiverNX/cache/abuse_nx.png",
    "romfs_fallback": "romfs:/icons/abuse_nx.png"
  },
  {
    "id": "commander_keen_cgenius",
    "title": "Commander Keen Episodios 1-6 (Commander Genius)",
    "original_system": "PC (DOS)",
    "category": "Plataformas",
    "type": "recreation",
    "project": "Gerstrong",
    "github_repo": "gerstrong/cgenius",
    "install_dir": "sdmc:/switch/cgenius/",
    "executable": "cgenius.nro",
    "asset_type": "game_data",
    "required_files": [
      "games/keen/"
    ],
    "rom_source_hint": "Episodios 1 a 6 de Commander Keen (id Software)",
    "notes": "Filtros gráficos, música HQ opcional y guardado rápido.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Commander%20Keen%20Episodios%201-6",
    "cache_path": "sdmc:/switch/QuiverNX/cache/commander_keen_cgenius.png",
    "romfs_fallback": "romfs:/icons/commander_keen_cgenius.png"
  },
  {
    "id": "tyrian_2000_opentyrian",
    "title": "Tyrian 2000 (OpenTyrian-NX)",
    "original_system": "PC (DOS)",
    "category": "Shoot 'em up",
    "type": "source_port",
    "project": "opentyrian team",
    "github_repo": "opentyrian/opentyrian",
    "install_dir": "sdmc:/switch/opentyrian/",
    "executable": "opentyrian.nro",
    "asset_type": "included_freeware",
    "required_files": [
      "tyrian21/"
    ],
    "rom_source_hint": "Assets gratuitos de Tyrian 2000 (Freeware)",
    "notes": "Soporta modo vertical (TATE) girando la consola Nintendo Switch.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Tyrian%202000",
    "cache_path": "sdmc:/switch/QuiverNX/cache/tyrian_2000_opentyrian.png",
    "romfs_fallback": "romfs:/icons/tyrian_2000_opentyrian.png"
  },
  {
    "id": "aquaria_nx",
    "title": "Aquaria (Aquaria-NX)",
    "original_system": "PC",
    "category": "Metroidvania",
    "type": "source_port",
    "project": "Bit-Blot / switch porter",
    "github_repo": "AquariaOS/Aquaria",
    "install_dir": "sdmc:/switch/aquaria/",
    "executable": "aquaria.nro",
    "asset_type": "game_data",
    "required_files": [
      "data/"
    ],
    "rom_source_hint": "Aquaria (Steam / GOG / Humble)",
    "notes": "Controles de nado fluidos con stick analógico y 60 FPS estables.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Aquaria",
    "cache_path": "sdmc:/switch/QuiverNX/cache/aquaria_nx.png",
    "romfs_fallback": "romfs:/icons/aquaria_nx.png"
  },
  {
    "id": "celeste_64_nx",
    "title": "Celeste 64: Fragments of the Mountain",
    "original_system": "PC",
    "category": "Plataformas 3D",
    "type": "source_port",
    "project": "Extremely OK Games / switch-celeste64",
    "github_repo": "Mupen64Plus/celeste64-switch",
    "install_dir": "sdmc:/switch/celeste64/",
    "executable": "celeste64.nro",
    "asset_type": "included_freeware",
    "required_files": [
      "Content/"
    ],
    "rom_source_hint": "Versión oficial gratuita de Celeste 64 (Itch.io)",
    "notes": "Juego 3D completo desarrollado por el equipo original de Celeste, port nativo C# / C++.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Celeste%2064%3A%20Fragments%20of%20the%20Mountain",
    "cache_path": "sdmc:/switch/QuiverNX/cache/celeste_64_nx.png",
    "romfs_fallback": "romfs:/icons/celeste_64_nx.png"
  },
  {
    "id": "cave_story_nxengine",
    "title": "Cave Story (NXEngine-evo)",
    "original_system": "PC",
    "category": "Metroidvania",
    "type": "recreation",
    "project": "nxengine-evo team",
    "github_repo": "nxengine/nxengine-evo",
    "install_dir": "sdmc:/switch/nxengine/",
    "executable": "nxengine.nro",
    "asset_type": "game_data",
    "required_files": [
      "data/"
    ],
    "rom_source_hint": "Doukutsu Monogatari versión PC freeware",
    "notes": "Permite alternar entre música original, remasterizada o de CS+.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Cave%20Story",
    "cache_path": "sdmc:/switch/QuiverNX/cache/cave_story_nxengine.png",
    "romfs_fallback": "romfs:/icons/cave_story_nxengine.png"
  },
  {
    "id": "am2r_switch",
    "title": "Another Metroid 2 Remake (AM2R)",
    "original_system": "PC",
    "category": "Metroidvania",
    "type": "native_wrapper",
    "project": "AM2R Community / switch porter",
    "github_repo": "AM2R-Community-Developers/AM2R-Autopatcher",
    "install_dir": "sdmc:/switch/am2r/",
    "executable": "am2r.nro",
    "asset_type": "game_data",
    "required_files": [
      "game.droid",
      "audiogroup1.dat"
    ],
    "rom_source_hint": "Instalación de PC parcheada a la versión comunitaria 1.5.x",
    "notes": "El mejor remake de Metroid II corriendo de forma nativa a 60 FPS.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/Another%20Metroid%202%20Remake",
    "cache_path": "sdmc:/switch/QuiverNX/cache/am2r_switch.png",
    "romfs_fallback": "romfs:/icons/am2r_switch.png"
  },
  {
    "id": "space_cadet_pinball_nx",
    "title": "3D Pinball for Windows - Space Cadet",
    "original_system": "Windows NT/95",
    "category": "Pinball",
    "type": "decomp",
    "project": "k4zmu2a",
    "github_repo": "k4zmu2a/SpaceCadetPinball",
    "install_dir": "sdmc:/switch/pinball/",
    "executable": "pinball.nro",
    "asset_type": "included_or_windows",
    "required_files": [
      "PINBALL.DAT",
      "sound/"
    ],
    "rom_source_hint": "Archivos originales de Windows 95/98/ME/2000/XP",
    "notes": "Descompilación inversa C++ con vibración háptica HD Rumble en los flippers.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/3D%20Pinball%20for%20Windows%20-%20Space%20Cadet",
    "cache_path": "sdmc:/switch/QuiverNX/cache/space_cadet_pinball_nx.png",
    "romfs_fallback": "romfs:/icons/space_cadet_pinball_nx.png"
  },
  {
    "id": "vvvvvv_nx",
    "title": "VVVVVV (Make and Play)",
    "original_system": "PC",
    "category": "Plataformas",
    "type": "decomp_source",
    "project": "Terry Cavanagh",
    "github_repo": "TerryCavanagh/VVVVVV",
    "install_dir": "sdmc:/switch/vvvvvv/",
    "executable": "vvvvvv.nro",
    "asset_type": "file_copy",
    "required_files": [
      "data.zip"
    ],
    "rom_source_hint": "Archivo data.zip de la versión comercial de PC (Steam/GOG)",
    "notes": "Código fuente oficial liberado en C++. Cero input lag y 60 FPS fijos.",
    "thumbnail_provider": "SteamGridDB",
    "thumbnail_url": "https://www.steamgriddb.com/api/v2/search/autocomplete/VVVVVV",
    "cache_path": "sdmc:/switch/QuiverNX/cache/vvvvvv_nx.png",
    "romfs_fallback": "romfs:/icons/vvvvvv_nx.png"
  }
]
```
