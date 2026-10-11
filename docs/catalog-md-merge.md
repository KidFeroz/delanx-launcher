# Fusión del catálogo maestro

Generado por `tools/import_ports_md.py` a partir de `switch_ports_catalog.md`.

La salida (`romfs/catalog/ports-master.json`) es un catálogo **aparte** del verificado:
todas sus entradas llevan `verified: false` y una nota que recuerda que falta comprobar
que cada proyecto publique un asset `.nro` para Switch. Las entradas ya verificadas en
`romfs/catalog/switch-catalog.json` **no se duplican** y **no se tocan**: en caso de
discrepancia manda Delanx.

| Métrica | Valor |
|---|---|
| Entradas del catálogo maestro | 110 |
| Con `github_repo` | 110 |
| Sin `github_repo` (no instalables) | 0 |
| Ya presentes en Delanx (no se duplican) | 8 |
| Añadidas a `ports-master.json` (`verified: false`) | 102 |
| Conflictos de ruta/ejecutable (manda Delanx) | 3 |
| Total resultante (verificadas + nuevas) | 148 |

Criterios aplicados:

- Emparejamiento con Delanx **solo por `repository`** (sin distinguir mayúsculas).
- `switch.homeDir` = carpeta final de la app (`install_dir` del maestro).
- `switch.targetDir` = raíz de extracción; sin verificar el asset se usa la carpeta
  final (correcto cuando el zip no trae el prefijo `switch/`) y queda anotado en `notes`.
- `appIconUrl` = `thumbnail_url` del maestro: 44 PNG de Libretro Thumbnails
  (`raw.githubusercontent.com`, descargable tal cual) y 60 URL de API de SteamGridDB
  (`www.steamgriddb.com/api/v2/...`), que **no son imágenes** y necesitan API key: esas
  60 carátulas fallarán hasta sustituir la URL por un enlace directo al PNG.

## Entradas añadidas

