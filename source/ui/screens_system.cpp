// Delanx — pantallas de sistema: actualizaciones, ajustes, instalacion y lanzamiento.
#include <algorithm>

#include "core/i18n.hpp"
#include "core/paths.hpp"
#include "core/strings.hpp"
#include "platform/switch_platform.hpp"
#include "platform/text_input.hpp"
#include "services/asset_selector.hpp"
#include "services/github.hpp"
#include "services/gitlab.hpp"
#include "services/decomp_scanner.hpp"
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
    item.mrship = app.mrship;
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
  const int origin_y = theme_.topbar_height + theme_.tab_bar_height;
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

  // Nota informativa en la parte inferior de la pantalla.
  const int note_y = origin_y + content_h - theme_.px(28);
  renderer_.draw_text("Nota: las actualizaciones automaticas solo instalan releases con checksum verificado.",
                      origin_x + theme_.content_padding, note_y,
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
                   focus_ = 9;  // fila del token de GitHub
                 }},
                 DialogOption{"Cerrar", nullptr}});
    return;
  }
  toast(str::format("Comprobadas %d apps: %d con actualizacion", checked, updates));
}

// ------------------------------------------------------------------ ajustes

void App::organize_all_decomps() {
  std::vector<std::pair<size_t, std::string>> to_organize;
  for (size_t i = 0; i < library_.apps.size(); ++i) {
    std::string suggested;
    if (has_non_standard_folder(library_.apps[i], &suggested)) {
      to_organize.push_back({i, suggested});
    }
  }

  if (to_organize.empty()) {
    toast(i18n::t("toast.organize_ok"));
    return;
  }

  std::string summary = std::string(i18n::t("confirm.organize_folder_title")) + ":\n\n";
  for (const auto& item : to_organize) {
    const AppEntry& a = library_.apps[item.first];
    summary += "• " + paths::basename(a.install_dir()) + "  ->  " + item.second + "\n";
  }

  show_confirm(i18n::t("confirm.organize_folder_title"), summary, i18n::t("btn.organize_folder"),
               [this, to_organize]() {
                 int count = 0;
                 for (const auto& item : to_organize) {
                   if (item.first < library_.apps.size()) {
                     std::string err;
                     if (organize_decomp_folder(library_.apps[item.first], item.second, &err)) {
                       ++count;
                     }
                   }
                 }
                 save_library();
                 toast(str::format("%d %s", count, i18n::t("toast.organize_ok")));
               });
}

