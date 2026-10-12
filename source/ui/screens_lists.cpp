// Delanx — pantallas de navegacion: biblioteca, catalogo (local y Quiver) y Harbour Masters.
#include <algorithm>

#include "core/i18n.hpp"
#include "core/paths.hpp"
#include "core/strings.hpp"
#include "platform/switch_platform.hpp"
#include "platform/text_input.hpp"
#include "services/artwork.hpp"
#include "services/http.hpp"
#include "services/nro_inspector.hpp"
#include "ui/app.hpp"

namespace delanx::ui {
namespace {

constexpr int kChipBarHeight = 46;

Color badge_color(const Theme& theme, const ListItem& item) {
  if (item.unavailable) return theme.warning;
  if (item.unverified) return theme.warning;  // catalogo maestro: sin verificar
  if (item.mrship) return theme.accent_alt;
  if (item.update) return theme.warning;
  if (item.installed) return theme.success;
  if (item.verified) return theme.accent;
  return theme.text_muted;
}

using str::initials;

}  // namespace

void App::draw_tab_bar() {
  const int origin_x = theme_.sidebar_width;
  const int y = theme_.topbar_height;
  const int width = renderer_.width() - origin_x;
  const int height = theme_.tab_bar_height;

  // Fondo sutil con linea separadora inferior
  renderer_.fill_rect(origin_x, y, width, height, theme_.panel.with_alpha(180));
  renderer_.fill_rect(origin_x, y + height - 1, width, 1, theme_.border);

  struct TabDesc {
    Tab tab;
    const char* label;
  };

  const TabDesc tabs[] = {
      {Tab::Library, i18n::tr("tab.library")},
      {Tab::Catalog, i18n::tr("tab.catalog")},
      {Tab::MrShip, i18n::tr("tab.mrship")},
      {Tab::Homebrew, i18n::tr("tab.homebrew")},
      {Tab::Downloads, i18n::tr("tab.downloads")},
      {Tab::Settings, i18n::tr("tab.settings")},
  };

  int cursor_x = origin_x + theme_.content_padding;
  const int pill_y = y + theme_.px(6);
  const int pill_h = height - theme_.px(12);

  for (size_t i = 0; i < sizeof(tabs) / sizeof(tabs[0]); ++i) {
    const Tab t = tabs[i].tab;
    const bool active = (tab_ == t);
    const bool hovered = (in_tab_bar_ && tab_hover_ == static_cast<int>(i));

    const int text_w = renderer_.text_width(tabs[i].label, theme_.font_body, active, FontFace::Brand);
    const int pill_w = text_w + theme_.px(28);

    const SDL_Rect hit_rect{cursor_x, y, pill_w, height};
    hit_tabs_.push_back({t, hit_rect});

    if (active) {
      // Píldora activa con relieve y gradiente suave neon
      renderer_.vertical_gradient(cursor_x, pill_y, pill_w, pill_h,
                                  theme_.accent.with_alpha(80), theme_.accent.with_alpha(35), 6);
      renderer_.stroke_round_rect(cursor_x, pill_y, pill_w, pill_h, theme_.px(8),
                                  theme_.accent, 2);
      // Linea de acento luminosa inferior
      renderer_.fill_round_rect(cursor_x + theme_.px(8), pill_y + pill_h - theme_.px(3),
                                pill_w - theme_.px(16), theme_.px(3), theme_.px(1), theme_.accent);
      renderer_.draw_text(tabs[i].label, cursor_x + theme_.px(14), pill_y + theme_.px(6),
                          theme_.text, theme_.font_body, true, FontFace::Brand);
    } else if (hovered) {
      // Estado hover/seleccionado con D-Pad / stick izquierdo antes de pulsar A
      renderer_.vertical_gradient(cursor_x, pill_y, pill_w, pill_h,
                                  theme_.card_focus, theme_.panel_alt, 4);
      renderer_.stroke_round_rect(cursor_x, pill_y, pill_w, pill_h, theme_.px(8),
                                  theme_.focus_ring.with_alpha(180), 2);
      renderer_.draw_text(tabs[i].label, cursor_x + theme_.px(14), pill_y + theme_.px(6),
                          theme_.text, theme_.font_body, false, FontFace::Brand);
    } else {
      // Píldora normal
      renderer_.draw_text(tabs[i].label, cursor_x + theme_.px(14), pill_y + theme_.px(6),
                          theme_.text_dim, theme_.font_body, false, FontFace::Brand);
    }

    cursor_x += pill_w + theme_.px(10);
  }
}

// ------------------------------------------------------------------ favoritos

bool App::is_favorite(const std::string& identity) const {
  if (identity.empty()) return false;
  for (const auto& fav : settings_.DelanxFavorites) {
    if (str::iequals(fav, identity)) return true;
  }
  return false;
}

void App::toggle_favorite(const std::string& identity) {
  if (identity.empty()) return;
  auto it = std::find_if(settings_.DelanxFavorites.begin(), settings_.DelanxFavorites.end(),
                         [&](const std::string& item) { return str::iequals(item, identity); });
  if (it != settings_.DelanxFavorites.end()) {
    settings_.DelanxFavorites.erase(it);
  } else {
    settings_.DelanxFavorites.push_back(identity);
  }
  save_settings();
}

// ------------------------------------------------------------------ construccion de listas

std::vector<ListItem> App::build_library_items() {
  std::vector<ListItem> items;
  for (size_t index = 0; index < library_.apps.size(); ++index) {
    const AppEntry& app = library_.apps[index];
    if (!app.installed) continue;  // Solo los decompilados/ports instalados en la consola
    if (library_filter_ == 1 && !app.update_available) continue;
    if (library_filter_ == 2 && !app.mrship) continue;

    ListItem item;
    item.title = display_name(app);
    item.subtitle = app.project.empty() ? app.repository : app.project;
    item.icon_url = app.app_icon_url;
    item.installed = app.installed;
    item.update = app.update_available;
    item.mrship = app.mrship;
    item.verified = app.verified_entry;
    item.library_index = static_cast<int>(index);
    if (app.update_available) {
      item.badge = "Actualiza";
    } else if (app.installed) {
      item.badge = "Instalado";
    } else if (app.manually_managed()) {
      item.badge = "Manual";
    }
    item.meta = app.installed_version.empty() ? app.latest_version : app.installed_version;
    const size_t missing = app.missing_user_files().size();
    if (missing > 0) {
      item.meta += str::format(" | faltan %lu ficheros", static_cast<unsigned long>(missing));
    }
    items.push_back(item);
  }
  return items;
}

bool App::has_switch_build(const CatalogEntry& entry) const {
  if (entry.verified) return true;
  if (!entry.switch_proof.empty()) return true;

  // Si está explicitamente catalogado como unavailable por HarbourMasters, no tiene build
  for (const auto& unavail : mrship_catalog_.unavailable) {
    if (str::iequals(unavail.repository, entry.repository) ||
        str::iequals(unavail.name, entry.name)) {
      return false;
    }
  }
  for (const auto& unavail : local_catalog_.unavailable) {
    if (str::iequals(unavail.repository, entry.repository) ||
        str::iequals(unavail.name, entry.name)) {
      return false;
    }
  }

  // Comprobar si tiene NRO configurado o filtro de assets hacia switch
  const std::string filter = str::lower(entry.release_asset_filter);
  const std::string nro = str::lower(entry.sw.nro);
  if (!nro.empty() || filter.find(".nro") != std::string::npos || filter.find("switch") != std::string::npos) {
    return true;
  }

  // Lista de repositorios del catálogo maestro verificados con build de Switch
  static const char* const kKnownSwitchRepos[] = {
      "PalindromicBreadLoaf/NX-Diffuser",
      "perfect-dark-pc-port/perfect_dark",
      "fgsfdsfgs/doom64ex",
      "fildicio/jak-project-switch",
      "centollOS/SwitchWaker",
      "souldbminerr/dusklight-nx",
      "KawaiiBunga/Melee-NX",
      "new-coke/strikers",
      "StevensND/nfsmw-nx",
      "ChaykaDed/silent-hill-decomp-nx",
      "CostelaCNX/ReChan-Switch",
      "XProger/OpenLara",
      "bryanthaboi/gen1recomp",
      "UNDERdecoded/Gen2Recomped",
      "delsonazevedo/Zelda-LA-DX-HD-Updated",
      "toniisound/oracles-nx",
      "HayatoG/tmc",
      "boraeskicioglu/sonic_advance2_nx",
      "Thorhax/Daytona-NX-Modern",
      "Thorhax/VF2-NX-Modern",
      "Thorhax/Virtual-On-NX",
      "Thorhax/Killer-Instinct-NX-Modern",
      "boraeskicioglu/sonic_the_fighters_nx",
      "fgsfdsfgs/eduke32",
      "fgsfdsfgs/Serious-Engine",
      "FWGS/xash3d-fwgs",
      "NaGaa95/hl2_nx",
      "NaGaa95/max_nx_v2.1.131",
      "ryandeering/fallout-ce-switch",
      "ryandeering/fallout2-ce-switch",
      "ChanseyIsTheBest/daggerfall_nx",
      "bvschaik/julius",
      "capsterx-switch/Vanilla-Conquer",
      "ihhub/fheroes2",
      "ChanseyIsTheBest/revolt_nx",
      "mariob0y/gta3-nx",
      "NaGaa95/gtasa_nx",
      "KranKRival/OpenJazzNX",
  };
  for (const char* repo : kKnownSwitchRepos) {
    if (str::iequals(entry.repository, repo)) return true;
  }

  const std::string text = str::lower(entry.notes + " " + entry.requirements);
  if (text.find("para pc") != std::string::npos ||
      text.find("solo pc") != std::string::npos ||
      text.find("verificado actualmente para pc") != std::string::npos ||
      text.find("soporte switch en desarrollo") != std::string::npos ||
      text.find("pendiente release de binario .nro") != std::string::npos ||
      text.find("pendiente publicacion de release nro") != std::string::npos ||
      text.find("pendiente publicación de release nro") != std::string::npos ||
      text.find("pendiente de release oficial para switch") != std::string::npos ||
      text.find("pendiente release oficial de puerto switch") != std::string::npos ||
      text.find("solo linux/mac/win") != std::string::npos) {
    return false;
  }

  // Por defecto, las entradas sin confirmación de Switch quedan ocultas cuando hide_unsupported_switch_ está activo
  return false;
}

std::vector<ListItem> App::build_catalog_items() {
  std::vector<ListItem> items;
  if (catalog_mode_ == CatalogMode::Quiver) {
    for (size_t index = 0; index < catalog_online_.size(); ++index) {
      const quiver::CatalogApp& entry = catalog_online_[index];
      if (library_.find_by_identity(entry.repository) || library_.find(entry.name)) continue;
      ListItem item;
      item.title = entry.name;
      item.subtitle = entry.project_name.empty() ? entry.repository : entry.project_name;
      item.icon_url = entry.icon_url;
      item.catalog_index = static_cast<int>(index);
      item.badge = "Quiver";
      item.verified = !entry.verified_version.empty();
      item.meta = entry.verified_version;
      if (entry.repository.empty()) {
        item.badge = "Sin repo";
        item.unavailable = true;
        item.unavailable_reason = "La entrada de Quiver no publica repositorio (app gestionada a mano).";
      }
      items.push_back(item);
    }
    return items;
  }

  const Catalog& catalog = (catalog_mode_ == CatalogMode::Master && !master_catalog_.entries.empty())
                               ? master_catalog_
                               : local_catalog_;
  for (size_t index = 0; index < catalog.entries.size(); ++index) {
    const CatalogEntry& entry = catalog.entries[index];
    // Ocultar por defecto los que no tienen build para Switch
    if (hide_unsupported_switch_ && !has_switch_build(entry)) {
      continue;
    }

    const AppEntry* existing = library_.find_by_identity(entry.to_app_entry().identity_key());
    const bool is_installed = (existing != nullptr && existing->installed);
    const bool is_fav = is_favorite(entry.name) || (!entry.repository.empty() && is_favorite(entry.repository));

    // Si el filtro de favoritos está activo, omitir los no favoritos
    if (catalog_show_favorites_ && !is_fav) {
      continue;
    }

    ListItem item;
    item.title = entry.name;
    item.subtitle = entry.project.empty() ? entry.repository : entry.project;
    item.catalog_index = static_cast<int>(index);
    item.source = &entry;
    if (existing) item.library_index = static_cast<int>(existing - library_.apps.data());
    item.icon_url = entry.app_icon_url;
    item.mrship = entry.mrship;
    item.verified = entry.verified;
    item.unverified = !entry.verified;
    item.installed = is_installed;
    if (is_installed) {
      item.badge = "Instalado";
    } else if (is_fav) {
      item.badge = entry.verified ? "★ Verificado" : "★ Favorito";
    } else {
      item.badge = entry.verified ? entry.badge() : "Sin verificar";
    }
    item.meta = entry.sw.nro.empty() ? entry.requirements : entry.sw.nro;
    items.push_back(item);
  }
  return items;
}

std::vector<ListItem> App::build_mrship_items() {
  std::vector<ListItem> items;
  for (size_t index = 0; index < mrship_catalog_.entries.size(); ++index) {
    const CatalogEntry& entry = mrship_catalog_.entries[index];
    if (hide_unsupported_switch_ && !has_switch_build(entry)) {
      continue;
    }
    ListItem item;
    item.title = entry.name;
    item.subtitle = entry.project.empty() ? entry.repository : entry.project;
    item.catalog_index = static_cast<int>(index);
    item.source = &entry;
    item.icon_url = entry.app_icon_url;
    item.mrship = true;
    item.verified = entry.verified;
    const AppEntry* existing = library_.find_by_identity(entry.to_app_entry().identity_key());
    if (existing && existing->installed) {
      item.installed = true;
      item.library_index = static_cast<int>(existing - library_.apps.data());
    }
    item.badge = item.installed ? "Instalado" : (entry.verified ? "Con build Switch" : "Sin verificar");
    item.meta = entry.sw.nro.empty() ? entry.requirements : entry.sw.nro;
    items.push_back(item);
  }
  if (!hide_unsupported_switch_) {
    for (size_t index = 0; index < mrship_catalog_.unavailable.size(); ++index) {
      const UnavailableEntry& entry = mrship_catalog_.unavailable[index];
      ListItem item;
      item.title = entry.name;
      item.subtitle = entry.repository;
      item.mrship_index = static_cast<int>(index);
      item.unavailable = true;
      item.badge = "Sin build Switch";
      item.unavailable_reason = entry.reason;
      item.meta = entry.workaround.empty() ? "no disponible" : "nightly disponible";
      items.push_back(item);
    }
  }
  return items;
}

void App::refresh_homebrew() {
  homebrew_cache_ = scan_switch_homebrew();
}

std::vector<ListItem> App::build_homebrew_items() {
  if (homebrew_cache_.empty()) {
    refresh_homebrew();
  }
  std::vector<ListItem> items;
  for (size_t index = 0; index < homebrew_cache_.size(); ++index) {
    const auto& hb = homebrew_cache_[index];
    ListItem item;
    item.title = hb.name;
    item.subtitle = hb.author.empty() ? hb.folder_path : hb.author;
    item.badge = i18n::tr("badge.homebrew");
    item.installed = true;
    item.meta = hb.version.empty() ? hb.nro_path : ("v" + hb.version);
    item.icon_url = hb.icon_path;
    item.unavailable_reason = hb.github_repo;
    items.push_back(item);
  }
  return items;
}

void App::open_details_for_homebrew(const HomebrewAppInfo& hb) {
  active_homebrew_ = hb;
  details_is_homebrew_ = true;
  details_open_ = true;
}

// ------------------------------------------------------------------ dibujo de listas

void App::draw_list_screen(const std::string& title, const std::string& subtitle,
                           const std::vector<ListItem>& items, const std::string& empty_text) {
  const int origin_x = theme_.sidebar_width;
  const int origin_y = theme_.topbar_height + theme_.tab_bar_height;
  const int content_w = renderer_.width() - origin_x;
  const int content_h = renderer_.height() - origin_y - theme_.footer_height;
  const bool has_chips = (tab_ == Tab::Library || tab_ == Tab::Catalog);
  const int bottom_bar = has_chips ? theme_.px(kChipBarHeight) : 0;

  const std::string counter_str = str::format("%lu elementos", static_cast<unsigned long>(items.size()));
  const int counter_w = renderer_.text_width(counter_str, theme_.font_small);
  renderer_.draw_text_right(counter_str, renderer_.width() - theme_.content_padding, origin_y + theme_.px(8),
                            theme_.text_muted, theme_.font_small);

  const int avail_sub_w = renderer_.width() - theme_.content_padding * 2 - counter_w - theme_.px(16);
  const std::vector<std::string> sub_lines = renderer_.wrap_text(subtitle, theme_.font_small, avail_sub_w);
  if (!sub_lines.empty()) {
    renderer_.draw_text(sub_lines[0], origin_x + theme_.content_padding, origin_y + theme_.px(8), theme_.text_muted,
                        theme_.font_small);
  }

  const int grid_top = origin_y + theme_.px(34);
  const int grid_height = content_h - theme_.px(34) - bottom_bar;
  hit_targets_.clear();
  if (items.empty()) {
    renderer_.draw_text_centered(empty_text, origin_x + content_w / 2, grid_top + grid_height / 2,
                                 theme_.text_muted, theme_.font_body);
    return;
  }

  const int columns = std::max(grid_columns_, 1);
  const int gap = theme_.gap;
  const int card_w = (content_w - theme_.content_padding * 2 - gap * (columns - 1)) / columns;
  const int card_h = card_w * 9 / 16 + theme_.px(108);
  const int rows = static_cast<int>((items.size() + static_cast<size_t>(columns) - 1) / columns);

  focus_ = std::clamp(focus_, 0, static_cast<int>(items.size()) - 1);
  const int focus_row = focus_ / columns;
  const int visible_rows = std::max(1, grid_height / (card_h + gap));
  if (focus_row < scroll_) scroll_ = focus_row;
  if (focus_row >= scroll_ + visible_rows) scroll_ = focus_row - visible_rows + 1;
  scroll_ = std::max(scroll_, 0);

  renderer_.push_clip(origin_x, grid_top, content_w, grid_height);
  for (int row = scroll_; row < std::min(rows, scroll_ + visible_rows + 1); ++row) {
    for (int column = 0; column < columns; ++column) {
      const int index = row * columns + column;
      if (index >= static_cast<int>(items.size())) break;
      const ListItem& item = items[static_cast<size_t>(index)];
      const bool focused = index == focus_;
      const int card_x = origin_x + theme_.content_padding + column * (card_w + gap);
      const int card_y = grid_top + (row - scroll_) * (card_h + gap);
      hit_targets_.push_back({index, SDL_Rect{card_x, card_y, card_w, card_h}});

      renderer_.fill_round_rect(card_x, card_y, card_w, card_h, theme_.card_radius,
                                focused ? theme_.card_focus : theme_.card);
      if (focused) {
        const uint32_t ticks = SDL_GetTicks();
        const float pulse = 0.5f + 0.5f * sinf(ticks * 0.007f);
        const Color ring_color = theme_.accent.with_alpha(static_cast<uint8_t>(180 + 75 * pulse));
        renderer_.stroke_round_rect(card_x, card_y, card_w, card_h, theme_.card_radius, ring_color, 3);
      } else {
        renderer_.stroke_round_rect(card_x, card_y, card_w, card_h, theme_.card_radius, theme_.border, 1);
      }

      // Zona de caratula con conservacion de relacion de aspecto y degradado
      const int art_h = card_w * 9 / 16;
      const int art_x = card_x + theme_.px(8);
      const int art_y = card_y + theme_.px(8);
      const int art_w = card_w - theme_.px(16);
      const int art_box_h = art_h - theme_.px(8);

      renderer_.fill_round_rect(art_x, art_y, art_w, art_box_h, theme_.px(8), theme_.panel_alt);
      Texture* artwork = nullptr;
      if (item.library_index >= 0 && static_cast<size_t>(item.library_index) < library_.apps.size()) {
        artwork = icon_for(library_.apps[static_cast<size_t>(item.library_index)]);
      } else if (item.source) {
        artwork = icon_for(item.source->to_app_entry());
      } else if (!item.icon_url.empty()) {
        artwork = renderer_.texture(item.icon_url);
      }

      if (!artwork) {
        artwork = renderer_.texture("romfs:/icons/default.png");
      }

      if (artwork) {
        renderer_.draw_texture_cover(artwork, art_x, art_y, art_w, art_box_h);
        // Sutil degradado en la parte inferior de la caratula para darle profundidad
        renderer_.vertical_gradient(art_x, art_y + art_box_h - theme_.px(28), art_w, theme_.px(28),
                                   Color::rgb(0, 0, 0, 0), Color::rgb(14, 16, 22, 160), 8);
      } else {
        renderer_.draw_text_centered(initials(item.title), card_x + card_w / 2,
                                     art_y + art_box_h / 2 - theme_.px(14),
                                     theme_.text_muted, theme_.font_header, true, FontFace::Brand);
      }

      // Texto de la tarjeta con margen interior seguro
      const int text_x = card_x + theme_.px(12);
      const int text_w = card_w - theme_.px(24);
      int text_y = card_y + art_h + theme_.px(8);
      const int badge_h = theme_.px(22);
      const int badge_y = card_y + card_h - badge_h - theme_.px(10);

      // Titulo (max 2 lineas con salto de linea y elipsis de seguridad)
      const std::vector<std::string> title_lines = renderer_.wrap_text(item.title, theme_.font_body, text_w, true, FontFace::Brand);
      const int title_line_h = renderer_.font_height(theme_.font_body, true, FontFace::Brand) + 1;
      for (size_t line = 0; line < title_lines.size() && line < 2; ++line) {
        renderer_.draw_text(title_lines[line], text_x, text_y, theme_.text, theme_.font_body, true, FontFace::Brand);
        text_y += title_line_h;
      }

      // Subtitulo (solo si cabe comodamente por encima de las chapas)
      if (!item.subtitle.empty() && (text_y + renderer_.font_height(theme_.font_small) <= badge_y - theme_.px(4))) {
        const std::vector<std::string> subtitle_lines =
            renderer_.wrap_text(item.subtitle, theme_.font_small, text_w, false, FontFace::Regular);
        if (!subtitle_lines.empty()) {
          renderer_.draw_text(subtitle_lines[0], text_x, text_y, theme_.text_dim, theme_.font_small, false, FontFace::Regular);
        }
      }

      // Chapa de estado y metadato a la derecha sin colisionar
      if (!item.badge.empty()) {
        const int badge_pad = theme_.px(14);
        const int badge_w = renderer_.text_width(item.badge, theme_.font_small, true, FontFace::Condensed) + badge_pad;
        const Color color = badge_color(theme_, item);
        renderer_.fill_round_rect(card_x + theme_.px(10), badge_y, badge_w, badge_h, theme_.px(6), color.with_alpha(46));
        renderer_.stroke_round_rect(card_x + theme_.px(10), badge_y, badge_w, badge_h, theme_.px(6), color.with_alpha(100), 1);
        renderer_.draw_text(item.badge, card_x + theme_.px(10) + theme_.px(7), badge_y + theme_.px(3), color,
                           theme_.font_small, true, FontFace::Condensed);

        if (!item.meta.empty()) {
          const int avail_meta_w = card_w - theme_.px(20) - badge_w - theme_.px(12);
          if (avail_meta_w > theme_.px(32)) {
            const std::vector<std::string> meta_lines = renderer_.wrap_text(item.meta, theme_.font_small, avail_meta_w, false, FontFace::Condensed);
            if (!meta_lines.empty()) {
              renderer_.draw_text_right(meta_lines[0], card_x + card_w - theme_.px(10), badge_y + theme_.px(3),
                                        theme_.text_muted, theme_.font_small, false, FontFace::Condensed);
            }
          }
        }
      } else if (!item.meta.empty()) {
        const int avail_meta_w = card_w - theme_.px(20);
        const std::vector<std::string> meta_lines = renderer_.wrap_text(item.meta, theme_.font_small, avail_meta_w, false, FontFace::Condensed);
        if (!meta_lines.empty()) {
          renderer_.draw_text(meta_lines[0], card_x + theme_.px(10), badge_y + theme_.px(3), theme_.text_muted, theme_.font_small, false, FontFace::Condensed);
        }
      }
    }
  }
  renderer_.pop_clip();

  // Barra de desplazamiento
  if (rows > visible_rows) {
    const int track_x = renderer_.width() - theme_.px(12);
    const int track_h = grid_height;
    renderer_.fill_round_rect(track_x, grid_top, theme_.scrollbar_width, track_h, theme_.scrollbar_width / 2,
                              theme_.panel_alt);
    const int thumb_h = std::max(theme_.px(40), track_h * visible_rows / rows);
    const int thumb_y = grid_top + (track_h - thumb_h) * scroll_ / std::max(rows - visible_rows, 1);
    renderer_.fill_round_rect(track_x, thumb_y, theme_.scrollbar_width, thumb_h,
                              theme_.scrollbar_width / 2, theme_.accent);
  }
}

void App::draw_library_filters() {
  const char* labels[] = {"Instalados", "Con actualizacion", "Ports MrShip"};
  const int origin_x = theme_.sidebar_width;
  const int y = renderer_.height() - theme_.footer_height - theme_.px(kChipBarHeight) + theme_.px(8);
  renderer_.draw_text("Y: filtrar", origin_x + theme_.content_padding, y + theme_.px(6), theme_.text_muted,
                      theme_.font_small);
  int x = origin_x + theme_.content_padding + theme_.px(90);
  for (int index = 0; index < 3; ++index) {
    const bool active = index == library_filter_;
    const int chip_w = renderer_.text_width(labels[index], theme_.font_small, active) + theme_.px(24);
    renderer_.fill_round_rect(x, y, chip_w, theme_.px(30), theme_.px(8),
                              active ? theme_.accent.with_alpha(45) : theme_.panel_alt);
    renderer_.stroke_round_rect(x, y, chip_w, theme_.px(30), theme_.px(8),
                                active ? theme_.accent : theme_.border, active ? 2 : 1);
    renderer_.draw_text(labels[index], x + theme_.px(12), y + theme_.px(5),
                        active ? theme_.text : theme_.text_dim, theme_.font_small, active);
    x += chip_w + theme_.px(10);
  }
}

// ------------------------------------------------------------------ fuente del catalogo

std::string App::catalog_source_title() const {
  return "Catálogo";
}

std::string App::catalog_source_origin() const {
  switch (catalog_mode_) {
    case CatalogMode::Master: return "Catálogo Maestro de Ports";
    case CatalogMode::Quiver: return "Quiver Online";
    case CatalogMode::Delanx: break;
  }
  return "Catálogo Delanx";
}

std::string App::catalog_source_label() const {
  switch (catalog_mode_) {
    case CatalogMode::Master: return "Catálogo Maestro";
    case CatalogMode::Quiver: return "Quiver Online";
    case CatalogMode::Delanx: break;
  }
  return "Catálogo Verificado";
}

std::string App::catalog_empty_text() const {
  if (catalog_show_favorites_) {
    return "No tienes ningún título marcado como favorito. Pulsa X para marcarlo.";
  }
  switch (catalog_mode_) {
    case CatalogMode::Master:
      return "Sin entradas en el catálogo maestro.";
    case CatalogMode::Quiver:
      return "Sin resultados. Pulsa L+R para buscar en Quiver.";
    case CatalogMode::Delanx: break;
  }
  return "Catálogo vacío.";
}

void App::cycle_catalog_mode() {
  // Tres fuentes en orden local -> maestro -> online, con la lista limpia al cambiar.
  catalog_mode_ = static_cast<CatalogMode>((static_cast<int>(catalog_mode_) + 1) % 3);
  focus_ = 0;
  scroll_ = 0;
  hit_targets_.clear();
  switch (catalog_mode_) {
    case CatalogMode::Master:
      toast(str::format("Catálogo Maestro: %lu títulos",
                        static_cast<unsigned long>(master_catalog_.entries.size())));
      break;
    case CatalogMode::Quiver:
      catalog_online_.clear();
      catalog_cursor_.clear();
      catalog_done_ = false;
      refresh_catalog_page(true);
      break;
    case CatalogMode::Delanx:
      toast("Catálogo Delanx (Switch verificado)");
      break;
  }
}

void App::draw_catalog_filters() {
  const int origin_x = theme_.sidebar_width;
  const int y = renderer_.height() - theme_.footer_height - theme_.px(kChipBarHeight) + theme_.px(8);

  const char* chip_labels[] = {"Todos", "Favoritos ★"};
  int chip_x = origin_x + theme_.content_padding;
  for (int i = 0; i < 2; ++i) {
    const bool active = (i == 1) ? catalog_show_favorites_ : !catalog_show_favorites_;
    const int chip_w = renderer_.text_width(chip_labels[i], theme_.font_small, active) + theme_.px(24);
    renderer_.fill_round_rect(chip_x, y, chip_w, theme_.px(30), theme_.px(8),
                              active ? theme_.accent.with_alpha(45) : theme_.panel_alt);
    renderer_.stroke_round_rect(chip_x, y, chip_w, theme_.px(30), theme_.px(8),
                                active ? theme_.accent : theme_.border, active ? 2 : 1);
    renderer_.draw_text(chip_labels[i], chip_x + theme_.px(12), y + theme_.px(5),
                        active ? theme_.text : theme_.text_dim, theme_.font_small, active);
    chip_x += chip_w + theme_.px(10);
  }

  // Acciones y atajos a la derecha
  const std::string hint = "X: Favorito ★   Y: Filtrar   L+R: Buscar";
  const int right_limit = renderer_.width() - theme_.content_padding;
  renderer_.draw_text_right(hint, right_limit, y + theme_.px(6), theme_.text_muted, theme_.font_small);
}

// ------------------------------------------------------------------ metadatos de catalogo

const CatalogEntry* App::catalog_metadata_for(const AppEntry& app) const {
  // Busca la entrada de catalogo de la que salio la app para poder mostrar sus metadatos
  // informativos. Primero por repositorio (identidad fuerte) y despues por folderName, que
  // solo vale si no contradice a un repositorio conocido.
  const Catalog* catalogs[] = {&master_catalog_, &local_catalog_, &mrship_catalog_};
  const auto has_info = [](const CatalogEntry& entry) {
    return !entry.category.empty() || !entry.asset_type.empty() || !entry.rom_source_hint.empty();
  };
  if (!app.repository.empty()) {
    for (const Catalog* catalog : catalogs) {
      for (const CatalogEntry& entry : catalog->entries) {
        if (has_info(entry) && !entry.repository.empty() && str::iequals(entry.repository, app.repository)) {
          return &entry;
        }
      }
    }
  }
  if (!app.folder_name.empty()) {
    for (const Catalog* catalog : catalogs) {
      for (const CatalogEntry& entry : catalog->entries) {
        if (!has_info(entry) || entry.folder_name.empty()) continue;
        if (!str::iequals(entry.folder_name, app.folder_name)) continue;
        if (!app.repository.empty() && !entry.repository.empty() &&
            !str::iequals(entry.repository, app.repository)) {
          continue;  // misma carpeta pero otro repositorio: no es la misma app
        }
        return &entry;
      }
    }
  }
  return nullptr;
}

// ------------------------------------------------------------------ acciones de navegacion

void App::on_accept() {
  if (details_open_) {
    if (details_is_homebrew_) {
      launch_nro(active_homebrew_.nro_path);
      return;
    }
    if (details_index_ >= 0 && details_index_ < static_cast<int>(library_.apps.size())) {
      AppEntry& app = library_.apps[static_cast<size_t>(details_index_)];
      if (app.installed) {
        launch_app(details_index_);
      } else {
        install_app(details_index_, false);
      }
    }
    return;
  }
  switch (tab_) {
    case Tab::Library: {
      const std::vector<ListItem> items = build_library_items();
      if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
        details_index_ = items[static_cast<size_t>(focus_)].library_index;
        details_open_ = true;
      }
      break;
    }
    case Tab::Catalog: {
      const std::vector<ListItem> items = build_catalog_items();
      if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
        activate_catalog_item(items[static_cast<size_t>(focus_)]);
      }
      break;
    }
    case Tab::MrShip: {
      const std::vector<ListItem> items = build_mrship_items();
      if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
        const ListItem& item = items[static_cast<size_t>(focus_)];
        if (item.unavailable) {
          open_dialog(item.title, item.unavailable_reason.empty() ? "Este proyecto no publica build de Switch."
                                                                 : item.unavailable_reason,
                      {DialogOption{"Entendido", nullptr}});
        } else {
          activate_catalog_item(item);
        }
      }
      break;
    }
    case Tab::Homebrew: {
      if (focus_ >= 0 && focus_ < static_cast<int>(homebrew_cache_.size())) {
        open_details_for_homebrew(homebrew_cache_[static_cast<size_t>(focus_)]);
      }
      break;
    }
    default:
      break;
  }
}