| Entrada del maestro | Repositorio y carpeta final |
|---|---|
| The Legend of Zelda: Majora's Mask (2Ship2Harkinian) | `HarbourMasters/2ship2harkinian` → `/switch/2ship2harkinian` |
| Paper Mario 64 (PaperBoat) | `HarbourMasters/PaperBoat` → `/switch/PaperBoat` |
| Banjo-Kazooie (Lighthouse / BanjoRecomp) | `HarbourMasters/Lighthouse` → `/switch/Lighthouse` |
| F-Zero X (NX-Diffuser / G-Diffuser) | `PalindromicBreadLoaf/NX-Diffuser` → `/switch/nx-diffuser` |
| Perfect Dark | `perfect-dark-pc-port/perfect_dark` → `/switch/perfect_dark` |
| Doom 64 (Doom64EX-NX) | `fgsfdsfgs/doom64ex` → `/switch/doom64ex` |
| Super Smash Bros. 64 (Smash 64 Recomp) | `zestydevy/smash64r` → `/switch/smash64recomp` |
| Pokemon Stadium (SS Anne) | `HarbourMasters/SSAnne` → `/switch/ssanne` |
| Bomberman 64 (Bomberman 64: Recompiled) | `N64Recomp/Bomberman64Recomp` → `/switch/bomberman64` |
| Castlevania: Legacy of Darkness | `fliperama86/CastlevaniaLegacyOfDarkness` → `/switch/castlevania_lod` |
| Quest 64 (Quest 64 Recomp) | `N64Recomp/Quest64Recomp` → `/switch/quest64` |
| GoldenEye 007 (ge007-nx) | `n64decomp/ge007` → `/switch/ge007` |
| Jak and Daxter: The Precursor Legacy (OpenGOAL) | `fildicio/jak-project-switch` → `/switch/jak1` |
| The Simpsons: Hit & Run (simpsons-nx) | `q4a/simpsons` → `/switch/simpsons` |
| The Legend of Zelda: The Wind Waker (SwitchWaker) | `centollOS/SwitchWaker` → `/switch/SwitchWaker` |
| The Legend of Zelda: Twilight Princess (Dusklight-NX) | `souldbminerr/dusklight-nx` → `/switch/dusklight` |
| Super Smash Bros. Melee (Melee-NX) | `KawaiiBunga/Melee-NX` → `/switch/Melee-NX` |
| Super Mario Strikers (strikers-nx) | `new-coke/strikers` → `/switch/strikers` |
| Super Mario Galaxy (AstroCore) | `SMGCommunity/AstroCore` → `/switch/AstroCore` |
| Animal Crossing GameCube (ACGC-NX) | `zwaetschge/ACGC-NX` → `/switch/acgc` |
| Need For Speed: Most Wanted 2005 | `StevensND/nfsmw-nx` → `/switch/nfsmw` |
| Driver 2 (REDRIVER2) | `OpenDriver2/REDRIVER2` → `/switch/redriver2` |
| WipEout (wipeout-rewrite) | `phoboslab/wipeout-rewrite` → `/switch/wipeout` |
| Silent Hill 1 (silent-hill-decomp-nx) | `ChaykaDed/silent-hill-decomp-nx` → `/switch/silenthill` |
| Jackie Chan Stuntmaster (ReChan-Switch) | `CostelaCNX/ReChan-Switch` → `/switch/rechan` |
| Tomb Raider I & II (OpenLara) | `XProger/OpenLara` → `/switch/openlara` |
| Pokemon Rojo / Azul / Amarillo (gen1recomp) | `bryanthaboi/gen1recomp` → `/switch/gen1recomp` |
| Pokemon Oro / Plata / Cristal (Gen2Recomped) | `UNDERdecoded/Gen2Recomped` → `/switch/gen2recomp` |
| The Legend of Zelda: Link's Awakening DX HD | `delsonazevedo/Zelda-LA-DX-HD-Updated` → `/switch/zeldaladx` |
| The Legend of Zelda: Oracle of Seasons & Ages (oracles-nx) | `toniisound/oracles-nx` → `/switch/oracles` |
| The Legend of Zelda: The Minish Cap (tmc-nx) | `HayatoG/tmc` → `/switch/minishcap` |
| Sonic Advance 2 | `boraeskicioglu/sonic_advance2_nx` → `/switch/sonicadvance2` |
| The Legend of Zelda: A Link to the Past (zelda3) | `snesrev/zelda3` → `/switch/zelda3` |
| Super Metroid (sm-decomp) | `snesrev/sm` → `/switch/supermetroid` |
| Super Mario World (smw-decomp) | `snesrev/smw` → `/switch/smw` |
| Daytona USA (Daytona-NX-Modern) | `Thorhax/Daytona-NX-Modern` → `/switch/daytona` |
| Virtua Fighter 2 (VF2-NX-Modern) | `Thorhax/VF2-NX-Modern` → `/switch/vf2` |
| Cyber Troopers Virtual-On (Virtual-On-NX) | `Thorhax/Virtual-On-NX` → `/switch/virtualon` |
| Killer Instinct 1 & 2 (Killer-Instinct-NX-Modern) | `Thorhax/Killer-Instinct-NX-Modern` → `/switch/killerinstinct` |
| Sega Rally Championship (port0r) | `jacquesdupontd/port0r` → `/switch/segarally` |
| Sonic the Fighters | `boraeskicioglu/sonic_the_fighters_nx` → `/switch/sonicthefighters` |
| Doom 3 & RoE (dhewm3-nx) | `dhewm/dhewm3` → `/switch/dhewm3` |
| Quake III Arena (ioquake3-nx) | `fgsfdsfgs/ioquake3` → `/switch/ioquake3` |
| Duke Nukem 3D (EDuke32-NX) | `fgsfdsfgs/eduke32` → `/switch/eduke32` |
| Blood (NBlood-NX) | `nukeykt/nblood` → `/switch/nblood` |
| Shadow Warrior (VoidSW-NX) | `fgsfdsfgs/voidsw` → `/switch/voidsw` |
| Powerslave / Exhumed (PCExhumed-NX) | `nukeykt/pcexhumed` → `/switch/pcexhumed` |
| Redneck Rampage (Redneck-NX) | `nukeykt/redneck` → `/switch/redneck` |
| Ion Fury (Fury-NX) | `fgsfdsfgs/eduke32` → `/switch/fury` |
| Wolfenstein 3D & Spear of Destiny (ECWolf-NX) | `fgsfdsfgs/ecwolf` → `/switch/ecwolf` |
| Return to Castle Wolfenstein (iortcw-nx) | `fgsfdsfgs/iortcw` → `/switch/iortcw` |
| Descent 1 & 2 (DXX-Rebirth-NX) | `fgsfdsfgs/dxx-rebirth` → `/switch/dxx-rebirth` |
| Rise of the Triad (rott-nx) | `fgsfdsfgs/rott` → `/switch/rott` |
| Serious Sam: The First & Second Encounter | `fgsfdsfgs/Serious-Engine` → `/switch/serioussam` |
| Alien vs Predator Classic (AvP-NX) | `fgsfdsfgs/avp` → `/switch/avp` |
| Postal 1 (Postal-NX) | `fgsfdsfgs/postal` → `/switch/postal` |
| Half-Life 1, Opposing Force, Blue Shift (Xash3D-FWGS) | `FWGS/xash3d-fwgs` → `/switch/xash3d` |
| Half-Life 2 & Portal (hl2_nx) | `NaGaa95/hl2_nx` → `/switch/hl2` |
| Max Payne 1 (max_nx) | `NaGaa95/max_nx_v2.1.131` → `/switch/maxpayne` |
| Star Wars: Dark Forces (The Force Engine - TFE) | `LuciusDXL/TheForceEngine` → `/switch/tfe` |
| Star Wars Jedi Knight: Jedi Outcast & Jedi Academy (OpenJK) | `JACoders/OpenJK` → `/switch/openjk` |
| Fallout 1 (Fallout-CE Switch) | `ryandeering/fallout-ce-switch` → `/switch/fallout1` |
| Fallout 2 (Fallout 2-CE Switch) | `ryandeering/fallout2-ce-switch` → `/switch/fallout2` |
| The Elder Scrolls II: Daggerfall (Daggerfall Unity) | `ChanseyIsTheBest/daggerfall_nx` → `/switch/daggerfall` |
| The Elder Scrolls III: Morrowind (OpenMW-Switch) | `OpenMW/openmw` → `/switch/openmw` |
| RollerCoaster Tycoon 1 & 2 (OpenRCT2) | `OpenRCT2/OpenRCT2` → `/switch/openrct2` |
| Transport Tycoon Deluxe (OpenTTD) | `OpenTTD/OpenTTD` → `/switch/openttd` |
| Caesar III (Julius / Augustus) | `bvschaik/julius` → `/switch/julius` |
| Theme Hospital (CorsixTH) | `CorsixTH/CorsixTH` → `/switch/corsixth` |
| Command & Conquer: Tiberian Dawn & Red Alert (Vanilla-Conquer) | `capsterx-switch/Vanilla-Conquer` → `/switch/vanillaconquer` |
| Dune II: Battle for Arrakis (OpenDUNE) | `OpenDUNE/OpenDUNE` → `/switch/opendune` |
| Heroes of Might and Magic II (fheroes2) | `ihhub/fheroes2` → `/switch/fheroes2` |
| Heroes of Might and Magic III (VCMI) | `vcmi/vcmi` → `/switch/vcmi` |
| Chris Sawyer's Locomotion (OpenLoco) | `OpenLoco/OpenLoco` → `/switch/openloco` |
| Re-Volt (RVGL-NX) | `ChanseyIsTheBest/revolt_nx` → `/switch/rvgl` |
| Oddworld: Abe's Oddysee & Exoddus (R.E.L.I.V.E.) | `AliveTeam/alive_reversing` → `/switch/relive` |
| Grand Theft Auto III (re3) | `mariob0y/gta3-nx` → `/switch/re3` |
| Grand Theft Auto: Vice City (reVC) | `mariob0y/reVC-nx` → `/switch/reVC` |
| Grand Theft Auto: San Andreas (gtasa_nx) | `NaGaa95/gtasa_nx` → `/switch/gtasa` |
| Sonic the Hedgehog 1 (Retro Engine) | `Rubberduckycooly/Sonic-1-2-2013-Decompilation` → `/switch/sonic1` |
| Sonic the Hedgehog 2 (Retro Engine) | `Rubberduckycooly/Sonic-1-2-2013-Decompilation` → `/switch/sonic2` |
| Sonic CD (Retro Engine) | `Rubberduckycooly/Sonic-CD-11-Decompilation` → `/switch/soniccd` |
| Sonic Mania (RSDKv5-Decompilation) | `Rubberduckycooly/Sonic-Mania-Decompilation` → `/switch/sonicmania` |
| Sonic 3 & Knuckles (Sonic 3 A.I.R.) | `Eukaryot/sonic3air` → `/switch/sonic3air` |
| Sonic Robo Blast 2 (SRB2-NX) | `carstene1ns/switch-srb2` → `/switch/srb2` |
| Dr. Robotnik's Ring Racers (RingRacers-NX) | `switch-ports/ringracers-nx` → `/switch/ringracers` |
| Another World / Out of This World (Raw-NX) | `fgsfdsfgs/raw` → `/switch/raw` |
| Flashback: The Quest for Identity (REminiscence-NX) | `cyxx/REminiscence` → `/switch/reminiscence` |
| Heart of Darkness (Hode-NX) | `cyxx/hode` → `/switch/hode` |
| Bermuda Syndrome (Bermuda-NX) | `cyxx/bstone` → `/switch/bermuda` |
| Fade to Black (f2b-nx) | `cyxx/f2b` → `/switch/f2b` |
| Little Big Adventure 1 & 2 (Twin-E NX) | `Twin-E/Twin-E` → `/switch/twine` |
| Jazz Jackrabbit (OpenJazzNX) | `KranKRival/OpenJazzNX` → `/switch/openjazz` |
| Captain Claw (OpenClaw-NX) | `pjasicek/OpenClaw` → `/switch/openclaw` |
| Abuse (abuse-nx) | `fgsfdsfgs/abuse` → `/switch/abuse` |
| Commander Keen Episodios 1-6 (Commander Genius) | `gerstrong/cgenius` → `/switch/cgenius` |
| Tyrian 2000 (OpenTyrian-NX) | `opentyrian/opentyrian` → `/switch/opentyrian` |
| Aquaria (Aquaria-NX) | `AquariaOS/Aquaria` → `/switch/aquaria` |
| Celeste 64: Fragments of the Mountain | `Mupen64Plus/celeste64-switch` → `/switch/celeste64` |
| Another Metroid 2 Remake (AM2R) | `AM2R-Community-Developers/AM2R-Autopatcher` → `/switch/am2r` |
| 3D Pinball for Windows - Space Cadet | `k4zmu2a/SpaceCadetPinball` → `/switch/pinball` |
| VVVVVV (Make and Play) | `TerryCavanagh/VVVVVV` → `/switch/vvvvvv` |