void App::draw_settings_screen() {
  const int origin_x = theme_.sidebar_width;
  const int origin_y = theme_.topbar_height + theme_.tab_bar_height;
  const int content_w = renderer_.width() - origin_x;

  auto yes_no_str = [](bool v) { return v ? i18n::t("val.yes") : i18n::t("val.no"); };

  std::vector<SettingsRow> rows;
  rows.push_back({i18n::t("settings.unverified"), yes_no_str(settings_.DelanxIncludeUnverified),
                  i18n::t("settings.unverified_hint")});
  rows.push_back({i18n::t("settings.confirm_blocked"), yes_no_str(settings_.DelanxConfirmBlockedReleases),
                  i18n::t("settings.confirm_blocked_hint")});
  rows.push_back({i18n::t("settings.mrship"), yes_no_str(settings_.DelanxMrShipSection),
                  i18n::t("settings.mrship_hint")});
  rows.push_back({i18n::t("settings.native_paths"), yes_no_str(settings_.DelanxUseNativeInstallPaths),
                  i18n::t("settings.native_paths_hint")});
  rows.push_back({i18n::t("settings.autocheck"), yes_no_str(settings_.DelanxAutoCheckUpdatesOnBoot),
                  i18n::t("settings.autocheck_hint")});
  rows.push_back({i18n::t("settings.grid_view"), yes_no_str(settings_.UseGridView),
                  i18n::t("settings.grid_view_hint")});
  rows.push_back({i18n::t("settings.grid_columns"), str::format("%d", grid_columns_),
                  i18n::t("settings.grid_columns_hint")});
  rows.push_back({i18n::t("settings.theme"), settings_.is_dark() ? i18n::t("val.dark") : i18n::t("val.light"),
                  i18n::t("settings.theme_hint")});
  rows.push_back({i18n::t("settings.language"), (settings_.DelanxLanguage == "en") ? i18n::t("val.lang_en") : i18n::t("val.lang_es"),
                  i18n::t("settings.language_hint")});
  rows.push_back({i18n::t("settings.organize_folders"), "",
                  i18n::t("settings.organize_folders_hint")});
  rows.push_back({i18n::t("settings.github_token"), settings_.GitHubApiToken.empty() ? i18n::t("val.not_configured") : i18n::t("val.configured"),
                  i18n::t("settings.github_token_hint")});
  rows.push_back({i18n::t("settings.gitlab_token"), settings_.GitLabApiToken.empty() ? i18n::t("val.not_configured") : i18n::t("val.configured"),
                  i18n::t("settings.gitlab_token_hint")});
  rows.push_back({i18n::t("settings.check_updates_now"), "",
                  i18n::t("settings.check_updates_now_hint")});
  rows.push_back({i18n::t("settings.system_info"), "",
                  i18n::t("settings.system_info_hint")});
  rows.push_back({i18n::t("settings.input_test"), "",
                  i18n::t("settings.input_test_hint")});

  focus_ = std::clamp(focus_, 0, static_cast<int>(rows.size()) - 1);
  const int content_h = renderer_.height() - origin_y - theme_.footer_height;
  const int row_h = theme_.px(54);
  const int max_visible = std::max(1, (content_h - theme_.px(40)) / row_h);

  if (focus_ < scroll_) scroll_ = focus_;
  if (focus_ >= scroll_ + max_visible) scroll_ = focus_ - max_visible + 1;
  scroll_ = std::clamp(scroll_, 0, std::max(0, static_cast<int>(rows.size()) - max_visible));

  int y = origin_y + theme_.px(20);
  const size_t end_idx = std::min(rows.size(), static_cast<size_t>(scroll_ + max_visible));
  for (size_t index = static_cast<size_t>(scroll_); index < end_idx; ++index) {
    const bool focused = static_cast<int>(index) == focus_;
    const int item_y = y;
    // Zona pulsable de la fila, para la pantalla tactil.
    hit_targets_.push_back({static_cast<int>(index),
                            SDL_Rect{origin_x, item_y - theme_.px(4), content_w - theme_.content_padding, row_h - theme_.px(6)}});
    if (focused) {
      renderer_.fill_round_rect(origin_x + theme_.content_padding - theme_.px(8), item_y - theme_.px(4),
                                content_w - theme_.content_padding * 2 + theme_.px(16), row_h - theme_.px(6),
                                theme_.px(10), theme_.panel_alt);
      renderer_.fill_round_rect(origin_x + theme_.content_padding - theme_.px(8), item_y - theme_.px(4),
                                theme_.px(4), row_h - theme_.px(6), theme_.px(2), theme_.accent);
      renderer_.stroke_round_rect(origin_x + theme_.content_padding - theme_.px(8), item_y - theme_.px(4),
                                  content_w - theme_.content_padding * 2 + theme_.px(16), row_h - theme_.px(6),
                                  theme_.px(10), theme_.focus_ring, 2);
    }
    const int right_bound = origin_x + content_w - theme_.content_padding - theme_.px(16);
    int value_w = 0;
    if (!rows[index].value.empty()) {
      value_w = renderer_.text_width(rows[index].value, theme_.font_body, true, FontFace::Condensed);
      renderer_.draw_text_right(rows[index].value, right_bound, item_y + theme_.px(8),
                                focused ? theme_.accent : theme_.text_dim, theme_.font_body, true, FontFace::Condensed);
    }

    const int label_max_w = right_bound - value_w - theme_.px(24) - (origin_x + theme_.content_padding);
    const std::vector<std::string> label_lines = renderer_.wrap_text(rows[index].label, theme_.font_body, label_max_w, focused, FontFace::Brand);
    if (!label_lines.empty()) {
      renderer_.draw_text(label_lines[0], origin_x + theme_.content_padding, item_y + theme_.px(2),
                          focused ? theme_.text : theme_.text_dim, theme_.font_body, focused, FontFace::Brand);
    }

    const std::vector<std::string> hint_lines = renderer_.wrap_text(rows[index].hint, theme_.font_small, label_max_w, false, FontFace::Regular);
    if (!hint_lines.empty()) {
      renderer_.draw_text(hint_lines[0], origin_x + theme_.content_padding, item_y + theme_.px(26),
                          theme_.text_muted, theme_.font_small, false, FontFace::Regular);
    }
    y += row_h;
  }

  // Barra de scroll vertical si hay mas opciones de las visibles
  if (static_cast<int>(rows.size()) > max_visible) {
    const int track_x = origin_x + content_w - theme_.px(16);
    const int track_y = origin_y + theme_.px(20);
    const int track_h = max_visible * row_h;
    const int thumb_h = std::max(theme_.px(30), track_h * max_visible / static_cast<int>(rows.size()));
    const int max_scroll = static_cast<int>(rows.size()) - max_visible;
    const int thumb_y = track_y + (track_h - thumb_h) * scroll_ / (max_scroll > 0 ? max_scroll : 1);
    renderer_.fill_round_rect(track_x, track_y, theme_.px(4), track_h, theme_.px(2), theme_.panel_alt);
    renderer_.fill_round_rect(track_x, thumb_y, theme_.px(4), thumb_h, theme_.px(2), theme_.accent);
  }
}