void App::on_secondary() {
  if (details_open_) {
    if (details_is_homebrew_) {
      check_homebrew_update(active_homebrew_);
      return;
    }
    if (details_index_ >= 0 && details_index_ < static_cast<int>(library_.apps.size())) {
      install_app(details_index_, false);
    }
    return;
  }
  switch (tab_) {
    case Tab::Library: {
      const std::vector<ListItem> items = build_library_items();
      if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
        install_app(items[static_cast<size_t>(focus_)].library_index, false);
      }
      break;
    }
    case Tab::Catalog: {
      const std::vector<ListItem> items = build_catalog_items();
      if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
        const ListItem& item = items[static_cast<size_t>(focus_)];
        toggle_favorite(item.title);
        toast(is_favorite(item.title) ? ("★ Marcado como favorito: " + item.title)
                                      : ("Eliminado de favoritos: " + item.title));
      }
      break;
    }
    case Tab::MrShip: {
      const std::vector<ListItem> items = build_mrship_items();
      if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
        activate_catalog_item(items[static_cast<size_t>(focus_)]);
      }
      break;
    }
    case Tab::Homebrew: {
      if (focus_ >= 0 && focus_ < static_cast<int>(homebrew_cache_.size())) {
        check_homebrew_update(homebrew_cache_[static_cast<size_t>(focus_)]);
      }
      break;
    }
    default:
      break;
  }
}

