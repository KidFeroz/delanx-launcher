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
    {"btn.uninstall", "Desinstalar", "Uninstall"},
    {"btn.delete", "Eliminar", "Delete"},

    // Modales y confirmaciones
    {"confirm.uninstall_app_title", "Desinstalar ", "Uninstall "},
    {"confirm.uninstall_app_body_1", "Se borrará el ejecutable y archivos de instalación en ", "The executable and installation files in "},
    {"confirm.uninstall_app_body_2", ".\nLos archivos que hayas aportado tú (partidas, ROMs, .otr/.o2r) no se tocan.", ".\nFiles you provided (saves, ROMs, .otr/.o2r) will not be touched."},
    {"confirm.uninstall_hb_title", "Eliminar ", "Delete "},
    {"confirm.uninstall_hb_body", "¿Deseas eliminar definitivamente este Homebrew y sus archivos de la tarjeta SD?\n\n", "Do you want to permanently delete this Homebrew and its files from the SD card?\n\n"},
    {"toast.uninstall_ok", "Desinstalada: ", "Uninstalled: "},
    {"toast.uninstall_failed", "No se pudo desinstalar", "Failed to uninstall"},
    {"toast.delete_hb_ok", "Homebrew eliminado: ", "Homebrew deleted: "},
    {"toast.delete_hb_failed", "No se pudo eliminar el archivo", "Failed to delete file"},
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
    {"empty.quiver", "Sin resultados. Pulsa L o R para buscar.", "No results. Press L or R to search."},
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

    // Valores de Ajustes
    {"val.yes", "Sí", "Yes"},
    {"val.no", "No", "No"},
    {"val.configured", "configurado", "configured"},
    {"val.not_configured", "sin configurar", "not set"},
    {"val.dark", "Oscuro", "Dark"},
    {"val.light", "Claro", "Light"},
    {"val.lang_es", "Español", "Spanish"},
    {"val.lang_en", "English", "English"},

    // Filas de Ajustes y explicaciones
    {"settings.unverified", "Permitir releases sin verificar", "Allow unverified releases"},
    {"settings.unverified_hint", "Si está en No, solo instala releases con checksum verificado", "If set to No, only releases with verified checksum are installed"},
    {"settings.confirm_blocked", "Confirmar releases bloqueadas", "Confirm blocked releases"},
    {"settings.confirm_blocked_hint", "Delanx marca algunas releases como bloqueadas", "Delanx flags certain releases as blocked"},
    {"settings.mrship", "Sección MrShip (Ports N64)", "MrShip Section (N64 Ports)"},
    {"settings.mrship_hint", "Muestra la pestaña de ports N64 de decompilación", "Displays the N64 decompilation ports tab"},
    {"settings.native_paths", "Instalar en rutas nativas de Switch", "Install to native Switch paths"},
    {"settings.native_paths_hint", "Usa /switch/<app>/ como los demás homebrew", "Uses /switch/<app>/ like standard homebrew"},
    {"settings.autocheck", "Comprobar actualizaciones al arrancar", "Check updates on boot"},
    {"settings.autocheck_hint", "Consulta releases al abrir Delanx (consume red)", "Queries releases when starting Delanx (uses network)"},
    {"settings.grid_view", "Vista de rejilla", "Grid view"},
    {"settings.grid_view_hint", "Rejilla de carátulas o lista compacta", "Cover grid or compact list"},
    {"settings.grid_columns", "Columnas de la rejilla", "Grid columns"},
    {"settings.grid_columns_hint", "2 a 6 columnas", "2 to 6 columns"},
    {"settings.theme", "Tema visual", "Visual theme"},
    {"settings.theme_hint", "Oscuro o claro", "Dark or light"},
    {"settings.language", "Idioma / Language", "Idioma / Language"},
    {"settings.language_hint", "Idioma de los textos e interfaz", "Interface and text language"},
    {"settings.organize_folders", "Organizar carpetas de decomps", "Organize decomp folders"},
    {"settings.organize_folders_hint", "Normaliza nombres de carpetas en /switch a rutas estándar", "Normalizes folder names in /switch to standard paths"},
    {"settings.github_token", "Token de GitHub", "GitHub Token"},
    {"settings.github_token_hint", "Evita el límite de peticiones de la API de GitHub", "Avoids GitHub API request rate limits"},
    {"settings.gitlab_token", "Token de GitLab", "GitLab Token"},
    {"settings.gitlab_token_hint", "Necesario solo para proyectos privados de GitLab", "Required only for private GitLab projects"},
    {"settings.check_updates_now", "Comprobar actualizaciones ahora", "Check for updates now"},
    {"settings.check_updates_now_hint", "Consulta las releases de la biblioteca", "Checks releases for library titles"},
    {"settings.system_info", "Información de la consola", "System information"},
    {"settings.system_info_hint", "Firmware, memoria, origen de arranque y rutas", "Firmware, memory, boot origin, and paths"},
    {"settings.input_test", "Test de mandos y pantalla", "Controller and touch test"},
    {"settings.input_test_hint", "Comprobar botones, sticks y respuesta táctil", "Test buttons, sticks, and touch input"},

    // Notificaciones emergentes de esquina (Corner notification)
    {"notify.downloading", "Descargando", "Downloading"},
    {"notify.installing", "Instalando", "Installing"},
    {"notify.busy_warning", "Descarga en curso. Espera a que termine.", "Download in progress. Please wait until finished."},
    {"notify.completed", "Instalación completada", "Installation completed"},
    {"notify.already_active", "ya se está descargando", "is already downloading"},
    {"notify.already_queued", "ya está en la cola de descargas", "is already in the download queue"},
    {"notify.queued_count", "en cola", "queued"},

    // Dialogos de actualizacion, reinstalacion y organizacion
    {"dialog.up_to_date_title", "Aplicación al día", "Application Up to Date"},
    {"dialog.up_to_date_body", "%s ya se encuentra en la versión más reciente (%s).\n\n¿Deseas reinstalar la aplicación de todas formas?", "%s is already at the latest version (%s).\n\nDo you want to reinstall the application anyway?"},
    {"dialog.update_available_title", "Actualización disponible", "Update Available"},
    {"dialog.update_available_body", "Se encontró una nueva versión para %s:\n\nInstalada: %s\nDisponible: %s (%s)\n\n¿Deseas actualizar ahora?", "A new version was found for %s:\n\nInstalled: %s\nAvailable: %s (%s)\n\nDo you want to update now?"},
    {"btn.organize_folder", "Organizar carpeta", "Organize folder"},
    {"confirm.organize_folder_title", "Organizar carpeta de decomp", "Organize Decomp Folder"},
    {"confirm.organize_folder_body", "¿Deseas renombrar y organizar la carpeta:\n'%s'\na la ruta estándar:\n'%s'?", "Do you want to rename and organize folder:\n'%s'\nto standard path:\n'%s'?"},
    {"dialog.repair_folder_title", "Reparar ruta de instalación", "Repair Installation Path"},
    {"dialog.repair_folder_body", "La carpeta actual de %s está en una ruta no estándar:\n'%s'\n\n¿Deseas reparar y mover la carpeta a la ruta estándar sugerida?\n'%s'", "%s is currently installed in a non-standard path:\n'%s'\n\nDo you want to repair and move the folder to the suggested standard path?\n'%s'"},
    {"btn.repair_folder", "Reparar ruta", "Repair path"},
    {"btn.reinstall_and_repair", "Reinstalar y reparar", "Reinstall & repair"},
    {"btn.reinstall_current", "Reinstalar en ruta actual", "Reinstall in current path"},
    // Insignias y tags
    {"badge.gh_linked", "GitHub Vinculado", "GitHub Linked"},
    {"badge.standalone", "NRO Suelto", "Standalone NRO"},

    // Acciones de botones e hints
    {"btn.accept", "Aceptar", "Accept"},
    {"btn.back", "Atrás", "Back"},
    {"btn.open_settings", "Abrir ajustes", "Open Settings"},
    {"btn.install_anyway", "Instalar de todas formas", "Install Anyway"},
    {"hint.details_launch", "Detalles / Lanzar", "Details / Launch"},
    {"hint.install_update", "Instalar / Actualizar", "Install / Update"},
    {"hint.add_view", "Añadir / Ver", "Add / View"},
    {"hint.info_install", "Ficha / Instalar", "Details / Install"},
    {"hint.rom_help", "Ayuda ROMs", "ROM Guide"},
    {"hint.launch_hb", "Lanzar Homebrew", "Launch Homebrew"},
    {"hint.update_sel", "Actualizar seleccion", "Update Selected"},
    {"hint.update_all", "Actualizar todo", "Update All"},
    {"hint.check_updates", "Buscar updates", "Check Updates"},
    {"hint.modify", "Modificar", "Change"},
    {"hint.search", "Buscar", "Search"},

    // Notificaciones y mensajes informativos
    {"info.free", "libres", "free"},
    {"info.apps_with_update", "%lu apps con actualizacion disponible", "%lu apps with update available"},
    {"info.no_updates_pending", "No hay actualizaciones pendientes", "No pending updates"},
    {"info.press_x_check", "Pulsa X para comprobar las releases de las apps de tu biblioteca.", "Press X to check releases for your library."},
    {"info.session_history", "Historial de esta sesion", "Session History"},
    {"info.no_history", "Todavia no has instalado ni actualizado nada.", "Nothing installed or updated yet."},
    {"info.checked_updates", "Comprobadas %d apps: %d con actualizacion", "Checked %d apps: %d with update"},
    {"info.more_apps", "y %lu mas...", "and %lu more..."},
    {"details.non_std_folder", "Ruta no estándar (sugerida: /switch/%s)", "Non-standard path (suggested: /switch/%s)"},

    // Dialogos adicionales
    {"dialog.system_info_title", "Información de la consola", "System Information"},
    {"dialog.check_updates_err_title", "Comprobación de actualización", "Update Check"},
    {"dialog.check_updates_err_body", "No se pudo consultar el repositorio %s: %s", "Could not query repository %s: %s"},
    {"dialog.rate_limit_title", "GitHub ha limitado las peticiones", "GitHub Rate Limit Reached"},
    {"dialog.rate_limit_body", "Se comprobaron %d apps antes de agotar el límite de GitHub (60 peticiones por hora sin token).\n\nPuedes agregar un token personal en Ajustes para aumentar el límite.", "Checked %d apps before reaching GitHub limit (60 requests per hour without token).\n\nYou can add a personal token in Settings to increase it."},
    {"dialog.blocked_title", "Release bloqueada por Delanx", "Release Blocked by Delanx"},
    {"dialog.blocked_body", "Delanx ha marcado esta release como bloqueada. La verificación del fichero puede fallar o el contenido puede ser problemático.\n\n%s", "Delanx has flagged this release as blocked. File verification may fail or content may have issues.\n\n%s"},
    {"dialog.double_confirm_title", "Confirmación doble", "Double Confirmation"},
    {"dialog.double_confirm_body", "¿De verdad quieres instalar una release bloqueada sin comprobación fiable?", "Do you really want to install a blocked release without reliable verification?"},
    {"btn.yes_install", "Sí, instalar", "Yes, install"},
    {"dialog.unverified_title", "Release sin verificar", "Unverified Release"},
    {"dialog.unverified_body", "Delanx no ha verificado esta release, así que el fichero se instalará sin comprobar contra un checksum conocido.\n\n%s", "Delanx has not verified this release, so the file will be installed without checking against a known checksum.\n\n%s"},
    {"dialog.reason_prefix", "Motivo: ", "Reason: "},
    {"toast.dark_theme", "Tema oscuro activado", "Dark theme enabled"},
    {"toast.light_theme", "Tema claro activado", "Light theme enabled"},
    {"toast.github_token_cleared", "Token de GitHub borrado", "GitHub token cleared"},
    {"toast.github_token_saved", "Token de GitHub guardado", "GitHub token saved"},
    {"toast.gitlab_token_cleared", "Token de GitLab borrado", "GitLab token cleared"},
    {"toast.gitlab_token_saved", "Token de GitLab guardado", "GitLab token saved"},
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