void App::activate_settings_item() {
  switch (focus_) {
    case 0:
      settings_.DelanxIncludeUnverified = !settings_.DelanxIncludeUnverified;
      hide_unsupported_switch_ = !settings_.DelanxIncludeUnverified;
      break;
    case 1:
      settings_.DelanxConfirmBlockedReleases = !settings_.DelanxConfirmBlockedReleases;
      break;
    case 2:
      settings_.DelanxMrShipSection = !settings_.DelanxMrShipSection;
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
    case 7: {
      const bool currently_dark = settings_.is_dark();
      settings_.ThemeMode = currently_dark ? ThemeTextMode::Light : ThemeTextMode::Dark;
      settings_.DelanxTheme = currently_dark ? "light" : "dark";
      theme_ = settings_.is_dark() ? Theme::delanx_dark() : Theme::delanx_light();
      theme_.apply_scale(static_cast<float>(renderer_.width()) / 1280.0f);
      toast(settings_.is_dark() ? "Tema oscuro activado" : "Tema claro activado");
      break;
    }
    case 8: {
      if (settings_.DelanxLanguage == "en") {
        settings_.DelanxLanguage = "es";
        i18n::set_language_code("es");
        toast("Idioma: Español");
      } else {
        settings_.DelanxLanguage = "en";
        i18n::set_language_code("en");
        toast("Language: English");
      }
      save_settings();
      break;
    }
    case 9:
      organize_all_decomps();
      return;
    case 10: {
      std::string token;
      if (platform::text_input("Token de GitHub (vacio para borrar)", settings_.GitHubApiToken, &token, 128, true)) {
        settings_.GitHubApiToken = str::trim(token);
        toast(settings_.GitHubApiToken.empty() ? "Token de GitHub borrado" : "Token de GitHub guardado");
      }
      break;
    }
    case 11: {
      std::string token;
      if (platform::text_input("Token de GitLab (vacio para borrar)", settings_.GitLabApiToken, &token, 128, true)) {
        settings_.GitLabApiToken = str::trim(token);
        toast(settings_.GitLabApiToken.empty() ? "Token de GitLab borrado" : "Token de GitLab guardado");
      }
      break;
    }
    case 12:
      check_updates();
      return;
    case 13:
      open_dialog("Informacion de la consola",
                  str::format("Firmware: %s\nOrigen de arranque: %s\nModo applet: %s\nMemoria: %s\nDatos: %s\n"
                              "Catalogo local: %lu entradas\nCatalogo maestro: %lu entradas (sin verificar)\n"
                              "MrShip / Ports: %lu entradas\n\n"
                              "Delanx %s — launcher homebrew para Switch. Lee y escribe apps.json en el mismo "
                              "formato que Quiver Launcher.",
                              platform::firmware_version().c_str(), platform::boot_origin().c_str(),
                              yes_no(applet_mode_).c_str(), platform::heap_usage_human().c_str(),
                              paths::root().c_str(),
                              static_cast<unsigned long>(local_catalog_.entries.size()),
                              static_cast<unsigned long>(master_catalog_.entries.size()),
                              static_cast<unsigned long>(mrship_catalog_.entries.size()), DELANX_VERSION),
                  {DialogOption{"Cerrar", nullptr}});
      return;
    case 14:
      controller_test_open_ = true;
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
  if (installing_active_) {
    toast(i18n::t("notify.busy_warning"), true);
    return false;
  }
  installing_active_ = true;

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
    if (now - last_draw < 60 && done != total) return true;
    last_draw = now;
    const float ratio = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.0f;

    // Dibujo normal no bloqueante de la pantalla de fondo
    draw_background();
    if (tab_ == Tab::Library) {
      draw_list_screen(i18n::t("tab.library"), "", build_library_items(), i18n::t("empty.library"));
    } else if (tab_ == Tab::Catalog) {
      draw_list_screen(catalog_source_title(), catalog_source_origin(), build_catalog_items(), catalog_empty_text());
    } else if (tab_ == Tab::MrShip) {
      draw_list_screen("Harbour Masters", "Ports N64", build_mrship_items(), i18n::t("empty.mrship"));
    } else if (tab_ == Tab::Homebrew) {
      draw_list_screen(i18n::t("tab.homebrew"), "/switch", build_homebrew_items(), i18n::t("empty.homebrew"));
    } else if (tab_ == Tab::Downloads) {
      draw_downloads_screen();
    } else if (tab_ == Tab::Settings) {
      draw_settings_screen();
    }
    draw_sidebar();
    draw_topbar();
    draw_footer();

    // Notificacion emergente elegante en la esquina superior derecha
    const int card_w = theme_.px(440);
    const int card_h = theme_.px(94);
    const int card_x = renderer_.width() - card_w - theme_.px(24);
    const int card_y = theme_.topbar_height + theme_.px(14);

    renderer_.fill_round_rect(card_x, card_y, card_w, card_h, theme_.px(14), theme_.panel);
    renderer_.stroke_round_rect(card_x, card_y, card_w, card_h, theme_.px(14), theme_.accent, 2);

    const int icon_size = theme_.px(60);
    const int icon_x = card_x + theme_.px(14);
    const int icon_y = card_y + theme_.px(17);
    renderer_.fill_round_rect(icon_x, icon_y, icon_size, icon_size, theme_.px(10), theme_.panel_alt);
    Texture* icon = icon_for(app);
    if (!icon) icon = renderer_.texture("romfs:/icons/delanx-icon.png");
    if (icon) {
      renderer_.draw_texture_fit(icon, icon_x, icon_y, icon_size, icon_size);
    } else {
      renderer_.draw_text_centered(str::initials(app.name), icon_x + icon_size / 2,
                                   icon_y + icon_size / 2 - theme_.px(12), theme_.text_muted,
                                   theme_.font_title, true);
    }

    const int text_x = icon_x + icon_size + theme_.px(14);
    const int text_max_w = card_w - (text_x - card_x) - theme_.px(14);
    const std::string action_title = std::string(i18n::t("notify.installing")) + ": " + display_name(app);
    const std::vector<std::string> title_lines = renderer_.wrap_text(action_title, theme_.font_body, text_max_w, true, FontFace::Brand);
    if (!title_lines.empty()) {
      renderer_.draw_text(title_lines[0], text_x, card_y + theme_.px(14), theme_.text, theme_.font_body, true, FontFace::Brand);
    }

    const int bar_x = text_x;
    const int bar_y = card_y + theme_.px(42);
    const int bar_w = text_max_w;
    const int bar_h = theme_.px(10);
    renderer_.fill_round_rect(bar_x, bar_y, bar_w, bar_h, bar_h / 2, theme_.panel_alt);
    if (ratio > 0.001f) {
      const int fill_w = std::clamp(static_cast<int>(bar_w * ratio), bar_h, bar_w);
      renderer_.fill_round_rect(bar_x, bar_y, fill_w, bar_h, bar_h / 2, theme_.accent);

      renderer_.push_clip(bar_x, bar_y, fill_w, bar_h);
      const uint32_t ticks = SDL_GetTicks();
      const int shimmer_w = theme_.px(40);
      const int shimmer_range = fill_w + shimmer_w * 2;
      const int shimmer_pos = bar_x - shimmer_w + static_cast<int>((ticks / 3) % std::max(shimmer_range, 1));
      renderer_.fill_round_rect(shimmer_pos, bar_y, shimmer_w, bar_h, bar_h / 2,
                                Color::rgb(255, 255, 255, 90));
      renderer_.pop_clip();
    }

    const std::string bytes_str = str::format("%s / %s", str::human_size(static_cast<uint64_t>(done)).c_str(),
                                              str::human_size(static_cast<uint64_t>(total)).c_str());
    renderer_.draw_text(bytes_str, bar_x, card_y + theme_.px(60), theme_.text_muted, theme_.font_small);
    const std::string pct_str = str::format("%d%%", static_cast<int>(ratio * 100.0f));
    renderer_.draw_text_right(pct_str, card_x + card_w - theme_.px(16), card_y + theme_.px(60),
                              theme_.accent, theme_.font_small, true);

    renderer_.end_frame();

    input_.update();
    handle_input();
    if (input_.pressed(Btn::B)) return false;
    return true;
  };

  const InstallReport report = install_release(request, progress);
  installing_active_ = false;

  DownloadItem item;
  item.app_name = display_name(app);
  item.version = release.tag;
  item.ok = report.status == InstallStatus::Ok;
  item.status = report.message.empty() ? (item.ok ? "instalado" : "fallo") : report.message;
  downloads_.insert(downloads_.begin(), item);

  if (report.status == InstallStatus::Ok) {
    toast(str::format("%s %s %s", display_name(app).c_str(), release.tag.c_str(), i18n::t("badge.installed")));
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
  toast(report.message.empty() ? i18n::t("info.install_failed") : report.message, true);
  return false;
}

void App::install_app(int library_index, bool force_unverified) {
  if (installing_active_) {
    toast(i18n::t("notify.busy_warning"), true);
    return;
  }
  if (library_index < 0 || library_index >= static_cast<int>(library_.apps.size())) return;
  const AppEntry app = library_.apps[static_cast<size_t>(library_index)];

  begin_busy(std::string(i18n::t("info.checking")) + ": " + display_name(app));
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
                     focus_ = 10;  // fila del token de GitHub
                   }},
                   DialogOption{i18n::t("btn.close"), nullptr}});
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

  if (app.installed) {
    const bool has_update = app.update_available_for(release.tag);
    if (!has_update) {
      const std::string body = str::format(i18n::t("dialog.up_to_date_body"),
                                           display_name(app).c_str(),
                                           app.installed_version.empty() ? release.tag.c_str() : app.installed_version.c_str());
      open_dialog(i18n::t("dialog.up_to_date_title"), body,
                  {DialogOption{i18n::t("btn.reinstall"), [this, library_index, release, asset, expected]() {
                     run_install(library_.apps[static_cast<size_t>(library_index)], release, asset, expected);
                   }},
                   DialogOption{i18n::t("btn.cancel"), nullptr}});
      return;
    } else {
      const std::string body = str::format(i18n::t("dialog.update_available_body"),
                                           display_name(app).c_str(),
                                           app.installed_version.empty() ? "-" : app.installed_version.c_str(),
                                           release.tag.c_str(),
                                           asset.name.c_str());
      open_dialog(i18n::t("dialog.update_available_title"), body,
                  {DialogOption{i18n::t("btn.update"), [this, library_index, release, asset, expected]() {
                     run_install(library_.apps[static_cast<size_t>(library_index)], release, asset, expected);
                   }},
                   DialogOption{i18n::t("btn.cancel"), nullptr}});
      return;
    }
  }

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