void App::on_tertiary() {
  switch (tab_) {
    case Tab::Library:
      library_filter_ = (library_filter_ + 1) % 3;
      focus_ = 0;
      scroll_ = 0;
      break;
    case Tab::Catalog:
      catalog_show_favorites_ = !catalog_show_favorites_;
      focus_ = 0;
      scroll_ = 0;
      toast(catalog_show_favorites_ ? "Mostrando solo Favoritos ★" : "Mostrando todo el Catálogo");
      break;
    case Tab::MrShip: {
      const std::vector<ListItem> items = build_mrship_items();
      if (focus_ < static_cast<int>(items.size()) && items[static_cast<size_t>(focus_)].unavailable) {
        const ListItem& item = items[static_cast<size_t>(focus_)];
        open_dialog(item.title, item.unavailable_reason, {DialogOption{"Entendido", nullptr}});
      } else {
        open_dialog("Ports N64 (Harbour Masters)",
                    "Los ports creados por Harbour Masters no incluyen los datos con copyright del juego: necesitas tu propia ROM "
                    "y generar los ficheros .otr/.o2r en un PC antes de copiarlos "
                    "a la tarjeta SD. Delanx instala el .nro y te recuerda los ficheros que faltan.",
                    {DialogOption{"Entendido", nullptr}});
      }
      break;
    }
    default:
      break;
  }
}

