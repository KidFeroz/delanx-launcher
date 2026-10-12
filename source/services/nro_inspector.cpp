// Delanx — inspeccion de ficheros NRO y separacion de Homebrew frente a Decomps.
#include "services/nro_inspector.hpp"

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "core/paths.hpp"
#include "core/strings.hpp"

namespace delanx {
namespace {

struct KnownHomebrew {
  const char* keyword;
  const char* repo;
};

// Mapeo autonomo de utilidades populares de Switch hacia sus repositorios oficiales en GitHub
// para permitir deteccion automatica y busqueda de actualizaciones.
const KnownHomebrew kKnownHomebrewRegistry[] = {
    {"appstore", "fortheusers/hb-appstore"},
    {"hb-appstore", "fortheusers/hb-appstore"},
    {"dbi", "rashevskyv/dbi"},
    {"ftpd", "mtheall/ftpd"},
    {"nx-shell", "joel16/NX-Shell"},
    {"jksv", "J-D-K/JKSV"},
    {"edizon", "WerWolv/EdiZon"},
    {"daybreak", "Atmosphere-NX/Atmosphere"},
    {"missioncontrol", "ndeadly/MissionControl"},
    {"sys-clk", "retronx-team/sys-clk"},
    {"breeze", "tomvita/Breeze-Beta"},
    {"aio-switch-updater", "HamletDuFromage/aio-switch-updater"},
    {"nxdumptool", "DarkMatterCore/nxdumptool"},
    {"simplemodmanager", "nadrino/SimpleModManager"},
    {"linkalho", "rdmrocha/linkalho"},
    {"goldleaf", "XorTroll/Goldleaf"},
    {"switch-time", "3096/switch-time"},
    {"emuiibo", "XorTroll/emuiibo"},
    {"status-monitor", "masagrator/Status-Monitor-Overlay"},
    {"nx-ovlloader", "WerWolv/nx-ovlloader"},
    {"checkpoint", "BernardoGiordano/Checkpoint"},
    {"moonlight", "XITRIX/Moonlight-Switch"},
    {"moonlight-switch", "XITRIX/Moonlight-Switch"},
};

// Firmas e identificadores de proyectos de descompilacion / ports nativos.
// Cualquier app que coincida se clasifica como DECOMP (Biblioteca/Catalogo/MrShip)
// y queda terminantemente fuera de la seccion Homebrew.
const char* const kKnownDecompSignatures[] = {
    "soh", "2ship", "starship", "ghostship", "spaghettify", "spaghettikart", "sm64", "zelda3", "smw",
    "macready", "golf", "shipwright", "keiichi", "alfa", "2harkinian", "harkinian",
    "openlara", "revolt", "fheroes2", "julius", "eduke32", "xash3d", "fallout",
    "daggerfall", "gta3", "gtasa", "openjazz", "vanilla-conquer", "redalert", "doom64ex",
    "nx-diffuser", "perfect_dark", "dusklight", "switchwaker", "tmc", "oracles",
    "daytona", "vf2", "sonic", "am2r", "spacecadetpinball", "delanx", "ssanne",
    "quest64", "bomberman64", "castlevania_lod", "smash64", "strikers", "nfsmw",
    "silent-hill", "rechan", "gen1recomp", "gen2recomp", "tmc", "hla", "hl2",
    "max_nx", "serious-engine", "blood", "voidsw", "pcexhumed", "redneck",
    "hode", "f2b", "twin-e", "openclaw", "abuse", "cgenius", "opentyrian",
    "aquaria", "celeste64", "vvvvvv", "projectz"
};

std::string sanitize_name(const std::string& input) {
  std::string out;
  for (char c : input) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      out += c;
    } else {
      out += '_';
    }
  }
  return out;
}

uint32_t read_u32(const uint8_t* buf) {
  return static_cast<uint32_t>(buf[0]) |
         (static_cast<uint32_t>(buf[1]) << 8) |
         (static_cast<uint32_t>(buf[2]) << 16) |
         (static_cast<uint32_t>(buf[3]) << 24);
}

uint64_t read_u64(const uint8_t* buf) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v |= static_cast<uint64_t>(buf[i]) << (i * 8);
  }
  return v;
}

}  // namespace

