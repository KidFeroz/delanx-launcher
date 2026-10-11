// Delanx — pantallas de sistema: actualizaciones, ajustes, instalacion y lanzamiento.
#include <algorithm>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "platform/switch_platform.hpp"
#include "platform/text_input.hpp"
#include "services/asset_selector.hpp"
#include "services/github.hpp"
#include "services/gitlab.hpp"
#include "services/installer.hpp"
#include "services/launcher.hpp"
#include "services/release_check.hpp"
#include "ui/app.hpp"

namespace delanx::ui {
namespace {

struct SettingsRow {
  std::string label;
  std::string value;
  std::string hint;
};

std::string yes_no(bool value) { return value ? "Si" : "No"; }

// GitHub sin token permite 60 peticiones por hora; al agotarlas responde 403. Es un caso
// real y frecuente, asi que merece un aviso accionable en vez de un error tecnico.
bool is_rate_limit_error(const std::string& message) {
  return str::icontains(message, "rate limit") || str::icontains(message, "403");
}

}  // namespace

// ------------------------------------------------------------------ actualizaciones

std::vector<ListItem> App::build_update_items() {
  std::vector<ListItem> items;
  for (size_t index = 0; index < library_.apps.size(); ++index) {
    const AppEntry& app = library_.apps[index];
    if (!app.update_available) continue;
    ListItem item;
    item.title = display_name(app);
    item.subtitle = app.project.empty() ? app.repository : app.project;
    item.icon_url = app.app_icon_url;
    item.library_index = static_cast<int>(index);
    item.installed = true;
    item.update = true;
    item.harbour = app.harbour_masters;
    item.verified = app.verified_entry;
    item.badge = "Actualizar";
    item.meta = (app.installed_version.empty() ? std::string("-") : app.installed_version) + "  ->  " +
                (app.latest_version.empty() ? std::string("-") : app.latest_version);
    items.push_back(item);
  }
  return items;
}

void App::draw_downloads_screen() {
  const int origin_x = theme_.sidebar_width;
  const int origin_y = theme_.topbar_height;
  const int content_w = renderer_.width() - origin_x;
  const int content_h = renderer_.height() - origin_y - theme_.footer_height;

  const std::vector<ListItem> updates = build_update_items();
  const std::string headline =
      updates.empty()
          ? std::string("No hay actualizaciones pendientes")
          : str::format("%lu apps con actualizacion disponible",
                        static_cast<unsigned long>(updates.size()));
  renderer_.draw_text(headline, origin_x + theme_.content_padding, origin_y + theme_.px(8),
                      updates.empty() ? theme_.text_muted : theme_.warning, theme_.font_body, !updates.empty());

  const int row_h = theme_.px(58);
  int y = origin_y + theme_.px(40);

  if (updates.empty()) {
    renderer_.draw_text("Pulsa X para comprobar las releases de las apps de tu biblioteca.",
                        origin_x + theme_.content_padding, y, theme_.text_dim, theme_.font_small);
    y += theme_.px(34);
  } else {
    focus_ = std::clamp(focus_, 0, static_cast<int>(updates.size()) - 1);
    const int max_rows = 5;
    for (size_t index = 0; index < updates.size() && static_cast<int>(index) < max_rows; ++index) {
      const ListItem& item = updates[index];
      const bool focused = static_cast<int>(index) == focus_;
      const int row_y = y + static_cast<int>(index) * row_h;
      const int row_x = origin_x + theme_.content_padding;
      const int row_w = content_w - theme_.content_padding * 2;
      hit_targets_.push_back({static_cast<int>(index), SDL_Rect{row_x, row_y, row_w, row_h - theme_.px(8)}});
      renderer_.fill_round_rect(row_x, row_y, row_w, row_h - theme_.px(8), theme_.px(10),
                                focused ? theme_.card_focus : theme_.panel_alt);
      if (focused) {
        renderer_.stroke_round_rect(row_x, row_y, row_w, row_h - theme_.px(8), theme_.px(10),
                                    theme_.focus_ring, 2);
      }
      renderer_.draw_text(item.title, row_x + theme_.px(16), row_y + theme_.px(6), theme_.text,
                          theme_.font_body, true);
      renderer_.draw_text(item.meta, row_x + theme_.px(16), row_y + theme_.px(28), theme_.accent,
                          theme_.font_small);
      if (!item.badge.empty()) {
        const int badge_w = renderer_.text_width(item.badge, theme_.font_small) + theme_.px(16);
        renderer_.fill_round_rect(row_x + row_w - badge_w - theme_.px(12), row_y + theme_.px(12), badge_w,
                                  theme_.px(24), theme_.px(8), theme_.warning.with_alpha(46));
        renderer_.draw_text(item.badge, row_x + row_w - badge_w - theme_.px(4), row_y + theme_.px(16),
                            theme_.warning, theme_.font_small, true);
      }
    }
    y += std::min(static_cast<int>(updates.size()), max_rows) * row_h + theme_.px(8);
    if (static_cast<int>(updates.size()) > 5) {
      renderer_.draw_text(str::format("y %lu mas...",
                                      static_cast<unsigned long>(updates.size() - 5)),
                          origin_x + theme_.content_padding, y, theme_.text_muted, theme_.font_small);
      y += theme_.px(24);
    }
  }

  renderer_.draw_text("Historial de esta sesion", origin_x + theme_.content_padding, y, theme_.text_dim,
                      theme_.font_small, true);
  y += theme_.px(26);
  if (downloads_.empty()) {
    renderer_.draw_text("Todavia no has instalado ni actualizado nada.", origin_x + theme_.content_padding, y,
                        theme_.text_muted, theme_.font_small);
  } else {
    for (size_t index = 0; index < downloads_.size() && index < 6; ++index) {
      const DownloadItem& item = downloads_[index];
      renderer_.fill_circle(origin_x + theme_.content_padding + theme_.px(8), y + theme_.px(9), theme_.px(5),
                            item.ok ? theme_.success : theme_.danger);
      renderer_.draw_text(item.app_name, origin_x + theme_.content_padding + theme_.px(24), y, theme_.text_dim,
                          theme_.font_small);
      renderer_.draw_text_right(item.version + "  " + item.status,
                                origin_x + content_w - theme_.content_padding, y, theme_.text_muted,
                                theme_.font_small);
      y += theme_.px(22);
    }
  }

  // Acciones disponibles en esta pantalla.
  const int hint_y = origin_y + content_h - theme_.px(56);
  renderer_.fill_round_rect(origin_x + theme_.content_padding, hint_y, content_w - theme_.content_padding * 2,
                            theme_.px(48), theme_.px(12), theme_.panel);
  renderer_.draw_text("A: actualizar la seleccionada    X: comprobar actualizaciones    Y: actualizar todo",
                      origin_x + theme_.content_padding + theme_.px(18), hint_y + theme_.px(8), theme_.text_dim,
                      theme_.font_body);
  renderer_.draw_text("Las actualizaciones solo instalan releases que Quiver haya verificado.",
                      origin_x + theme_.content_padding + theme_.px(18), hint_y + theme_.px(28),
                      theme_.text_muted, theme_.font_small);
}

void App::update_all_pending() {
  const std::vector<ListItem> items = build_update_items();
  if (items.empty()) {
    toast("No hay actualizaciones pendientes");
    return;
  }
  const std::string body = str::format(
      "Se actualizaran %lu apps a su release mas reciente.\n\nLas releases que Quiver no ha "
      "verificado se omiten: igual que en Quiver Launcher, las actualizaciones automaticas nunca "
      "instalan releases sin verificar. Esas puedes actualizarlas una a una desde la biblioteca.",
      static_cast<unsigned long>(items.size()));
  show_confirm("Actualizar todo", body, "Actualizar", [this, items]() {
    int updated = 0;
    int skipped = 0;
    int failed = 0;
    for (const ListItem& item : items) {
      if (item.library_index < 0 || item.library_index >= static_cast<int>(library_.apps.size())) continue;
      AppEntry& app = library_.apps[static_cast<size_t>(item.library_index)];
      begin_busy("Buscando " + display_name(app));
      Release release;
      ReleaseAsset asset;
      std::string error;
      const bool fetched = fetch_candidate_release(app, &release, &asset, &error);
      end_busy();
      if (!fetched) {
        platform::log("update-all %s: %s", app.folder_name.c_str(), error.c_str());
        ++failed;
        continue;
      }
      if (release.check != Release::Check::Verified) {
        ++skipped;
        continue;
      }
      if (run_install(app, release, asset, release.checksum_for(asset.name))) {
        ++updated;
      } else {
        ++failed;
      }
    }
    const std::string summary =
        str::format("Actualizadas %d, omitidas sin verificar %d, fallidas %d", updated, skipped, failed);
    downloads_.insert(downloads_.begin(),
                      DownloadItem{"Actualizar todo", "", summary, failed == 0});
    toast(summary, failed > 0);
  });
}

void App::check_updates() {
  if (library_.apps.empty()) {
    toast("La biblioteca esta vacia");
    return;
  }
  int updates = 0;
  int checked = 0;
  bool rate_limited = false;
  for (AppEntry& app : library_.apps) {
    if (app.manually_managed()) continue;
    begin_busy("Comprobando " + display_name(app));
    Release release;
    ReleaseAsset asset;
    std::string error;
    const bool ok = fetch_candidate_release(app, &release, &asset, &error);
    end_busy();
    ++checked;
    if (!ok) {
      platform::log("update check %s: %s", app.folder_name.c_str(), error.c_str());
      if (is_rate_limit_error(error)) {
        // Sin cuota no tiene sentido seguir preguntando por las demas apps.
        rate_limited = true;
        break;
      }
      continue;
    }
    app.latest_version = release.tag;
    app.refresh_install_state();
    app.update_available = app.update_available_for(release.tag);
    if (app.update_available) ++updates;
  }
  save_library();
  if (rate_limited && settings_.GitHubApiToken.empty()) {
    open_dialog("GitHub ha limitado las peticiones",
                str::format("Se comprobaron %d apps antes de agotar el limite de GitHub (60 peticiones "
                            "por hora sin token).\n\nPuedes crear un token personal (sin permisos "
                            "especiales) y pegarlo en Ajustes para subir el limite.",
                            checked),
                {DialogOption{"Abrir ajustes", [this]() {
                   tab_ = Tab::Settings;
                   focus_ = 8;  // fila del token de GitHub
                 }},
                 DialogOption{"Cerrar", nullptr}});
    return;
  }
  toast(str::format("Comprobadas %d apps: %d con actualizacion", checked, updates));
}

// ------------------------------------------------------------------ ajustes

void App::draw_settings_screen() {
  const int origin_x = theme_.sidebar_width;
  const int origin_y = theme_.topbar_height;
  const int content_w = renderer_.width() - origin_x;

  std::vector<SettingsRow> rows;
  rows.push_back({"Permitir releases sin verificar", yes_no(settings_.DelanxIncludeUnverified),
                  "Si esta en No, Delanx solo instala releases con checksum verificado"});
  rows.push_back({"Confirmar releases bloqueadas", yes_no(settings_.DelanxConfirmBlockedReleases),
                  "Quiver marca algunas releases como bloqueadas"});
  rows.push_back({"Seccion Harbour Masters", yes_no(settings_.DelanxHarbourMastersSection),
                  "Muestra la pestana exclusiva de ports de Harbour Masters"});
  rows.push_back({"Instalar en rutas nativas de Switch", yes_no(settings_.DelanxUseNativeInstallPaths),
                  "Usa /switch/<app>/ como los demas homebrew, en vez de Delanx/Apps"});
  rows.push_back({"Comprobar actualizaciones al arrancar", yes_no(settings_.DelanxAutoCheckUpdatesOnBoot),
                  "Consulta releases al abrir Delanx (consume red)"});
  rows.push_back({"Vista de rejilla", yes_no(settings_.UseGridView), "Rejilla de caratulas o lista compacta"});
  rows.push_back({"Columnas de la rejilla", str::format("%d", grid_columns_), "2 a 6 columnas"});
  rows.push_back({"Tema", settings_.is_dark() ? "Oscuro (Sphaira)" : "Claro", "Oscuro o claro"});
  rows.push_back({"Token de GitHub", settings_.GitHubApiToken.empty() ? "sin configurar" : "configurado",
                  "Evita el limite de peticiones de la API de GitHub"});
  rows.push_back({"Token de GitLab", settings_.GitLabApiToken.empty() ? "sin configurar" : "configurado",
                  "Necesario solo para proyectos privados de GitLab"});
  rows.push_back({"Comprobar actualizaciones ahora", "", "Consulta las releases de la biblioteca"});
  rows.push_back({"Informacion de la consola", "", "Firmware, memoria, origen de arranque y rutas"});

  focus_ = std::clamp(focus_, 0, static_cast<int>(rows.size()) - 1);
  const int row_h = theme_.px(52);
  int y = origin_y + theme_.px(30);
  for (size_t index = 0; index < rows.size(); ++index) {
    const bool focused = static_cast<int>(index) == focus_;
    // Zona pulsable de la fila, para la pantalla tactil.
    hit_targets_.push_back({static_cast<int>(index),
                            SDL_Rect{origin_x, y - theme_.px(6), content_w, row_h - theme_.px(6)}});
    if (focused) {
      renderer_.fill_round_rect(origin_x + theme_.content_padding - theme_.px(8), y - theme_.px(6),
                                content_w - theme_.content_padding * 2 + theme_.px(16), row_h - theme_.px(6),
                                theme_.px(10), theme_.panel_alt);
      renderer_.fill_round_rect(origin_x + theme_.content_padding - theme_.px(8), y - theme_.px(6),
                                theme_.px(4), row_h - theme_.px(6), theme_.px(2), theme_.accent);
    }
    renderer_.draw_text(rows[index].label, origin_x + theme_.content_padding, y + theme_.px(2),
                        focused ? theme_.text : theme_.text_dim, theme_.font_body, focused);
    renderer_.draw_text(rows[index].hint, origin_x + theme_.content_padding, y + theme_.px(24),
                        theme_.text_muted, theme_.font_small);
    if (!rows[index].value.empty()) {
      renderer_.draw_text_right(rows[index].value, origin_x + content_w - theme_.content_padding, y + theme_.px(6),
                                focused ? theme_.accent : theme_.text_dim, theme_.font_body, true);
    }
    y += row_h;
  }
}

void App::activate_settings_item() {
  switch (focus_) {
    case 0:
      settings_.DelanxIncludeUnverified = !settings_.DelanxIncludeUnverified;
      break;
    case 1:
      settings_.DelanxConfirmBlockedReleases = !settings_.DelanxConfirmBlockedReleases;
      break;
    case 2:
      settings_.DelanxHarbourMastersSection = !settings_.DelanxHarbourMastersSection;
      break;
    case 3:
      settings_.DelanxUseNativeInstallPaths = !settings_.DelanxUseNativeInstallPaths;
      break;
    case 4:
      settings_.DelanxAutoCheckUpdatesOnBoot = !settings_.DelanxAutoCheckUpdatesOnBoot;
      break;
    case 5:
      settings_.UseGridView = !settings_.UseGridView;
      break;
    case 6:
      grid_columns_ = grid_columns_ >= 6 ? 2 : grid_columns_ + 1;
      settings_.DelanxGridColumns = grid_columns_;
      break;
    case 7:
      settings_.DelanxTheme = settings_.is_dark() ? "sphaira-light" : "sphaira-dark";
      theme_ = settings_.is_dark() ? Theme::sphaira_dark() : Theme::sphaira_light();
      theme_.apply_scale(static_cast<float>(renderer_.width()) / 1280.0f);
      break;
    case 8: {
      std::string token;
      if (platform::text_input("Token de GitHub (vacio para borrar)", settings_.GitHubApiToken, &token, 128, true)) {
        settings_.GitHubApiToken = str::trim(token);
        toast(settings_.GitHubApiToken.empty() ? "Token de GitHub borrado" : "Token de GitHub guardado");
      }
      break;
    }
    case 9: {
      std::string token;
      if (platform::text_input("Token de GitLab (vacio para borrar)", settings_.GitLabApiToken, &token, 128, true)) {
        settings_.GitLabApiToken = str::trim(token);
        toast(settings_.GitLabApiToken.empty() ? "Token de GitLab borrado" : "Token de GitLab guardado");
      }
      break;
    }
    case 10:
      check_updates();
      return;
    case 11:
      open_dialog("Informacion de la consola",
                  str::format("Firmware: %s\nOrigen de arranque: %s\nModo applet: %s\nMemoria: %s\nDatos: %s\n"
                              "Catalogo local: %lu entradas\nCatalogo maestro: %lu entradas (sin verificar)\n"
                              "Harbour Masters: %lu entradas\n\n"
                              "Delanx %s — launcher homebrew para Switch. Lee y escribe apps.json en el mismo "
                              "formato que Quiver Launcher.",
                              platform::firmware_version().c_str(), platform::boot_origin().c_str(),
                              yes_no(applet_mode_).c_str(), platform::heap_usage_human().c_str(),
                              paths::root().c_str(),
                              static_cast<unsigned long>(local_catalog_.entries.size()),
                              static_cast<unsigned long>(master_catalog_.entries.size()),
                              static_cast<unsigned long>(harbour_catalog_.entries.size()), DELANX_VERSION),
                  {DialogOption{"Cerrar", nullptr}});
      return;
    default:
      return;
  }
  save_settings();
}

// ------------------------------------------------------------------ releases

bool App::fetch_candidate_release(const AppEntry& app, Release* release, ReleaseAsset* asset,
                                  std::string* error) {
  if (app.manually_managed()) {
    *error = "Esta entrada es manual: no tiene repositorio del que descargar.";
    return false;
  }
  std::vector<Release> releases;
  std::string source = app.repository_source.empty() ? "github" : app.repository_source;
  if (source == "gitlab") {
    releases = gl::fetch_releases(app.repository, settings_.GitLabApiToken, error);
  } else {
    releases = gh::fetch_releases(app.repository, settings_.GitHubApiToken, error);
  }
  if (releases.empty()) {
    if (error->empty()) *error = "El repositorio no publica ninguna release.";
    return false;
  }

  // Preferencia: version fijada por el usuario -> release verificada -> la mas reciente.
  int chosen = -1;
  if (!app.preferred_version.empty()) {
    for (size_t index = 0; index < releases.size(); ++index) {
      if (releases[index].tag == app.preferred_version) {
        chosen = static_cast<int>(index);
        break;
      }
    }
  }
  if (chosen < 0) {
    for (size_t index = 0; index < releases.size(); ++index) {
      if (releases[index].check == Release::Check::Verified) {
        chosen = static_cast<int>(index);
        break;
      }
    }
  }
  if (chosen < 0) {
    for (size_t index = 0; index < releases.size(); ++index) {
      chosen = static_cast<int>(index);
      break;
    }
  }
  Release candidate = releases[static_cast<size_t>(chosen)];

  // Verificacion contra el catalogo de Quiver cuando la entrada viene de alli (slug).
  const bool slug_like = !app.catalog_entry_id.empty() && app.catalog_entry_id.find('/') == std::string::npos;
  if (slug_like) {
    std::string check_error;
    const ReleaseCheckInfo info =
        fetch_release_check(settings_.DelanxCatalogBaseUrl, app.catalog_entry_id, candidate.tag, &check_error);
    apply_check(&candidate, info);
  } else if (!candidate.checksums.empty()) {
    candidate.check = Release::Check::Verified;
    candidate.check_reasons.push_back("Checksum publicado por el repositorio.");
  }

  const AssetChoice choice = select_switch_asset(candidate, app.release_asset_filter);
  if (choice.index < 0) {
    *error = choice.reason.empty() ? "El release no incluye ningun asset instalable en Switch." : choice.reason;
    return false;
  }
  *release = candidate;
  *asset = candidate.assets[static_cast<size_t>(choice.index)];
  return true;
}

bool App::run_install(const AppEntry& app, const Release& release, const ReleaseAsset& asset,
                      const std::string& expected_sha256) {
  InstallRequest request;
  request.app = app;
  request.release = release;
  request.asset = asset;
  request.expected_sha256 = expected_sha256;
  // La interfaz ya pidio confirmacion antes de llegar aqui, asi que una release sin
  // checksum publicado puede instalarse si el usuario lo acepto.
  request.allow_unverified = expected_sha256.empty();

  uint32_t last_draw = 0;
  const auto progress = [&](int64_t done, int64_t total) -> bool {
    const uint32_t now = SDL_GetTicks();
    if (now - last_draw < 80 && done != total) return true;
    last_draw = now;
    const float ratio = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.0f;

    renderer_.clear(theme_.bg);
    const int width = theme_.px(640);
    const int height = theme_.px(200);
    const int x = (renderer_.width() - width) / 2;
    const int y = (renderer_.height() - height) / 2;
    renderer_.fill_round_rect(x, y, width, height, theme_.px(16), theme_.panel);
    renderer_.draw_text("Instalando " + display_name(app), x + theme_.px(28), y + theme_.px(26), theme_.text,
                        theme_.font_title, true);
    renderer_.draw_text(asset.name, x + theme_.px(28), y + theme_.px(66), theme_.text_dim, theme_.font_small);
    const int bar_x = x + theme_.px(28);
    const int bar_y = y + theme_.px(104);
    const int bar_w = width - theme_.px(56);
    renderer_.fill_round_rect(bar_x, bar_y, bar_w, theme_.px(14), theme_.px(7), theme_.panel_alt);
    renderer_.fill_round_rect(bar_x, bar_y, std::max(theme_.px(6), static_cast<int>(bar_w * ratio)),
                              theme_.px(14), theme_.px(7), theme_.accent);
    renderer_.draw_text(str::format("%s / %s", str::human_size(static_cast<uint64_t>(done)).c_str(),
                                    str::human_size(static_cast<uint64_t>(total)).c_str()),
                        bar_x, bar_y + theme_.px(24), theme_.text_dim, theme_.font_small);
    renderer_.draw_text("B: cancelar", x + theme_.px(28), y + theme_.px(158), theme_.text_muted,
                        theme_.font_small);
    renderer_.end_frame();

    input_.update();
    if (input_.pressed(Btn::B)) return false;
    return true;
  };

  const InstallReport report = install_release(request, progress);

  DownloadItem item;
  item.app_name = display_name(app);
  item.version = release.tag;
  item.ok = report.status == InstallStatus::Ok;
  item.status = report.message.empty() ? (item.ok ? "instalado" : "fallo") : report.message;
  downloads_.insert(downloads_.begin(), item);

  if (report.status == InstallStatus::Ok) {
    toast(str::format("%s %s instalado", display_name(app).c_str(), release.tag.c_str()));
    for (AppEntry& entry : library_.apps) {
      if (entry.identity_key() == app.identity_key()) {
        entry.refresh_install_state();
        entry.latest_version = release.tag;
        entry.update_available = false;
        break;
      }
    }
    save_library();
    return true;
  }
  toast(report.message.empty() ? "La instalacion ha fallado" : report.message, true);
  return false;
}

void App::install_app(int library_index, bool force_unverified) {
  if (library_index < 0 || library_index >= static_cast<int>(library_.apps.size())) return;
  const AppEntry app = library_.apps[static_cast<size_t>(library_index)];

  begin_busy("Buscando release de " + display_name(app));
  Release release;
  ReleaseAsset asset;
  std::string error;
  const bool ok = fetch_candidate_release(app, &release, &asset, &error);
  end_busy();
  if (!ok) {
    if (is_rate_limit_error(error) && settings_.GitHubApiToken.empty()) {
      open_dialog("GitHub ha limitado las peticiones",
                  error + "\n\nSin token, GitHub solo permite 60 peticiones por hora. Puedes crear un "
                          "token personal (sin permisos especiales) y pegarlo en Ajustes para subir el limite.",
                  {DialogOption{"Abrir ajustes", [this]() {
                     tab_ = Tab::Settings;
                     focus_ = 8;  // fila del token de GitHub
                   }},
                   DialogOption{"Cerrar", nullptr}});
      return;
    }
    toast(error, true);
    return;
  }
  if (!platform::network_connected()) {
    toast("Sin conexion a internet", true);
    return;
  }

  const std::string expected = release.checksum_for(asset.name);
  const std::string asset_info = str::format("%s (%s)", asset.name.c_str(),
                                             str::human_size(static_cast<uint64_t>(asset.size)).c_str());

  if (release.check == Release::Check::Blocked && settings_.DelanxConfirmBlockedReleases) {
    show_confirm("Release bloqueada por Quiver",
                 "Quiver ha marcado esta release como bloqueada. La verificacion del fichero puede fallar o "
                 "el contenido puede ser problemático.\n\n" + asset_info,
                 "Instalar de todas formas", [this, library_index, release, asset, expected]() {
                   show_confirm("Confirmacion doble",
                                "De verdad quieres instalar una release bloqueada sin comprobacion fiable?",
                                "Si, instalar", [this, library_index, release, asset, expected]() {
                                  run_install(library_.apps[static_cast<size_t>(library_index)], release, asset,
                                              expected);
                                });
                 });
    return;
  }

  if (release.check != Release::Check::Verified && !force_unverified) {
    std::string body = "Quiver no ha verificado esta release, asi que el fichero se instalara sin comprobar "
                       "contra un checksum conocido.\n\n" + asset_info;
    if (!release.check_reasons.empty()) {
      body += "\n\nMotivo: " + release.check_reasons.front();
    }
    show_confirm("Release sin verificar", body, "Instalar igualmente",
                 [this, library_index, release, asset, expected]() {
                   run_install(library_.apps[static_cast<size_t>(library_index)], release, asset, expected);
                 });
    return;
  }

  if (expected.empty() && !settings_.DelanxIncludeUnverified) {
    toast("La release no tiene checksum y las releases sin verificar estan desactivadas", true);
    return;
  }
  run_install(app, release, asset, expected);
}

void App::launch_app(int library_index) {
  if (library_index < 0 || library_index >= static_cast<int>(library_.apps.size())) return;
  AppEntry& app = library_.apps[static_cast<size_t>(library_index)];
  app.refresh_install_state();
  const std::string nro = find_launchable_nro(app.install_dir(), app.sw.nro);
  if (nro.empty()) {
    toast("No se encontro ningun .nro en la carpeta de la app", true);
    return;
  }
  const std::vector<SwitchInstall::RequiredFile> missing = app.missing_user_files();
  if (!missing.empty()) {
    std::string body = "Esta app necesita ficheros que aporta el usuario y aun faltan:\n\n";
    for (const auto& file : missing) body += "  - " + file.path + "\n";
    body += "\nPuedes lanzarla igualmente, pero es probable que el juego no arranque.";
    show_confirm("Faltan ficheros", body, "Lanzar de todas formas", [this, nro, library_index]() {
      LaunchTarget target;
      target.nro_path = nro;
      std::string error;
      if (!request_next_nro(target, &error)) {
        toast(error, true);
        return;
      }
      launching_ = true;
    });
    return;
  }

  LaunchTarget target;
  target.nro_path = nro;
  std::string error;
  if (!request_next_nro(target, &error)) {
    toast(error, true);
    return;
  }
  launching_ = true;
}

void App::uninstall_app(int library_index) {
  if (library_index < 0 || library_index >= static_cast<int>(library_.apps.size())) return;
  const AppEntry app = library_.apps[static_cast<size_t>(library_index)];
  show_confirm("Desinstalar " + display_name(app),
               "Se borrara " + app.install_dir() +
                   ". Los ficheros que hayas aportado tu (ROMs, .otr/.o2r, partidas) no se tocan.",
               "Desinstalar", [this, app]() {
                 std::string error;
                 if (!::delanx::uninstall_app(app, false, &error)) {
                   toast(error.empty() ? "No se pudo desinstalar" : error, true);
                   return;
                 }
                 for (size_t index = 0; index < library_.apps.size(); ++index) {
                   if (library_.apps[index].identity_key() == app.identity_key()) {
                     library_.apps[index].refresh_install_state();
                     break;
                   }
                 }
                 save_library();
                 toast("Desinstalada: " + display_name(app));
               });
}

}  // namespace delanx::ui