void App::open_search() {
  if (tab_ != Tab::Catalog) {
    toast("La busqueda solo esta disponible en el catalogo");
    return;
  }
  if (catalog_mode_ != CatalogMode::Quiver) {
    // Solo Quiver es online: los catalogos locales se recorren a mano.
    toast("La busqueda solo funciona en la fuente Quiver (online). Pulsa Y para cambiarla.", true);
    return;
  }
  std::string text;
  if (!platform::text_input("Buscar en el catalogo", catalog_search_, &text, 64)) return;
  catalog_search_ = text;
  refresh_catalog_page(true);
}

int App::add_catalog_entry_to_library(const ListItem& item) {
  AppEntry entry;
  if (item.source) {
    entry = item.source->to_app_entry();
    if (!item.source->verified) {
      // El aviso viaja con la app a la biblioteca: alli no hay chapa de "sin verificar".
      const std::string note =
          "Entrada del catalogo maestro: sin verificar que el proyecto publique un .nro para Switch. "
          "Comprueba los assets del repositorio antes de instalarla.";
      entry.requirements = entry.requirements.empty() ? note : (entry.requirements + " " + note);
    }
  } else if (catalog_mode_ == CatalogMode::Quiver && item.catalog_index >= 0 &&
             item.catalog_index < static_cast<int>(catalog_online_.size())) {
    const quiver::CatalogApp& source = catalog_online_[static_cast<size_t>(item.catalog_index)];
    if (source.repository.empty()) {
      open_dialog(source.name, "Esta entrada del catalogo no publica repositorio, asi que no se puede "
                               "descargar nada automaticamente. Puedes crearla a mano como app manual.",
                  {DialogOption{"Entendido", nullptr}});
      return -1;
    }
    entry.name = source.name;
    entry.project = source.project_name;
    entry.repository = source.repository;
    entry.repository_source = source.repository_source.empty() ? "github" : source.repository_source;
    entry.folder_name = source.folder_name.empty() ? source.slug : source.folder_name;
    entry.release_asset_filter = source.release_asset_filter;
    entry.app_icon_url = source.icon_url;
    entry.tags = source.tags;
    entry.catalog_entry_id = source.id;
    entry.requirements =
        "Entrada del catalogo de Quiver (escritorio): normalmente NO publica builds de Switch. "
        "Comprueba los assets del repositorio antes de instalarla.";
  } else {
    return -1;
  }

  if (entry.folder_name.empty()) entry.folder_name = entry.name;
  const std::string key = entry.identity_key();
  for (size_t i = 0; i < library_.apps.size(); ++i) {
    if (library_.apps[i].identity_key() == key) return static_cast<int>(i);
  }
  entry.refresh_install_state();
  library_.apps.push_back(entry);
  queue_artwork(entry);
  save_library();
  return static_cast<int>(library_.apps.size()) - 1;
}