void App::launch_nro(const std::string& nro_path) {
  if (nro_path.empty()) {
    toast("Ruta del ejecutable no valida", true);
    return;
  }
  LaunchTarget target;
  target.nro_path = nro_path;
  std::string error;
  if (!request_next_nro(target, &error)) {
    toast(error.empty() ? "No se pudo encadenar el NRO" : error, true);
    return;
  }
  launching_ = true;
}

void App::uninstall_app(int library_index) {
  if (library_index < 0 || library_index >= static_cast<int>(library_.apps.size())) return;
  const AppEntry app = library_.apps[static_cast<size_t>(library_index)];
  const std::string title = std::string(i18n::t("confirm.uninstall_app_title")) + display_name(app);
  const std::string body = std::string(i18n::t("confirm.uninstall_app_body_1")) + app.install_dir() +
                           std::string(i18n::t("confirm.uninstall_app_body_2"));
  show_confirm(title, body, i18n::t("btn.uninstall"), [this, app]() {
    std::string error;
    if (!::delanx::uninstall_app(app, false, &error)) {
      toast(error.empty() ? i18n::t("toast.uninstall_failed") : error, true);
      return;
    }
    for (size_t index = 0; index < library_.apps.size(); ++index) {
      if (library_.apps[index].identity_key() == app.identity_key()) {
        library_.apps.erase(library_.apps.begin() + index);
        break;
      }
    }
    save_library();
    details_open_ = false;
    details_is_homebrew_ = false;
    toast(std::string(i18n::t("toast.uninstall_ok")) + display_name(app));
  });
}

