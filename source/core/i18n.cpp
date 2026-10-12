// Delanx — tabla de cadenas localizadas (espanol / ingles).
#include "core/i18n.hpp"

#include <cstring>
#include "core/strings.hpp"

namespace delanx::i18n {
namespace {

Language g_lang = Language::Spanish;

struct Entry {
  const char* key;
  const char* es;
  const char* en;
};

// Diccionario centralizado de cadenas de interfaz.
const Entry kTranslations[] = {
    // Pestanas superiores
    {"tab.library", "Biblioteca", "Library"},
    {"tab.catalog", "Catálogo", "Catalog"},
    {"tab.mrship", "MrShip", "MrShip"},
    {"tab.homebrew", "Homebrew", "Homebrew"},
    {"tab.downloads", "Actualizaciones", "Updates"},
    {"tab.settings", "Ajustes", "Settings"},

    // Estados e insignias
    {"badge.installed", "Instalado", "Installed"},
    {"badge.update", "Actualiza", "Update"},
    {"badge.manual", "Manual", "Manual"},
    {"badge.verified", "Verificado", "Verified"},
    {"badge.unverified", "Sin verificar", "Unverified"},
    {"badge.homebrew", "Homebrew", "Homebrew"},
    {"badge.favorite", "Favorito", "Favorite"},
    {"badge.fav_star", "★ Favorito", "★ Favorite"},
    {"badge.ver_star", "★ Verificado", "★ Verified"},
    {"badge.with_build", "Con build Switch", "With Switch build"},
    {"badge.no_build", "Sin build Switch", "No Switch build"},

    // Acciones de botones
    {"btn.launch", "Lanzar", "Launch"},
    {"btn.install", "Instalar", "Install"},
    {"btn.update", "Actualizar", "Update"},
    {"btn.reinstall", "Reinstalar", "Reinstall"},
    {"btn.close", "Cerrar", "Close"},
    {"btn.cancel", "Cancelar", "Cancel"},
    {"btn.filter", "Filtrar", "Filter"},
    {"btn.search", "Buscar", "Search"},
    {"btn.favorite", "Favorito", "Favorite"},
    {"btn.exit", "Salir", "Exit"},
    {"btn.understood", "Entendido", "Got it"},
    {"btn.check_updates", "Comprobar", "Check"},

    // Modales y confirmaciones
    {"quit.title", "Salir de Delanx", "Exit Delanx"},
    {"quit.body", "¿Deseas salir y regresar al menú principal (hbmenu)?", "Do you want to exit and return to hbmenu?"},
    {"details.requirements", "Requisitos", "Requirements"},
    {"details.user_files", "Ficheros del usuario", "User Files"},
    {"details.no_user_files", "Esta app no necesita ficheros adicionales.", "This app does not require extra files."},
    {"details.installed_at", "Instala en: ", "Installs at: "},
    {"details.extracts_at", "Extrae en: ", "Extracts at: "},
    {"details.executable", "Ejecutable: ", "Executable: "},
    {"details.installed_ver", "Instalada: ", "Installed: "},
    {"details.available_ver", "Disponible: ", "Available: "},

    // Mensajes de listas vacias
    {"empty.library", "Sin apps instaladas. Añade desde el catálogo.", "No installed apps. Add from the catalog."},
    {"empty.catalog", "Catálogo vacío.", "Catalog is empty."},
    {"empty.master", "Sin entradas en el catálogo maestro.", "No entries in the master catalog."},
    {"empty.quiver", "Sin resultados. Pulsa L+R para buscar en Quiver.", "No results. Press L+R to search Quiver."},
    {"empty.mrship", "Sin entradas de ports N64.", "No N64 port entries found."},
    {"empty.homebrew", "No se encontraron aplicaciones homebrew en /switch.", "No homebrew applications found in /switch."},
    {"empty.downloads", "No hay descargas o actualizaciones pendientes.", "No pending downloads or updates."},
    {"empty.favs", "No tienes ningún título marcado como favorito.", "No titles marked as favorite."},

    // Textos informativos y toasts
    {"info.items_count", "%lu elementos", "%lu items"},
    {"info.installing", "Instalando %s", "Installing %s"},
    {"info.installed_ok", "%s %s instalado", "%s %s installed"},
    {"info.install_failed", "La instalación ha fallado", "Installation failed"},
    {"info.checking", "Comprobando %s", "Checking %s"},
    {"info.download_busy", "Descargando", "Downloading"},
    {"info.cancel_hint", "Pulsa B para cancelar", "Press B to cancel"},
    {"info.fav_added", "★ Marcado como favorito: %s", "★ Added to favorites: %s"},
    {"info.fav_removed", "Eliminado de favoritos: %s", "Removed from favorites: %s"},

    // Filtros de biblioteca y catalogo
    {"filter.installed", "Instalados", "Installed"},
    {"filter.with_update", "Con actualización", "With update"},
    {"filter.mrship_ports", "Ports MrShip", "MrShip Ports"},
    {"filter.all", "Todos", "All"},
    {"filter.favorites", "Favoritos ★", "Favorites ★"},
};

constexpr size_t kTranslationCount = sizeof(kTranslations) / sizeof(kTranslations[0]);

}  // namespace

void set_language(Language lang) {
  g_lang = lang;
}

void set_language_code(const std::string& code) {
  const std::string lower = str::lower(str::trim(code));
  if (lower == "en" || lower == "english" || lower == "inglés" || lower == "ingles") {
    g_lang = Language::English;
  } else {
    g_lang = Language::Spanish;
  }
}

Language current_language() {
  return g_lang;
}

std::string current_language_code() {
  return (g_lang == Language::English) ? "en" : "es";
}

const char* tr(const char* key) {
  if (!key || key[0] == '\0') return "";
  for (size_t i = 0; i < kTranslationCount; ++i) {
    if (std::strcmp(kTranslations[i].key, key) == 0) {
      return (g_lang == Language::English) ? kTranslations[i].en : kTranslations[i].es;
    }
  }
  return key;
}

}  // namespace delanx::i18n