## Ya verificadas en Delanx (no se duplican ni se tocan)

| Entrada del maestro | Repositorio | Entrada verificada que manda |
|---|---|---|
| Super Mario 64 (Ghostship / sm64-port) | `HarbourMasters/Ghostship` | Ghostship (Super Mario 64) |
| The Legend of Zelda: Ocarina of Time (Ship of Harkinian) | `HarbourMasters/Shipwright` | Ship of Harkinian (Ocarina of Time) |
| Diddy Kong Racing (Golden Balloon NX) | `boraeskicioglu/goldenballoon_nx` | Golden Balloon NX (Diddy Kong Racing) |
| Star Fox 64 (Starship / SF64 Recompiled) | `HarbourMasters/Starship` | Starship (Star Fox 64) |
| Mario Kart 64 (mk64-port / SpaghettiKart) | `HarbourMasters/SpaghettiKart` | SpaghettiKart (Mario Kart 64) |
| Quake II (nxquake2 / Yamagi Quake II) | `fgsfdsfgs/nxquake2` | nxquake2 (Quake II) |
| Diablo 1 & Hellfire (DevilutionX) | `diasurgical/devilutionX` | DevilutionX (Diablo) |
| Cave Story (NXEngine-evo) | `nxengine/nxengine-evo` | NXEngine-Evo (Cave Story) |

## Conflictos (se conserva lo verificado por Delanx)

| Entrada | Campo | Delanx (verificado) | Catálogo maestro |
|---|---|---|---|
| nxquake2 (Quake II) | targetDir | `/` | `/switch/nxquake2` |
| NXEngine-Evo (Cave Story) | targetDir | `/switch` | `/switch/nxengine` |
| NXEngine-Evo (Cave Story) | ejecutable | `NXEngine-Evo-v2.6.5-Switch.nro` | `nxengine.nro` |