void App::uninstall_homebrew(const HomebrewAppInfo& hb) {
  const std::string title = std::string(i18n::t("confirm.uninstall_hb_title")) + hb.name;
  const std::string body = std::string(i18n::t("confirm.uninstall_hb_body")) + hb.nro_path;
  show_confirm(title, body, i18n::t("btn.delete"), [this, hb]() {
    bool ok = false;
    const std::string switch_root = paths::to_sd_path("/switch");
    if (!hb.folder_path.empty() && hb.folder_path != switch_root &&
        str::starts_with(hb.folder_path, switch_root) && paths::is_dir(hb.folder_path)) {
      ok = paths::remove_dir_recursive(hb.folder_path);
    } else if (paths::exists(hb.nro_path)) {
      ok = paths::remove_file(hb.nro_path);
    }
    if (!ok) {
      toast(i18n::t("toast.delete_hb_failed"), true);
      return;
    }
    details_open_ = false;
    details_is_homebrew_ = false;
    refresh_homebrew();
    toast(std::string(i18n::t("toast.delete_hb_ok")) + hb.name);
  });
}

void App::draw_controller_test_overlay() {
  const int overlay_w = renderer_.width();
  const int overlay_h = renderer_.height();
  renderer_.fill_rect(0, 0, overlay_w, overlay_h, theme_.overlay);

  const int width = std::min(theme_.px(1080), overlay_w - theme_.px(40));
  const int height = std::min(theme_.px(620), overlay_h - theme_.px(40));
  const int x = (overlay_w - width) / 2;
  const int y = (overlay_h - height) / 2;

  renderer_.fill_round_rect(x, y, width, height, theme_.px(16), theme_.panel);
  renderer_.stroke_round_rect(x, y, width, height, theme_.px(16), theme_.border, 1);

  // Cabecera
  renderer_.draw_text("Test de Mandos y Pantalla Tactil", x + theme_.px(28), y + theme_.px(20),
                      theme_.text, theme_.font_title, true);
  renderer_.draw_text("Comprueba cada boton, cruceta, sticks analogicos y pantalla tactil. Pulsa B o + para salir.",
                      x + theme_.px(28), y + theme_.px(54), theme_.text_muted, theme_.font_small);

  struct ButtonVisual {
    Btn btn;
    const char* label;
    int col;
    int row;
  };
  const ButtonVisual b_visuals[] = {
    {Btn::A, "A", 0, 0},
    {Btn::B, "B", 0, 1},
    {Btn::X, "X", 0, 2},
    {Btn::Y, "Y", 0, 3},

    {Btn::Up, "Arriba", 1, 0},
    {Btn::Down, "Abajo", 1, 1},
    {Btn::Left, "Izquierda", 1, 2},
    {Btn::Right, "Derecha", 1, 3},

    {Btn::L, "L", 2, 0},
    {Btn::R, "R", 2, 1},
    {Btn::ZL, "ZL", 2, 2},
    {Btn::ZR, "ZR", 2, 3},

    {Btn::StickLPress, "Stick L Click", 3, 0},
    {Btn::StickRPress, "Stick R Click", 3, 1},
    {Btn::Minus, "Minus (-)", 3, 2},
    {Btn::Plus, "Plus (+)", 3, 3},
  };

  const int grid_x = x + theme_.px(28);
  const int grid_y = y + theme_.px(90);
  const int col_w = theme_.px(160);
  const int row_h = theme_.px(42);

  for (const auto& bv : b_visuals) {
    const bool held = input_.held(bv.btn);
    const int bx = grid_x + bv.col * col_w;
    const int by = grid_y + bv.row * row_h;
    renderer_.fill_round_rect(bx, by, col_w - theme_.px(12), row_h - theme_.px(8),
                              theme_.px(8), held ? theme_.success : theme_.panel_alt);
    if (held) {
      renderer_.stroke_round_rect(bx, by, col_w - theme_.px(12), row_h - theme_.px(8),
                                  theme_.px(8), theme_.focus_ring, 2);
    }
    renderer_.draw_text_centered(bv.label, bx + (col_w - theme_.px(12)) / 2, by + theme_.px(8),
                                 held ? theme_.bg : theme_.text, theme_.font_body, held);
  }

  // Sticks analogicos y panel tactil
  const int stick_panel_y = grid_y + 4 * row_h + theme_.px(14);
  const int stick_box_size = theme_.px(120);

  int stick_lx = 0;
  int stick_ly = 0;
  int stick_rx = 0;
  int stick_ry = 0;
#if defined(__SWITCH__)
  platform::PadSnapshot snap;
  if (platform::pad_poll(&snap)) {
    stick_lx = snap.stick_lx;
    stick_ly = snap.stick_ly;
    stick_rx = snap.stick_rx;
    stick_ry = snap.stick_ry;
  }
#endif

  // Stick Izquierdo
  const int stick_l_box_x = x + theme_.px(28);
  renderer_.fill_round_rect(stick_l_box_x, stick_panel_y, stick_box_size, stick_box_size,
                            theme_.px(10), theme_.panel_alt);
  renderer_.stroke_round_rect(stick_l_box_x, stick_panel_y, stick_box_size, stick_box_size,
                              theme_.px(10), theme_.border, 1);
  renderer_.draw_text("Stick L", stick_l_box_x + theme_.px(10), stick_panel_y + theme_.px(6),
                      theme_.text_muted, theme_.font_small);

  const int dot_l_cx = stick_l_box_x + stick_box_size / 2 + (stick_lx * (stick_box_size / 2 - 14) / 32767);
  const int dot_l_cy = stick_panel_y + stick_box_size / 2 - (stick_ly * (stick_box_size / 2 - 14) / 32767);
  renderer_.stroke_circle(stick_l_box_x + stick_box_size / 2, stick_panel_y + stick_box_size / 2,
                          theme_.px(14), theme_.border, 1);
  renderer_.fill_circle(dot_l_cx, dot_l_cy, theme_.px(8), theme_.accent);

  const std::string l_coords = str::format("X: %+6d\nY: %+6d", stick_lx, stick_ly);
  renderer_.draw_text(l_coords, stick_l_box_x + stick_box_size + theme_.px(12), stick_panel_y + theme_.px(28),
                      theme_.text, theme_.font_small);

  // Stick Derecho
  const int stick_r_box_x = x + theme_.px(360);
  renderer_.fill_round_rect(stick_r_box_x, stick_panel_y, stick_box_size, stick_box_size,
                            theme_.px(10), theme_.panel_alt);
  renderer_.stroke_round_rect(stick_r_box_x, stick_panel_y, stick_box_size, stick_box_size,
                              theme_.px(10), theme_.border, 1);
  renderer_.draw_text("Stick R", stick_r_box_x + theme_.px(10), stick_panel_y + theme_.px(6),
                      theme_.text_muted, theme_.font_small);

  const int dot_r_cx = stick_r_box_x + stick_box_size / 2 + (stick_rx * (stick_box_size / 2 - 14) / 32767);
  const int dot_r_cy = stick_panel_y + stick_box_size / 2 - (stick_ry * (stick_box_size / 2 - 14) / 32767);
  renderer_.stroke_circle(stick_r_box_x + stick_box_size / 2, stick_panel_y + stick_box_size / 2,
                          theme_.px(14), theme_.border, 1);
  renderer_.fill_circle(dot_r_cx, dot_r_cy, theme_.px(8), theme_.accent_alt);

  const std::string r_coords = str::format("X: %+6d\nY: %+6d", stick_rx, stick_ry);
  renderer_.draw_text(r_coords, stick_r_box_x + stick_box_size + theme_.px(12), stick_panel_y + theme_.px(28),
                      theme_.text, theme_.font_small);

  // Pantalla Tactil
  const int touch_box_x = x + theme_.px(690);
  const int touch_box_w = width - theme_.px(718);
  renderer_.fill_round_rect(touch_box_x, stick_panel_y, touch_box_w, stick_box_size,
                            theme_.px(10), theme_.panel_alt);
  renderer_.stroke_round_rect(touch_box_x, stick_panel_y, touch_box_w, stick_box_size,
                              theme_.px(10), theme_.border, 1);
  renderer_.draw_text("Pantalla Tactil", touch_box_x + theme_.px(14), stick_panel_y + theme_.px(8),
                      theme_.text_muted, theme_.font_small);
  const bool touching = input_.held(Btn::Touch);
  renderer_.draw_text(touching ? "ESTADO: TOCANDO" : "ESTADO: SIN TOQUE",
                      touch_box_x + theme_.px(14), stick_panel_y + theme_.px(38),
                      touching ? theme_.success : theme_.text_dim, theme_.font_body, touching);
  if (touching) {
    renderer_.draw_text(str::format("X: %d  |  Y: %d", input_.pointer_x(), input_.pointer_y()),
                        touch_box_x + theme_.px(14), stick_panel_y + theme_.px(72),
                        theme_.accent, theme_.font_small);
  }

  // Pie del modal
  const int footer_modal_y = y + height - theme_.px(48);
  renderer_.draw_text("Pulsa B o + para salir del test de mandos",
                      x + theme_.px(28), footer_modal_y + theme_.px(10),
                      theme_.text_muted, theme_.font_body);
}