void App::activate_catalog_item(const ListItem& item) {
  if (item.installed && item.library_index >= 0 &&
      item.library_index < static_cast<int>(library_.apps.size())) {
    details_index_ = item.library_index;
    details_open_ = true;
    return;
  }
  const int index = add_catalog_entry_to_library(item);
  if (index < 0) return;
  // La biblioteca solo muestra lo instalado: la ficha ofrece instalar esta entrada exacta.
  details_index_ = index;
  details_open_ = true;
}

void App::refresh_catalog_page(bool reset) {
  if (catalog_mode_ != CatalogMode::Quiver) return;  // Quiver es la unica fuente con red
  begin_busy("Consultando el catalogo de Quiver");
  quiver::Query query;
  query.search = catalog_search_;
  query.console = catalog_console_;
  query.project_type = catalog_project_type_;
  query.sort = catalog_sort_;
  query.limit = 48;
  if (!reset) query.cursor = catalog_cursor_;
  std::string error;
  const quiver::Page page = quiver::fetch_apps(settings_.DelanxCatalogBaseUrl, query, &error);
  end_busy();
  if (!error.empty() && page.items.empty()) {
    toast("No se pudo consultar el catalogo: " + error, true);
    return;
  }
  if (reset) catalog_online_.clear();
  catalog_online_.insert(catalog_online_.end(), page.items.begin(), page.items.end());
  catalog_cursor_ = page.next_cursor;
  catalog_done_ = page.is_done;
  toast(str::format("Catalogo Quiver: %lu entradas", static_cast<unsigned long>(catalog_online_.size())));
}