bool is_homebrew_app(const std::string& name_or_folder, const std::string& repo) {
  const std::string lower = str::lower(name_or_folder);
  const std::string lower_repo = str::lower(repo);

  static const char* const kHomebrewKeys[] = {
      "appstore", "hb-appstore", "goldleaf", "moonlight", "dbi", "ftpd",
      "nx-shell", "jksv", "edizon", "daybreak", "missioncontrol", "sys-clk",
      "breeze", "aio-switch-updater", "nxdumptool", "simplemodmanager",
      "linkalho", "switch-time", "emuiibo", "status-monitor", "nx-ovlloader",
      "checkpoint", "tinfoil", "themezer", "nxmp", "ppsspp", "retroarch",
      "nxlink", "haze", "tinwoo", "atmo-updater"
  };

  for (const char* k : kHomebrewKeys) {
    if (lower == k || str::icontains(lower, k) ||
        (!lower_repo.empty() && str::icontains(lower_repo, k))) {
      return true;
    }
  }
  return false;
}

bool is_decomp_app(const std::string& name_or_folder, const std::string& nro_file) {
  if (is_homebrew_app(name_or_folder, "")) return false;
  if (!nro_file.empty() && is_homebrew_app(paths::basename(nro_file), "")) return false;

  const std::string lower_name = str::lower(name_or_folder);
  const std::string lower_nro = str::lower(paths::basename(nro_file));

  for (const char* sig : kKnownDecompSignatures) {
    if (lower_name == sig || str::starts_with(lower_name, sig) || str::ends_with(lower_name, sig) ||
        str::icontains(lower_name, sig)) {
      return true;
    }
    if (!lower_nro.empty()) {
      if (lower_nro == sig || str::starts_with(lower_nro, sig) || str::icontains(lower_nro, sig)) {
        return true;
      }
    }
  }
  return false;
}