void App::check_homebrew_update(const HomebrewAppInfo& hb) {
  if (installing_active_) {
    toast(i18n::t("notify.busy_warning"), true);
    return;
  }
  if (hb.github_repo.empty()) {
    toast("Esta app no tiene repositorio GitHub asociado para comprobar actualizaciones");
    return;
  }
  AppEntry temp_app;
  temp_app.name = hb.name;
  temp_app.repository = hb.github_repo;
  temp_app.repository_source = "github";
  temp_app.installed_version = hb.version;
  temp_app.sw.target_dir = hb.folder_path;
  temp_app.sw.nro = paths::basename(hb.nro_path);
  temp_app.sw.home_dir = hb.folder_path;

  begin_busy("Comprobando " + hb.name);
  Release release;
  ReleaseAsset asset;
  std::string error;
  const bool ok = fetch_candidate_release(temp_app, &release, &asset, &error);
  end_busy();

  if (!ok) {
    open_dialog("Comprobación de actualización",
                "No se pudo consultar el repositorio " + hb.github_repo + ": " + error,
                {DialogOption{i18n::t("btn.understood"), nullptr}});
    return;
  }

  const bool has_update = temp_app.update_available_for(release.tag);
  if (!has_update && !hb.version.empty()) {
    const std::string body = str::format(i18n::t("dialog.up_to_date_body"),
                                         hb.name.c_str(), hb.version.c_str());
    open_dialog(i18n::t("dialog.up_to_date_title"), body,
                {DialogOption{i18n::t("btn.reinstall"), [this, temp_app, release, asset, hb]() {
                   run_install(temp_app, release, asset, "");
                   refresh_homebrew();
                   toast(hb.name + ": " + i18n::t("notify.completed"));
                 }},
                 DialogOption{i18n::t("btn.cancel"), nullptr}});
    return;
  }

  const std::string body = str::format(i18n::t("dialog.update_available_body"),
                                       hb.name.c_str(),
                                       hb.version.empty() ? "-" : hb.version.c_str(),
                                       release.tag.c_str(),
                                       asset.name.c_str());
  open_dialog(i18n::t("dialog.update_available_title"), body,
              {DialogOption{i18n::t("btn.update"), [this, temp_app, release, asset, hb]() {
                 run_install(temp_app, release, asset, "");
                 refresh_homebrew();
                 toast(hb.name + ": " + i18n::t("notify.completed"));
               }},
               DialogOption{i18n::t("btn.cancel"), nullptr}});
}

}  // namespace delanx::ui
