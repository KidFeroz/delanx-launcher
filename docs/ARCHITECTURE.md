# Arquitectura de Delanx

## 1. Hechos verificados que condicionan el diseño

| Hecho | Evidencia | Consecuencia |
|---|---|---|
| Quiver Launcher es .NET 10 + Avalonia | `QuiverLauncher.Desktop.csproj` (`net10.0`), README | No hay runtime .NET para Horizon OS: **imposible portar el código**; se reimplementa |
| Quiver no tiene lógica de Switch | 0 coincidencias de `nro`, `nsp`, `hbmenu`, `sphaira`, `homebrew`, `title id` en los 590 ficheros del repo | Nada que reutilizar a nivel de ejecución |
| Switch aparece en Quiver como metadato y como exclusión | `console=switch` en `/api/v1/facets`; `"switch"` en `PlatformAssetMatcher` como marcador *no-Windows/macOS/Linux/Android* | Delanx **invierte** ese filtro: para Switch, `switch` es el marcador **positivo** |
| Esquema `apps.json` documentado y tolerante | `SerializeApp` en `Services/AppCatalogService.cs`; el parser solo valida tipos de campos conocidos | Delanx lee/escribe el mismo fichero y añade campos extra que Quiver ignora |
| Catálogo con API pública y sin auth | `https://api.quiverlauncher.com/api/v1` (`/apps`, `/facets`, `/apps/{slug}`, `/apps/{slug}/release-history`, `/release-status`) | Delanx consume la misma API en la consola |
| Verificación por SHA-256 y estados | `CatalogReleases.ReleaseCheck` (`verified`/`unverified`/`blocked`, `checksum: "sha256:<hex>"`) | Delanx implementa la misma semántica |

## 2. Capas

```
source/
├── main.cpp              arranque: SDL2, romfs, detección applet/application, argv, bucle
├── core/                 json (jansson), strings, paths (sdmc), log, sha256, tiempo
├── model/                AppEntry, Settings, CatalogEntry, Release, Asset, DownloadJob
├── services/             http (curl), github, gitlab, quiver catalog, release check,
│                         installer (libarchive), launcher (envSetNextLoad), artwork
├── ui/                   theme, widgets (list/grid/sidebar/dialog/toast), screens
└── platform/             switch: applet type, batería, reloj, memoria, red
```

Reglas de dependencia: `ui → services → model → core`. `platform` solo lo usan `main` y `services`.

## 3. Compatibilidad de datos

### 3.1 `apps.json` (biblioteca)

Delanx escribe el mismo contrato que Quiver (`name`, `folderName`, `installPath`, `appIconUrl`,
`repository`, `repositorySource`, `project`, `customDisplayName`, `releaseAssetFilter`, `tags`,
`preferredVersion`, `skippedUpdateVersion`, `autoUpdate`, `filesToAdd`, `catalogEntryId`,
`catalog`, `mods`) **más** un bloque propio que Quiver ignora sin romperse:

```json
"switch": {
  "targetDir": "/switch/soh",
  "nro": "soh.nro",
  "nroPath": "switch/soh/soh.nro",
  "requiredUserFiles": [{ "path": "sdmc:/switch/soh/oot.otr", "present": true }]
}
```

`repositorySource` ausente ⇒ GitHub (igual que Quiver). Entrada sin `repository` ⇒ app
**manually managed** (sin descargas ni actualizaciones), igual que Quiver.

### 3.2 `settings.json`

Se conservan los nombres PascalCase y los enums numéricos de Quiver para que el fichero sea
intercambiable; Delanx añade claves propias con prefijo `Delanx*` y las ignora al leer lo que no
entiende.

### 3.3 Catálogo

- `romfs:/catalog/switch-catalog.json` — catálogo propio **verificado**: 46 proyectos con
  releases reales comprobados (asset existente, ruta interna del `.nro` y `stripRoot`
  contrastados con el contenido de los paquetes).
- `romfs:/catalog/ports-master.json` — catálogo maestro del proyecto (110 títulos), importado de
  `switch_ports_catalog.md` con `tools/import_ports_md.py`. Todas sus entradas llevan
  `verified: false` porque **no** se ha comprobado que publiquen un `.nro` para Switch; van en un
  fichero aparte precisamente para no rebajar la confianza del catálogo verificado.
- `romfs:/catalog/harbourmasters.json` — sección Harbour Masters (disponibles y no disponibles).
- API de Quiver — búsqueda online por `console=switch`, `maker:Nintendo` y `projectType`.
- `romfs:/catalog/art-map.json` — carátulas de Libretro Thumbnails (`raw.githubusercontent.com`,
  sin clave de API) que rellenan `appIconUrl` en las entradas verificadas que no traen icono
  propio. `tools/apply_art_map.py` las aplica de forma idempotente.

El catálogo propio existe porque el de Quiver **nunca** ofrece assets de Switch: su matcher
descarta deliberadamente cualquier asset con el marcador `switch`.

**Semántica de las rutas** (fuente de confusión habitual al comparar catálogos):

| Campo | Significado |
|---|---|
| `switch.targetDir` | Raíz donde se **extrae** el archivo. En un zip que ya trae `switch/<app>/...` es `/` |
| `switch.homeDir` | Carpeta **final** de la app en la SD (p. ej. `/switch/nxquake`), para mostrar y para los ficheros del usuario |
| `switch.nroPath` | Ruta del ejecutable **dentro del asset** |
| `switch.nro` | Nombre con el que debe quedar el ejecutable en la SD |

## 4. Semántica de verificación

Idéntica a Quiver, adaptada a assets de Switch:

1. Elegir release: pin del usuario → release verificado → más reciente (excluyendo `blocked`).
2. Elegir asset: `releaseAssetFilter` + marcador positivo `switch`/`nro`, descartando
   `.exe/.msi/.appimage/.apk/.dmg`.
3. Verificar SHA-256 contra el checksum del catálogo (`sha256:<hex>`) o el `digest` de GitHub.
4. `verified` instala; `unverified` pide confirmación; `blocked` pide confirmación doble.
5. Escribir `installed-build.txt` (`filename\nsha256`) y `version.txt` en la carpeta de la app.

## 5. Instalación en la SD

- Staging en `sdmc:/switch/Delanx/Temp/`; al terminar se extrae con **libarchive** a
  `switch.targetDir` (o `Apps/<folderName>`), con protección anti *path traversal*.
- Se respeta `stripRoot` para zips que envuelven todo en una carpeta.
- Nunca se sobrescriben ficheros aportados por el usuario (ROMs, `.otr`, `.o2r`, saves) sin
  confirmación explícita.

## 6. Lanzamiento

- `envSetNextLoad()` + salida del proceso: el camino que respetan hbmenu y Sphaira.
- Detección del modo de arranque (`appletGetAppletType`) para avisar de limitaciones cuando se
  ejecuta desde el Álbum (applet mode) y para adaptar la interfaz.
- El NRO destino se resuelve desde `switch.nro`/`switch.nroPath` de la entrada.

## 7. Modos de arranque soportados

| Origen | Cómo se comporta Delanx |
|---|---|
| hbmenu | Caso base; `envSetNextLoad` funciona |
| Sphaira | Idéntico a hbmenu (Sphaira respeta el encadenado) |
| Forwarder NSP | Se detecta por argv/entorno, se avisa de las limitaciones y se ofrece la ruta hbmenu |
| Álbum (applet mode) | Memoria reducida: se avisa y se activan perfiles de UI ligeros |
