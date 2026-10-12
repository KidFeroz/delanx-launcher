// Delanx — detector difuso y organizador de carpetas de decomps en la tarjeta SD.
#include "services/decomp_scanner.hpp"

#include <cstdio>
#include <algorithm>
#include <vector>
#include <string>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "services/nro_inspector.hpp"

namespace delanx {
namespace {

struct FolderSignature {
  const char* id_keyword;       // Identificador del decomp (p. ej. "soh", "2ship", "starship")
  const char* canonical_dir;    // Carpeta recomendada estandar (p. ej. "soh", "2ship", "Starship")
  const char* const dir_tokens[8];   // Palabras clave en el nombre de la carpeta
  const char* const nro_tokens[6];   // Palabras clave en el nombre del fichero .nro
  const char* const file_signatures[6]; // Ficheros clave dentro de la carpeta (.otr, .o2r, etc.)
};

const FolderSignature kSignatures[] = {
    {
        "soh",
        "soh",
        {"macready", "golf", "shipwright", "soh", "ocarina", nullptr},
        {"soh.nro", "shipwright.nro", "oot.nro", nullptr},
        {"oot.otr", "oot-mq.otr", "soh.otr", nullptr},
    },
    {
        "2ship",
        "2ship",
        {"2ship", "keiichi", "alfa", "2harkinian", "majora", nullptr},
        {"2ship.nro", "2ship2harkinian.nro", "mm.nro", nullptr},
        {"mm.o2r", "2ship.o2r", nullptr},
    },
    {
        "starship",
        "Starship",
        {"starship", "starfox", "sf64", nullptr},
        {"starship.nro", "sf64.nro", nullptr},
        {"sf64.otr", nullptr},
    },
    {
        "spaghettify",
        "Spaghettify",
        {"spaghetti", "spaghettikart", "mk64", nullptr},
        {"spaghettikart.nro", "mk64.nro", nullptr},
        {"mk64.otr", nullptr},
    },
    {
        "sm64",
        "sm64",
        {"sm64", "ghostship", nullptr},
        {"sm64.nro", "sm64.us.f3dex2e.nro", nullptr},
        {"sm64.us.z64", nullptr},
    },
    {
        "zelda3",
        "zelda3",
        {"zelda3", "alttp", nullptr},
        {"zelda3.nro", nullptr},
        {"zelda3_assets.dat", nullptr},
    },
    {
        "perfect_dark",
        "perfect_dark",
        {"perfect_dark", "perfectdark", nullptr},
        {"perfect_dark.nro", nullptr},
        {"pd.ntsc-final.z64", "pd.pal-final.z64", nullptr},
    },
    {
        "smw",
        "smw",
        {"smw", nullptr},
        {"smw.nro", nullptr},
        {nullptr},
    },
};

// Comprueba si un directorio coincide con alguna firma de decompilacion
const FolderSignature* match_signature(const std::string& folder_name,
                                       const std::vector<std::string>& files,
                                       const std::string& nro_name) {
  const std::string lower_folder = str::lower(folder_name);
  const std::string lower_nro = str::lower(nro_name);

  for (const auto& sig : kSignatures) {
    // 1. Coincidencia por archivo de firma dentro de la carpeta (ej. oot.otr, mm.o2r, sf64.otr)
    for (int i = 0; sig.file_signatures[i] != nullptr; ++i) {
      const char* file_sig = sig.file_signatures[i];
      for (const auto& f : files) {
        if (str::iequals(f, file_sig)) return &sig;
      }
    }

    // 2. Coincidencia combinada de carpeta + nro
    bool folder_match = false;
    for (int i = 0; sig.dir_tokens[i] != nullptr; ++i) {
      if (str::icontains(lower_folder, sig.dir_tokens[i])) {
        folder_match = true;
        break;
      }
    }

    bool nro_match = false;
    if (!lower_nro.empty()) {
      for (int i = 0; sig.nro_tokens[i] != nullptr; ++i) {
        if (str::icontains(lower_nro, sig.nro_tokens[i])) {
          nro_match = true;
          break;
        }
      }
    }

    if (folder_match || nro_match) {
      return &sig;
    }
  }
  return nullptr;
}

const CatalogEntry* find_catalog_for_sig(const FolderSignature* sig,
                                        const std::vector<Catalog>& catalogs) {
  if (!sig) return nullptr;
  const std::string target_id = sig->id_keyword;

  for (const auto& cat : catalogs) {
    for (const auto& entry : cat.entries) {
      const std::string lower_name = str::lower(entry.name);
      const std::string lower_proj = str::lower(entry.project);
      const std::string lower_folder = str::lower(entry.folder_name);
      const std::string lower_repo = str::lower(entry.repository);

      if (str::icontains(lower_name, target_id) ||
          str::icontains(lower_proj, target_id) ||
          str::icontains(lower_folder, target_id) ||
          str::icontains(lower_repo, target_id)) {
        return &entry;
      }
      for (const auto& t : entry.tags) {
        if (str::iequals(t, target_id)) return &entry;
      }
    }
  }
  return nullptr;
}

}  // namespace

int scan_and_adopt_decomps(Library& library, const std::vector<Catalog>& catalogs) {
  const std::string switch_dir = paths::to_sd_path("/switch");
  if (!paths::is_dir(switch_dir)) return 0;

  int adopted_count = 0;
  const std::vector<std::string> subdirs = paths::list_dir(switch_dir);

  for (const std::string& name : subdirs) {
    const std::string full_path = paths::join(switch_dir, name);
    if (!paths::is_dir(full_path)) continue;

    // Saltar utilidades homebrew puras
    if (is_homebrew_app(name, "")) continue;

    const std::vector<std::string> files = paths::list_dir(full_path);
    std::string nro_file;
    std::string nro_path;

    for (const auto& f : files) {
      if (str::iends_with(f, ".nro")) {
        nro_file = f;
        nro_path = paths::join(full_path, f);
        break;
      }
    }

    // Buscar en subdirectorios de 1 nivel si no hay nro en la raiz
    if (nro_path.empty()) {
      for (const auto& sub : files) {
        const std::string sub_path = paths::join(full_path, sub);
        if (paths::is_dir(sub_path)) {
          for (const auto& sub_f : paths::list_dir(sub_path)) {
            if (str::iends_with(sub_f, ".nro")) {
              nro_file = sub_f;
              nro_path = paths::join(sub_path, sub_f);
              break;
            }
          }
        }
        if (!nro_path.empty()) break;
      }
    }

    if (nro_path.empty()) continue;
    if (is_homebrew_app(paths::basename(nro_path), "")) continue;

    const FolderSignature* sig = match_signature(name, files, nro_file);
    if (!sig) continue;

    const CatalogEntry* cat_entry = find_catalog_for_sig(sig, catalogs);
    if (!cat_entry) continue;

    // Buscar si ya existe en la biblioteca
    AppEntry* existing = nullptr;
    const std::string cat_key = cat_entry->to_app_entry().identity_key();
    for (auto& app : library.apps) {
      if (app.identity_key() == cat_key ||
          str::iequals(app.name, cat_entry->name) ||
          str::iequals(app.folder_name, name) ||
          str::iequals(app.install_dir(), full_path)) {
        existing = &app;
        break;
      }
    }

    if (existing) {
      // Actualizar a la ruta real instalada detectada en la SD
      existing->install_path = full_path;
      existing->sw.target_dir = full_path;
      existing->sw.nro = nro_file;
      existing->installed = true;
      const std::string vfile = paths::join(full_path, "version.txt");
      if (paths::exists(vfile)) {
        if (auto text = paths::read_file(vfile)) {
          existing->installed_version = str::trim(*text);
        }
      }
    } else {
      // Adoptar nueva entrada de decomp en la biblioteca
      AppEntry new_entry = cat_entry->to_app_entry();
      new_entry.folder_name = name;
      new_entry.install_path = full_path;
      new_entry.sw.target_dir = full_path;
      new_entry.sw.nro = nro_file;
      new_entry.installed = true;
      const std::string vfile = paths::join(full_path, "version.txt");
      if (paths::exists(vfile)) {
        if (auto text = paths::read_file(vfile)) {
          new_entry.installed_version = str::trim(*text);
        }
      }
      library.apps.push_back(new_entry);
      ++adopted_count;
    }
  }

  return adopted_count;
}

bool has_non_standard_folder(const AppEntry& app, std::string* out_suggested_folder) {
  if (!app.installed) return false;
  const std::string current = paths::basename(app.install_dir());
  if (current.empty()) return false;

  const std::string lower_current = str::lower(current);
  const std::string lower_name = str::lower(app.name);
  const std::string lower_nro = str::lower(app.sw.nro);

  for (const auto& sig : kSignatures) {
    bool matched = false;
    if (str::icontains(lower_current, sig.id_keyword) ||
        str::icontains(lower_name, sig.id_keyword)) {
      matched = true;
    }
    if (!matched) {
      for (int i = 0; sig.dir_tokens[i] != nullptr; ++i) {
        if (str::icontains(lower_current, sig.dir_tokens[i]) ||
            str::icontains(lower_name, sig.dir_tokens[i])) {
          matched = true;
          break;
        }
      }
    }
    if (!matched && !lower_nro.empty()) {
      for (int i = 0; sig.nro_tokens[i] != nullptr; ++i) {
        if (str::icontains(lower_nro, sig.nro_tokens[i])) {
          matched = true;
          break;
        }
      }
    }

    if (matched) {
      if (!str::iequals(current, sig.canonical_dir)) {
        if (out_suggested_folder) *out_suggested_folder = sig.canonical_dir;
        return true;
      }
      return false;
    }
  }
  return false;
}

bool organize_decomp_folder(AppEntry& app, const std::string& new_folder_name, std::string* error) {
  if (new_folder_name.empty()) {
    if (error) *error = "Nombre de carpeta vacio";
    return false;
  }
  const std::string old_path = app.install_dir();
  if (old_path.empty() || !paths::exists(old_path)) {
    if (error) *error = "La carpeta original no existe";
    return false;
  }

  const std::string switch_dir = paths::to_sd_path("/switch");
  const std::string new_path = paths::join(switch_dir, new_folder_name);

  if (str::iequals(old_path, new_path)) return true;
  if (paths::exists(new_path)) {
    if (error) *error = "Ya existe un directorio con ese nombre en /switch";
    return false;
  }

  if (std::rename(old_path.c_str(), new_path.c_str()) != 0) {
    if (error) *error = "Error del sistema al renombrar directorio";
    return false;
  }

  app.folder_name = new_folder_name;
  app.install_path = new_path;
  app.sw.target_dir = new_path;
  app.refresh_install_state();
  return true;
}

}  // namespace delanx