HomebrewAppInfo inspect_nro(const std::string& nro_path, const std::string& folder_name) {
  HomebrewAppInfo info;
  info.nro_path = nro_path;
  info.name = folder_name.empty() ? paths::basename(nro_path) : folder_name;
  if (str::iends_with(info.name, ".nro")) {
    info.name = info.name.substr(0, info.name.size() - 4);
  }
  info.folder_path = paths::dirname(nro_path);
  info.is_decomp = is_decomp_app(info.name, nro_path);

  // Buscar iconos locales primero (icon.png o <nombre>.png en la misma carpeta)
  const std::string cand_icon1 = paths::join(info.folder_path, "icon.png");
  const std::string cand_icon2 = paths::join(info.folder_path, info.name + ".png");
  if (paths::exists(cand_icon1)) {
    info.icon_path = cand_icon1;
  } else if (paths::exists(cand_icon2)) {
    info.icon_path = cand_icon2;
  }

  // Comprobar registro conocido de repositorios de GitHub
  const std::string search_key = str::lower(info.name);
  for (const auto& reg : kKnownHomebrewRegistry) {
    if (search_key == reg.keyword || str::icontains(search_key, reg.keyword)) {
      info.github_repo = reg.repo;
      break;
    }
  }

  // Inspeccionar fichero .nro para extraer NACP y ASET icon
  FILE* f = fopen(nro_path.c_str(), "rb");
  if (!f) return info;

  // Leer cabecera NRO0 (offset 0x10)
  if (fseek(f, 0x10, SEEK_SET) == 0) {
    uint8_t hdr[0x20];
    if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr)) {
      if (std::memcmp(hdr, "NRO0", 4) == 0) {
        // En offset 0x18 de NRO (offset 0x08 de hdr) se encuentra el tamaño del ejecutable,
        // que es exactamente donde empieza la cabecera ASET.
        const uint32_t aset_offset = read_u32(hdr + 0x08);
        if (aset_offset > 0 && fseek(f, aset_offset, SEEK_SET) == 0) {
          uint8_t aset_hdr[0x38];
          if (fread(aset_hdr, 1, sizeof(aset_hdr), f) == sizeof(aset_hdr)) {
            if (std::memcmp(aset_hdr, "ASET", 4) == 0) {
              const uint64_t icon_offset = read_u64(aset_hdr + 0x08);
              const uint64_t icon_size = read_u64(aset_hdr + 0x10);
              const uint64_t nacp_offset = read_u64(aset_hdr + 0x18);
              const uint64_t nacp_size = read_u64(aset_hdr + 0x20);

              // 1. Extraer metadatos NACP si estan disponibles
              if (nacp_size >= 0x4000 && fseek(f, aset_offset + nacp_offset, SEEK_SET) == 0) {
                std::vector<uint8_t> nacp(0x4000);
                if (fread(nacp.data(), 1, nacp.size(), f) == nacp.size()) {
                  // Titulo en NACP (primeros 0x200 bytes)
                  char title_buf[0x201] = {0};
                  std::memcpy(title_buf, nacp.data(), 0x200);
                  const std::string nacp_title = str::trim(title_buf);
                  if (!nacp_title.empty()) {
                    info.name = nacp_title;
                    if (is_decomp_app(nacp_title, nro_path)) {
                      info.is_decomp = true;
                    }
                  }

                  // Autor en NACP (offset 0x200, 0x100 bytes)
                  char author_buf[0x101] = {0};
                  std::memcpy(author_buf, nacp.data() + 0x200, 0x100);
                  info.author = str::trim(author_buf);

                  // Version en NACP (offset 0x3060, 0x10 bytes)
                  char ver_buf[0x11] = {0};
                  std::memcpy(ver_buf, nacp.data() + 0x3060, 0x10);
                  info.version = str::trim(ver_buf);
                }
              }

              // 2. Extraer icono incrustado si no habia icon.png local
              if (info.icon_path.empty() && icon_size > 0 && icon_size < 4 * 1024 * 1024) {
                if (fseek(f, aset_offset + icon_offset, SEEK_SET) == 0) {
                  std::vector<uint8_t> icon_bytes(icon_size);
                  if (fread(icon_bytes.data(), 1, icon_size, f) == icon_size) {
                    const std::string ext = (icon_bytes.size() >= 4 && icon_bytes[0] == 0x89 &&
                                             icon_bytes[1] == 'P' && icon_bytes[2] == 'N' &&
                                             icon_bytes[3] == 'G')
                                                ? ".png"
                                                : ".jpg";
                    const std::string cached_icon =
                        paths::join(paths::artwork_dir(), "nro_" + sanitize_name(info.name) + ext);
                    FILE* out = fopen(cached_icon.c_str(), "wb");
                    if (out) {
                      fwrite(icon_bytes.data(), 1, icon_bytes.size(), out);
                      fclose(out);
                      info.icon_path = cached_icon;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  fclose(f);

  if (info.icon_path.empty()) {
    info.icon_path = "romfs:/icons/default.png";
  }
  return info;
}

std::vector<HomebrewAppInfo> scan_switch_homebrew() {
  std::vector<HomebrewAppInfo> result;
  const std::string switch_dir = paths::to_sd_path("/switch");
  if (!paths::exists(switch_dir)) return result;

  const auto entries = paths::list_dir(switch_dir);
  for (const std::string& name : entries) {
    if (name == "." || name == ".." || str::iequals(name, "delanx")) continue;
    const std::string full = paths::join(switch_dir, name);

    if (paths::is_dir(full)) {
      // Buscar el .nro principal de la carpeta
      std::string nro_path;
      const std::string primary = paths::join(full, name + ".nro");
      if (paths::exists(primary) && !paths::is_dir(primary)) {
        nro_path = primary;
      } else {
        for (const std::string& sub : paths::list_dir(full)) {
          if (paths::extension(sub) == ".nro") {
            nro_path = paths::join(full, sub);
            break;
          }
        }
      }
      if (nro_path.empty()) continue;

      HomebrewAppInfo info = inspect_nro(nro_path, name);
      // Solo aplicaciones Homebrew puras: los decomps quedan estrictamente fuera
      if (!info.is_decomp) {
        result.push_back(info);
      }
    } else if (paths::extension(name) == ".nro") {
      const std::string base = name.substr(0, name.size() - 4);
      HomebrewAppInfo info = inspect_nro(full, base);
      if (!info.is_decomp) {
        result.push_back(info);
      }
    }
  }
  return result;
}

}  // namespace delanx