// ------------------------------------------------------------------ detalles

void App::draw_details_overlay() {
  if (details_is_homebrew_) {
    renderer_.fill_rect(0, 0, renderer_.width(), renderer_.height(), theme_.overlay);
    const int width = std::min(theme_.px(980), renderer_.width() - theme_.px(120));
    const int height = std::min(theme_.px(620), renderer_.height() - theme_.px(90));
    const int x = (renderer_.width() - width) / 2;
    const int y = (renderer_.height() - height) / 2;

    renderer_.fill_round_rect(x, y, width, height, theme_.px(18), theme_.panel);
    renderer_.stroke_round_rect(x, y, width, height, theme_.px(18), theme_.border, 1);

    // Icono (formato cuadrado tipico de NRO 220x220)
    const int art_w = theme_.px(220);
    const int art_h = theme_.px(220);
    renderer_.fill_round_rect(x + theme_.px(28), y + theme_.px(28), art_w, art_h, theme_.px(14),
                              theme_.panel_alt);
    renderer_.stroke_round_rect(x + theme_.px(28), y + theme_.px(28), art_w, art_h, theme_.px(14),
                                theme_.border, 1);
    Texture* artwork = !active_homebrew_.icon_path.empty() ? renderer_.texture(active_homebrew_.icon_path) : nullptr;
    if (artwork) {
      renderer_.draw_texture_fit(artwork, x + theme_.px(28), y + theme_.px(28), art_w, art_h);
    } else {
      renderer_.draw_text_centered(initials(active_homebrew_.name), x + theme_.px(28) + art_w / 2,
                                   y + theme_.px(28) + art_h / 2 - theme_.px(20), theme_.text_muted,
                                   theme_.font_header, true);
    }

    const int info_x = x + theme_.px(28) + art_w + theme_.px(26);
    const int info_w = x + width - info_x - theme_.px(28);
    int info_y = y + theme_.px(30);

    // Limite de recorte para prevenir cualquier desbordamiento de texto hacia el pie
    renderer_.push_clip(info_x, y + theme_.px(20), info_w, height - theme_.px(82));

    const std::vector<std::string> title_lines = renderer_.wrap_text(active_homebrew_.name, theme_.font_header, info_w, true);
    for (const auto& line : title_lines) {
      renderer_.draw_text(line, info_x, info_y, theme_.text, theme_.font_header, true);
      info_y += renderer_.font_height(theme_.font_header, true) + theme_.px(2);
    }
    info_y += theme_.px(4);

    if (!active_homebrew_.author.empty()) {
      renderer_.draw_text(active_homebrew_.author, info_x, info_y, theme_.text_dim, theme_.font_body);
      info_y += renderer_.font_height(theme_.font_body) + theme_.px(8);
    }

    // Chapas identificativas
    int chip_x = info_x;
    auto chip = [&](const std::string& text, const Color& color) {
      const int chip_w = renderer_.text_width(text, theme_.font_small) + theme_.px(18);
      if (chip_x + chip_w > info_x + info_w) {
        chip_x = info_x;
        info_y += theme_.px(30);
      }
      renderer_.fill_round_rect(chip_x, info_y, chip_w, theme_.px(24), theme_.px(6), color.with_alpha(46));
      renderer_.stroke_round_rect(chip_x, info_y, chip_w, theme_.px(24), theme_.px(6), color.with_alpha(90), 1);
      renderer_.draw_text(text, chip_x + theme_.px(9), info_y + theme_.px(3), color, theme_.font_small, true);
      chip_x += chip_w + theme_.px(8);
    };

    chip("Homebrew", theme_.accent_alt);
    if (!active_homebrew_.github_repo.empty()) {
      chip("GitHub Vinculado", theme_.success);
    } else {
      chip("Standalone NRO", theme_.text_muted);
    }
    info_y += theme_.px(36);

    const std::string ver_text = "Versión: " + (active_homebrew_.version.empty() ? "-" : ("v" + active_homebrew_.version));
    renderer_.draw_text(ver_text, info_x, info_y, theme_.text_dim, theme_.font_small);
    info_y += theme_.px(24);

    if (!active_homebrew_.github_repo.empty()) {
      renderer_.draw_text("Repositorio: " + active_homebrew_.github_repo, info_x, info_y, theme_.text_muted, theme_.font_small);
      info_y += theme_.px(24);
    }

    renderer_.draw_text("Ejecutable: " + active_homebrew_.nro_path, info_x, info_y, theme_.text_muted, theme_.font_small);
    info_y += theme_.px(24);

    if (!active_homebrew_.folder_path.empty()) {
      renderer_.draw_text("Ubicación: " + active_homebrew_.folder_path, info_x, info_y, theme_.text_muted, theme_.font_small);
      info_y += theme_.px(24);
    }

    info_y += theme_.px(8);
    if (!active_homebrew_.github_repo.empty()) {
      renderer_.draw_text("Esta aplicación cuenta con soporte autónomo de actualizaciones mediante GitHub Releases.",
                          info_x, info_y, theme_.accent, theme_.font_small);
    } else {
      renderer_.draw_text("Aplicación instalada localmente en la tarjeta SD (sin repositorio registrado).",
                          info_x, info_y, theme_.text_dim, theme_.font_small);
    }

    renderer_.pop_clip();

    // Pie de acciones
    const int footer_y = y + height - theme_.px(54);
    renderer_.fill_round_rect(x + theme_.px(20), footer_y, width - theme_.px(40), theme_.px(40), theme_.px(10),
                              theme_.panel_alt);
    renderer_.stroke_round_rect(x + theme_.px(20), footer_y, width - theme_.px(40), theme_.px(40), theme_.px(10),
                                theme_.border, 1);

    struct ModalHint {
      Btn btn;
      std::string label;
    };
    std::vector<ModalHint> modal_hints;
    modal_hints.push_back({Btn::A, "Lanzar"});
    if (!active_homebrew_.github_repo.empty()) {
      modal_hints.push_back({Btn::X, "Actualizar"});
    }
    modal_hints.push_back({Btn::B, "Cerrar"});

    int total_hints_w = 0;
    for (const auto& h : modal_hints) {
      total_hints_w += theme_.px(28) + renderer_.text_width(h.label, theme_.font_small) + theme_.px(32);
    }
    int hint_x = x + (width - total_hints_w) / 2;
    for (const auto& h : modal_hints) {
      const int radius = theme_.px(11);
      renderer_.fill_circle(hint_x + radius, footer_y + theme_.px(20), radius, theme_.card_focus);
      renderer_.stroke_circle(hint_x + radius, footer_y + theme_.px(20), radius, theme_.border, 1);
      renderer_.draw_text_centered(button_glyph(h.btn), hint_x + radius, footer_y + theme_.px(11), theme_.text,
                                   theme_.font_small, true);
      renderer_.draw_text(h.label, hint_x + radius * 2 + theme_.px(8), footer_y + theme_.px(10), theme_.text,
                          theme_.font_small);
      hint_x += theme_.px(28) + renderer_.text_width(h.label, theme_.font_small) + theme_.px(32);
    }
    return;
  }

  if (details_index_ < 0 || details_index_ >= static_cast<int>(library_.apps.size())) {
    details_open_ = false;
    return;
  }
  const AppEntry& app = library_.apps[static_cast<size_t>(details_index_)];

  renderer_.fill_rect(0, 0, renderer_.width(), renderer_.height(), theme_.overlay);
  const int width = std::min(theme_.px(980), renderer_.width() - theme_.px(120));
  const int height = std::min(theme_.px(620), renderer_.height() - theme_.px(90));
  const int x = (renderer_.width() - width) / 2;
  const int y = (renderer_.height() - height) / 2;

  renderer_.fill_round_rect(x, y, width, height, theme_.px(18), theme_.panel);
  renderer_.stroke_round_rect(x, y, width, height, theme_.px(18), theme_.border, 1);

  // Icono / caratula (proporciones de portada de juego 220x280)
  const int art_w = theme_.px(220);
  const int art_h = theme_.px(280);
  renderer_.fill_round_rect(x + theme_.px(28), y + theme_.px(28), art_w, art_h, theme_.px(14),
                            theme_.panel_alt);
  renderer_.stroke_round_rect(x + theme_.px(28), y + theme_.px(28), art_w, art_h, theme_.px(14),
                              theme_.border, 1);
  Texture* artwork = icon_for(app);
  if (artwork) {
    renderer_.draw_texture_fit(artwork, x + theme_.px(28), y + theme_.px(28), art_w, art_h);
  } else {
    renderer_.draw_text_centered(initials(app.name), x + theme_.px(28) + art_w / 2,
                                 y + theme_.px(28) + art_h / 2 - theme_.px(20), theme_.text_muted,
                                 theme_.font_header, true);
  }

  const int info_x = x + theme_.px(28) + art_w + theme_.px(26);
  const int info_w = x + width - info_x - theme_.px(28);
  int info_y = y + theme_.px(30);

  // Limite de recorte para prevenir cualquier desbordamiento de texto hacia el pie
  renderer_.push_clip(info_x, y + theme_.px(20), info_w, height - theme_.px(82));

  const std::vector<std::string> name_lines = renderer_.wrap_text(app.name, theme_.font_header, info_w, true);
  for (const auto& line : name_lines) {
    renderer_.draw_text(line, info_x, info_y, theme_.text, theme_.font_header, true);
    info_y += renderer_.font_height(theme_.font_header, true) + theme_.px(2);
  }
  info_y += theme_.px(4);

  if (!app.project.empty()) {
    renderer_.draw_text(app.project, info_x, info_y, theme_.text_dim, theme_.font_body);
    info_y += renderer_.font_height(theme_.font_body) + theme_.px(8);
  }
  if (!app.repository.empty()) {
    renderer_.draw_text(app.repository, info_x, info_y, theme_.text_muted, theme_.font_small);
    info_y += theme_.px(26);
  }

  // Metadatos informativos del catalogo (sobre todo del catalogo maestro): solo si existen.
  const CatalogEntry* catalog_meta = catalog_metadata_for(app);
  auto meta_line = [&](const char* label, const std::string& value) {
    if (value.empty()) return;
    renderer_.draw_text(std::string(label) + value, info_x, info_y, theme_.text_muted, theme_.font_small);
    info_y += theme_.px(22);
  };
  if (catalog_meta) {
    meta_line("Categoria: ", catalog_meta->category);
    meta_line("Tipo de asset: ", catalog_meta->asset_type);
    meta_line("ROM: ", catalog_meta->rom_source_hint);
    info_y += theme_.px(4);  // un poco de aire antes de las chapas
  }

  // Chapas de estado y etiquetas (con salto de linea para evitar desbordamiento)
  int chip_x = info_x;
  auto chip = [&](const std::string& text, const Color& color) {
    const int chip_w = renderer_.text_width(text, theme_.font_small) + theme_.px(18);
    if (chip_x + chip_w > info_x + info_w) {
      chip_x = info_x;
      info_y += theme_.px(30);
    }
    renderer_.fill_round_rect(chip_x, info_y, chip_w, theme_.px(24), theme_.px(6), color.with_alpha(46));
    renderer_.stroke_round_rect(chip_x, info_y, chip_w, theme_.px(24), theme_.px(6), color.with_alpha(90), 1);
    renderer_.draw_text(text, chip_x + theme_.px(9), info_y + theme_.px(3), color, theme_.font_small, true);
    chip_x += chip_w + theme_.px(8);
  };
  if (app.mrship) chip("Harbour Masters", theme_.accent_alt);
  if (app.verified_entry) chip("Verificado", theme_.accent);
  if (app.installed) chip("Instalado", theme_.success);
  if (app.update_available) chip("Actualizacion disponible", theme_.warning);
  if (app.manually_managed()) chip("Manual", theme_.text_muted);
  for (const auto& tag : app.tags) chip(tag, theme_.accent);
  info_y += theme_.px(36);

  const std::string versions = str::format("Instalada: %s    Disponible: %s",
                                           app.installed_version.empty() ? "-" : app.installed_version.c_str(),
                                           app.latest_version.empty() ? "-" : app.latest_version.c_str());
  renderer_.draw_text(versions, info_x, info_y, theme_.text_dim, theme_.font_small);
  info_y += theme_.px(26);

  const std::string install_target = app.sw.home_dir.empty() ? app.sw.target_dir : app.sw.home_dir;
  if (!install_target.empty()) {
    renderer_.draw_text("Instala en: " + install_target, info_x, info_y, theme_.text_muted, theme_.font_small);
    info_y += theme_.px(22);
  }
  if (!app.sw.home_dir.empty() && !app.sw.target_dir.empty() &&
      !str::iequals(app.sw.home_dir, app.sw.target_dir)) {
    renderer_.draw_text("Extrae en: " + app.sw.target_dir, info_x, info_y, theme_.text_muted, theme_.font_small);
    info_y += theme_.px(22);
  }
  if (!app.sw.nro.empty()) {
    renderer_.draw_text("Ejecutable: " + app.sw.nro, info_x, info_y, theme_.text_muted, theme_.font_small);
    info_y += theme_.px(22);
  }

  if (!app.requirements.empty()) {
    info_y += theme_.px(6);
    info_y += renderer_.draw_text_wrapped("Requisitos: " + app.requirements, info_x, info_y, theme_.warning,
                                         theme_.font_small, info_w, 2);
  }

  // Ficheros que debe aportar el usuario
  info_y += theme_.px(10);
  renderer_.draw_text("Ficheros del usuario", info_x, info_y, theme_.text, theme_.font_body, true);
  info_y += theme_.px(26);
  if (app.sw.required_user_files.empty()) {
    renderer_.draw_text("Esta app no necesita ficheros adicionales.", info_x, info_y, theme_.text_muted,
                        theme_.font_small);
  } else {
    for (const auto& file : app.sw.required_user_files) {
      const bool present = paths::exists(paths::to_sd_path(file.path));
      renderer_.fill_circle(info_x + theme_.px(6), info_y + theme_.px(9), theme_.px(5),
                            present ? theme_.success : theme_.danger);
      renderer_.draw_text((present ? "OK  " : "FALTA  ") + file.path, info_x + theme_.px(20), info_y,
                          present ? theme_.text_dim : theme_.warning, theme_.font_small);
      info_y += theme_.px(22);
    }
  }

  renderer_.pop_clip();

  // Acciones disponibles centradas en el pie del modal
  const int footer_y = y + height - theme_.px(54);
  renderer_.fill_round_rect(x + theme_.px(20), footer_y, width - theme_.px(40), theme_.px(40), theme_.px(10),
                            theme_.panel_alt);
  renderer_.stroke_round_rect(x + theme_.px(20), footer_y, width - theme_.px(40), theme_.px(40), theme_.px(10),
                              theme_.border, 1);

  struct ModalHint {
    Btn btn;
    std::string label;
  };
  std::vector<ModalHint> modal_hints;
  modal_hints.push_back({Btn::A, app.installed ? "Lanzar" : "Instalar"});
  modal_hints.push_back({Btn::X, app.installed ? "Actualizar" : "Reinstalar"});
  modal_hints.push_back({Btn::B, "Cerrar"});

  int total_hints_w = 0;
  for (const auto& h : modal_hints) {
    total_hints_w += theme_.px(28) + renderer_.text_width(h.label, theme_.font_small) + theme_.px(32);
  }
  int hint_x = x + (width - total_hints_w) / 2;
  for (const auto& h : modal_hints) {
    const int radius = theme_.px(11);
    renderer_.fill_circle(hint_x + radius, footer_y + theme_.px(20), radius, theme_.card_focus);
    renderer_.stroke_circle(hint_x + radius, footer_y + theme_.px(20), radius, theme_.border, 1);
    renderer_.draw_text_centered(button_glyph(h.btn), hint_x + radius, footer_y + theme_.px(11), theme_.text,
                                 theme_.font_small, true);
    renderer_.draw_text(h.label, hint_x + radius * 2 + theme_.px(8), footer_y + theme_.px(10), theme_.text,
                        theme_.font_small);
    hint_x += theme_.px(28) + renderer_.text_width(h.label, theme_.font_small) + theme_.px(32);
  }
}

}  // namespace delanx::ui
