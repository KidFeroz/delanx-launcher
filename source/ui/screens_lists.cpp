// Delanx — pantallas de navegacion: biblioteca, catalogo (local y Quiver) y Harbour Masters.
#include <algorithm>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "platform/switch_platform.hpp"
#include "platform/text_input.hpp"
#include "services/artwork.hpp"
#include "services/http.hpp"
#include "ui/app.hpp"

namespace delanx::ui {
namespace {

constexpr int kChipBarHeight = 46;

Color badge_color(const Theme& theme, const ListItem& item) {
  if (item.unavailable) return theme.warning;
  if (item.unverified) return theme.warning;  // catalogo maestro: sin verificar
  if (item.harbour) return theme.accent_alt;
  if (item.update) return theme.warning;
  if (item.installed) return theme.success;
  if (item.verified) return theme.accent;
  return theme.text_muted;
}

std::string initials(const std::string& text) {
  std::string out;
  for (char c : text) {
    if (std::isalpha(static_cast<unsigned char>(c))) {
      out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
      if (out.size() >= 2) break;
    }
  }
  if (out.empty()) out = "?";
  return out;
}

}  // namespace

// ------------------------------------------------------------------ construccion de listas

std::vector<ListItem> App::build_library_items() {
  std::vector<ListItem> items;
  for (size_t index = 0; index < library_.apps.size(); ++index) {
    const AppEntry& app = library_.apps[index];
    if (library_filter_ == 1 && !app.installed) continue;
    if (library_filter_ == 2 && !app.update_available) continue;
    if (library_filter_ == 3 && !app.harbour_masters) continue;

    ListItem item;
    item.title = display_name(app);
    item.subtitle = app.project.empty() ? app.repository : app.project;
    item.icon_url = app.app_icon_url;
    item.installed = app.installed;
    item.update = app.update_available;
    item.harbour = app.harbour_masters;
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

std::vector<ListItem> App::build_catalog_items() {
  std::vector<ListItem> items;
  if (catalog_mode_ == CatalogMode::Quiver) {
    for (size_t index = 0; index < catalog_online_.size(); ++index) {
      const quiver::CatalogApp& entry = catalog_online_[index];
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

  if (catalog_mode_ == CatalogMode::Master) {
    // Catalogo maestro del usuario: TODAS las entradas son propuestas sin comprobar que el
    // proyecto publique un .nro para Switch, asi que la chapa lo dice sin rodeos.
    const Catalog& catalog = master_catalog_;
    for (size_t index = 0; index < catalog.entries.size(); ++index) {
      const CatalogEntry& entry = catalog.entries[index];
      ListItem item;
      item.title = entry.name;
      item.subtitle = entry.project.empty() ? entry.repository : entry.project;
      item.catalog_index = static_cast<int>(index);
      item.icon_url = entry.app_icon_url;
      item.harbour = entry.harbour_masters;
      item.verified = entry.verified;
      item.unverified = !entry.verified;
      item.badge = entry.verified ? entry.badge() : "Sin verificar";
      item.meta = entry.sw.nro;  // .nro propuesto en el maestro (aun sin confirmar)
      items.push_back(item);
    }
    return items;
  }

  for (size_t index = 0; index < local_catalog_.entries.size(); ++index) {
    const CatalogEntry& entry = local_catalog_.entries[index];
    ListItem item;
    item.title = entry.name;
    item.subtitle = entry.project.empty() ? entry.repository : entry.project;
    item.catalog_index = static_cast<int>(index);
    item.icon_url = entry.app_icon_url;
    item.harbour = entry.harbour_masters;
    item.verified = entry.verified;
    item.badge = entry.badge();
    const bool has_nro = !entry.sw.nro.empty() || !entry.sw.nro_path.empty();
    item.meta = has_nro ? entry.sw.nro : "ver .nro no confirmado";
    items.push_back(item);
  }
  return items;
}

std::vector<ListItem> App::build_harbour_items() {
  std::vector<ListItem> items;
  for (size_t index = 0; index < harbour_catalog_.entries.size(); ++index) {
    const CatalogEntry& entry = harbour_catalog_.entries[index];
    ListItem item;
    item.title = entry.name;
    item.subtitle = entry.project.empty() ? entry.repository : entry.project;
    item.catalog_index = static_cast<int>(index);
    item.icon_url = entry.app_icon_url;
    item.harbour = true;
    item.verified = entry.verified;
    item.badge = entry.verified ? "Con build Switch" : "Sin verificar";
    item.meta = entry.sw.nro.empty() ? entry.requirements : entry.sw.nro;
    items.push_back(item);
  }
  for (size_t index = 0; index < harbour_catalog_.unavailable.size(); ++index) {
    const UnavailableEntry& entry = harbour_catalog_.unavailable[index];
    ListItem item;
    item.title = entry.name;
    item.subtitle = entry.repository;
    item.harbour_index = static_cast<int>(index);
    item.unavailable = true;
    item.badge = "Sin build Switch";
    item.unavailable_reason = entry.reason;
    item.meta = entry.workaround.empty() ? "no disponible" : "nightly disponible";
    items.push_back(item);
  }
  return items;
}

// ------------------------------------------------------------------ dibujo de listas

void App::draw_list_screen(const std::string& title, const std::string& subtitle,
                           const std::vector<ListItem>& items, const std::string& empty_text) {
  const int origin_x = theme_.sidebar_width;
  const int origin_y = theme_.topbar_height;
  const int content_w = renderer_.width() - origin_x;
  const int content_h = renderer_.height() - origin_y - theme_.footer_height;
  const bool has_chips = (tab_ == Tab::Library || tab_ == Tab::Catalog);
  const int bottom_bar = has_chips ? theme_.px(kChipBarHeight) : 0;

  renderer_.draw_text(subtitle, origin_x + theme_.content_padding, origin_y + theme_.px(8), theme_.text_muted,
                      theme_.font_small);
  renderer_.draw_text_right(str::format("%lu elementos", static_cast<unsigned long>(items.size())),
                            renderer_.width() - theme_.content_padding, origin_y + theme_.px(8),
                            theme_.text_muted, theme_.font_small);

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
  const int card_h = card_w * 9 / 16 + theme_.px(86);
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
        renderer_.stroke_round_rect(card_x, card_y, card_w, card_h, theme_.card_radius, theme_.focus_ring, 2);
      }

      // Zona de caratula
      const int art_h = card_w * 9 / 16;
      renderer_.fill_round_rect(card_x + theme_.px(8), card_y + theme_.px(8), card_w - theme_.px(16),
                                art_h - theme_.px(8), theme_.px(8), theme_.panel_alt);
      Texture* artwork = nullptr;
      if (item.library_index >= 0) {
        artwork = icon_for(library_.apps[static_cast<size_t>(item.library_index)]);
      } else if (item.catalog_index >= 0) {
        const CatalogEntry* entry = nullptr;
        if (catalog_mode_ == CatalogMode::Master && static_cast<size_t>(item.catalog_index) < master_catalog_.entries.size()) {
          entry = &master_catalog_.entries[static_cast<size_t>(item.catalog_index)];
        } else if (catalog_mode_ == CatalogMode::Delanx && static_cast<size_t>(item.catalog_index) < local_catalog_.entries.size()) {
          entry = &local_catalog_.entries[static_cast<size_t>(item.catalog_index)];
        }
        if (entry) {
          artwork = icon_for(entry->to_app_entry());
        }
      } else if (item.harbour_index >= 0 && static_cast<size_t>(item.harbour_index) < harbour_catalog_.entries.size()) {
        artwork = icon_for(harbour_catalog_.entries[static_cast<size_t>(item.harbour_index)].to_app_entry());
      }
      if (artwork) {
        renderer_.draw_texture(artwork, card_x + theme_.px(8), card_y + theme_.px(8), card_w - theme_.px(16),
                               art_h - theme_.px(8));
      } else {
        renderer_.draw_text_centered(initials(item.title), card_x + card_w / 2,
                                     card_y + theme_.px(8) + (art_h - theme_.px(8)) / 2 - theme_.px(18),
                                     theme_.text_muted, theme_.font_header, true);
      }

      // Texto
      const int text_x = card_x + theme_.px(12);
      const int text_w = card_w - theme_.px(24);
      int text_y = card_y + art_h + theme_.px(4);
      const std::vector<std::string> title_lines = renderer_.wrap_text(item.title, theme_.font_body, text_w, true);
      for (size_t line = 0; line < title_lines.size() && line < 2; ++line) {
        renderer_.draw_text(title_lines[line], text_x, text_y, theme_.text, theme_.font_body, true);
        text_y += renderer_.font_height(theme_.font_body, true) + 1;
      }
      if (!item.subtitle.empty()) {
        const std::vector<std::string> subtitle_lines =
            renderer_.wrap_text(item.subtitle, theme_.font_small, text_w);
        if (!subtitle_lines.empty()) {
          renderer_.draw_text(subtitle_lines[0], text_x, text_y + 1, theme_.text_dim, theme_.font_small);
        }
      }

      // Chapa de estado
      if (!item.badge.empty()) {
        const int badge_w = renderer_.text_width(item.badge, theme_.font_small) + theme_.px(16);
        const Color color = badge_color(theme_, item);
        renderer_.fill_round_rect(card_x + theme_.px(10), card_y + card_h - theme_.px(30), badge_w,
                                  theme_.px(24), theme_.px(8), color.with_alpha(46));
        renderer_.draw_text(item.badge, card_x + theme_.px(18), card_y + card_h - theme_.px(26), color,
                            theme_.font_small, true);
      }
      if (!item.meta.empty()) {
        const std::vector<std::string> meta_lines =
            renderer_.wrap_text(item.meta, theme_.font_small, text_w);
        std::string meta = meta_lines.empty() ? item.meta : meta_lines[0];
        renderer_.draw_text_right(meta, card_x + card_w - theme_.px(12), card_y + card_h - theme_.px(26),
                                  theme_.text_muted, theme_.font_small);
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
  const char* labels[] = {"Todas", "Instaladas", "Con actualizacion", "Harbour Masters"};
  const int origin_x = theme_.sidebar_width;
  const int y = renderer_.height() - theme_.footer_height - theme_.px(kChipBarHeight) + theme_.px(8);
  renderer_.draw_text("Y: filtrar", origin_x + theme_.content_padding, y + theme_.px(6), theme_.text_muted,
                      theme_.font_small);
  int x = origin_x + theme_.content_padding + theme_.px(90);
  for (int index = 0; index < 4; ++index) {
    const bool active = index == library_filter_;
    const int chip_w = renderer_.text_width(labels[index], theme_.font_small) + theme_.px(24);
    renderer_.fill_round_rect(x, y, chip_w, theme_.px(30), theme_.px(8),
                              active ? theme_.accent.with_alpha(60) : theme_.panel_alt);
    renderer_.draw_text(labels[index], x + theme_.px(12), y + theme_.px(5),
                        active ? theme_.accent : theme_.text_dim, theme_.font_small, active);
    x += chip_w + theme_.px(10);
  }
}

// ------------------------------------------------------------------ fuente del catalogo

std::string App::catalog_source_title() const {
  switch (catalog_mode_) {
    case CatalogMode::Master: return "Catalogo maestro (sin verificar)";
    case CatalogMode::Quiver: return "Catalogo Quiver (online)";
    case CatalogMode::Delanx: break;
  }
  return "Catalogo Delanx (Switch verificado)";
}

std::string App::catalog_source_origin() const {
  switch (catalog_mode_) {
    case CatalogMode::Master: return "romfs:/catalog/ports-master.json";
    case CatalogMode::Quiver: return "api.quiverlauncher.com";
    case CatalogMode::Delanx: break;
  }
  return "romfs:/catalog/switch-catalog.json";
}

std::string App::catalog_source_label() const {
  switch (catalog_mode_) {
    case CatalogMode::Master: return "Fuente: catalogo maestro (local, sin verificar)";
    case CatalogMode::Quiver: return "Fuente: Quiver (online)";
    case CatalogMode::Delanx: break;
  }
  return "Fuente: Delanx (local, verificada)";
}

std::string App::catalog_empty_text() const {
  switch (catalog_mode_) {
    case CatalogMode::Master:
      // El catalogo maestro lo genera otro proceso: puede no existir todavia.
      return "Sin entradas en el catalogo maestro (" + catalog_source_origin() + ").";
    case CatalogMode::Quiver:
      return "Sin resultados. Pulsa X para buscar en Quiver.";
    case CatalogMode::Delanx: break;
  }
  return "Catalogo vacio.";
}

void App::cycle_catalog_mode() {
  // Tres fuentes en orden local -> maestro -> online, con la lista limpia al cambiar.
  catalog_mode_ = static_cast<CatalogMode>((static_cast<int>(catalog_mode_) + 1) % 3);
  focus_ = 0;
  scroll_ = 0;
  // Las zonas pulsables son del fotograma anterior: un toque simultaneo al cambio de fuente
  // no debe golpear las tarjetas que acabamos de dejar atras.
  hit_targets_.clear();
  switch (catalog_mode_) {
    case CatalogMode::Master:
      toast(str::format("Catalogo maestro: %lu títulos sin verificar",
                        static_cast<unsigned long>(master_catalog_.entries.size())));
      break;
    case CatalogMode::Quiver:
      catalog_online_.clear();
      catalog_cursor_.clear();
      catalog_done_ = false;
      // refresh_catalog_page ya avisa (exito o error) con su propio toast.
      refresh_catalog_page(true);  // unica fuente con red: se consulta al entrar
      break;
    case CatalogMode::Delanx:
      toast("Catalogo Delanx (Switch verificado)");
      break;
  }
}

void App::draw_catalog_filters() {
  const int origin_x = theme_.sidebar_width;
  const int y = renderer_.height() - theme_.footer_height - theme_.px(kChipBarHeight) + theme_.px(8);
  const bool online = catalog_mode_ == CatalogMode::Quiver;
  renderer_.draw_text(catalog_source_label(), origin_x + theme_.content_padding, y + theme_.px(6),
                      online ? theme_.warning : theme_.accent, theme_.font_small, true);
  int x = origin_x + theme_.content_padding + theme_.px(300);
  if (catalog_mode_ == CatalogMode::Master) {
    // Advertencia explicita: el maestro son propuestas del usuario, ninguna comprobada.
    const std::string warning =
        str::format("%lu títulos del catálogo maestro: sin verificar que publiquen .nro para Switch",
                    static_cast<unsigned long>(master_catalog_.entries.size()));
    renderer_.draw_text(warning, x, y + theme_.px(6), theme_.warning, theme_.font_small, true);
    x += renderer_.text_width(warning, theme_.font_small) + theme_.px(24);
    renderer_.draw_text("Y: cambiar de fuente", x, y + theme_.px(6), theme_.text_muted, theme_.font_small);
  } else if (online) {
    const std::string info =
        catalog_search_.empty() ? "Sin busqueda (-)" : str::format("Busqueda: %s", catalog_search_.c_str());
    renderer_.draw_text(info, x, y + theme_.px(6), theme_.text_dim, theme_.font_small);
    x += theme_.px(260);
    renderer_.draw_text("X: buscar    Y: cambiar de fuente", x, y + theme_.px(6), theme_.text_muted,
                        theme_.font_small);
  } else {
    renderer_.draw_text("Releases verificadas con .nro para Switch", x, y + theme_.px(6), theme_.text_dim,
                        theme_.font_small);
    x += theme_.px(340);
    renderer_.draw_text("Y: cambiar de fuente", x, y + theme_.px(6), theme_.text_muted, theme_.font_small);
  }
}

// ------------------------------------------------------------------ metadatos de catalogo

const CatalogEntry* App::catalog_metadata_for(const AppEntry& app) const {
  // Busca la entrada de catalogo de la que salio la app para poder mostrar sus metadatos
  // informativos. Primero por repositorio (identidad fuerte) y despues por folderName, que
  // solo vale si no contradice a un repositorio conocido.
  const Catalog* catalogs[] = {&master_catalog_, &local_catalog_, &harbour_catalog_};
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
    case Tab::Library:
      if (!library_.apps.empty()) {
        details_index_ = std::clamp(focus_, 0, static_cast<int>(library_.apps.size()) - 1);
        details_open_ = true;
      }
      break;
    case Tab::Catalog:
      add_catalog_entry_to_library(focus_);
      break;
    case Tab::HarbourMasters:
      add_catalog_entry_to_library(focus_);
      break;
    default:
      break;
  }
}

void App::on_secondary() {
  switch (tab_) {
    case Tab::Library: {
      if (!library_.apps.empty()) {
        install_app(std::clamp(focus_, 0, static_cast<int>(library_.apps.size()) - 1), false);
      }
      break;
    }
    case Tab::Catalog:
    case Tab::HarbourMasters:
      add_catalog_entry_to_library(focus_);
      break;
    default:
      break;
  }
}

void App::on_tertiary() {
  switch (tab_) {
    case Tab::Library:
      library_filter_ = (library_filter_ + 1) % 4;
      focus_ = 0;
      scroll_ = 0;
      break;
    case Tab::Catalog:
      cycle_catalog_mode();  // Delanx -> Master -> Quiver
      break;
    case Tab::HarbourMasters: {
      const std::vector<ListItem> items = build_harbour_items();
      if (focus_ < static_cast<int>(items.size()) && items[static_cast<size_t>(focus_)].unavailable) {
        const ListItem& item = items[static_cast<size_t>(focus_)];
        open_dialog(item.title, item.unavailable_reason, {DialogOption{"Entendido", nullptr}});
      } else {
        open_dialog("Harbour Masters",
                    "Los ports de Harbour Masters no incluyen los datos del juego: necesitas tu propia ROM "
                    "y, en la mayoria de casos, generar los ficheros .otr/.o2r en un PC antes de copiarlos "
                    "a la SD. Delanx instala el .nro y te recuerda que ficheros faltan.",
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

void App::add_catalog_entry_to_library(int index) {
  AppEntry entry;
  if (tab_ == Tab::HarbourMasters) {
    const std::vector<ListItem> items = build_harbour_items();
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    const ListItem& item = items[static_cast<size_t>(index)];
    if (item.unavailable || item.catalog_index < 0) {
      open_dialog(item.title, item.unavailable_reason.empty() ? "Este proyecto no publica build de Switch."
                                                             : item.unavailable_reason,
                  {DialogOption{"Entendido", nullptr}});
      return;
    }
    entry = harbour_catalog_.entries[static_cast<size_t>(item.catalog_index)].to_app_entry();
  } else if (catalog_mode_ == CatalogMode::Quiver) {
    if (index < 0 || index >= static_cast<int>(catalog_online_.size())) return;
    const quiver::CatalogApp& source = catalog_online_[static_cast<size_t>(index)];
    if (source.repository.empty()) {
      open_dialog(source.name, "Esta entrada del catalogo no publica repositorio, asi que no se puede "
                               "descargar nada automaticamente. Puedes crearla a mano como app manual.",
                  {DialogOption{"Entendido", nullptr}});
      return;
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
    // Delanx (verificado) o maestro: ambos son catalogos locales con el mismo formato.
    const Catalog& catalog = (catalog_mode_ == CatalogMode::Master) ? master_catalog_ : local_catalog_;
    if (index < 0 || index >= static_cast<int>(catalog.entries.size())) return;
    const CatalogEntry& source = catalog.entries[static_cast<size_t>(index)];
    entry = source.to_app_entry();
    if (catalog_mode_ == CatalogMode::Master) {
      // El aviso viaja con la app a la biblioteca: alli no hay chapa de "sin verificar".
      const std::string note =
          "Entrada del catalogo maestro: sin verificar que el proyecto publique un .nro para Switch. "
          "Comprueba los assets del repositorio antes de instalarla.";
      entry.requirements = entry.requirements.empty() ? note : (entry.requirements + " " + note);
    }
  }

  if (entry.folder_name.empty()) entry.folder_name = entry.name;
  const std::string key = entry.identity_key();
  for (const auto& existing : library_.apps) {
    if (existing.identity_key() == key) {
      toast("Ya esta en la biblioteca: " + display_name(existing));
      return;
    }
  }
  entry.refresh_install_state();
  library_.apps.push_back(entry);
  queue_artwork(entry);
  save_library();
  toast("Anadida a la biblioteca: " + display_name(entry));
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

  // Icono / caratula
  const int art_size = theme_.px(220);
  renderer_.fill_round_rect(x + theme_.px(28), y + theme_.px(28), art_size, art_size, theme_.px(14),
                            theme_.panel_alt);
  Texture* artwork = icon_for(app);
  if (artwork) {
    renderer_.draw_nro_icon(artwork, x + theme_.px(28), y + theme_.px(28), art_size);
  } else {
    renderer_.draw_text_centered(initials(app.name), x + theme_.px(28) + art_size / 2,
                                 y + theme_.px(28) + art_size / 2 - theme_.px(20), theme_.text_muted,
                                 theme_.font_header, true);
  }

  const int info_x = x + theme_.px(28) + art_size + theme_.px(26);
  const int info_w = x + width - info_x - theme_.px(28);
  int info_y = y + theme_.px(30);

  renderer_.draw_text(app.name, info_x, info_y, theme_.text, theme_.font_header, true);
  info_y += renderer_.font_height(theme_.font_header, true) + theme_.px(4);
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

  // Chapas
  int chip_x = info_x;
  auto chip = [&](const std::string& text, const Color& color) {
    const int chip_w = renderer_.text_width(text, theme_.font_small) + theme_.px(18);
    renderer_.fill_round_rect(chip_x, info_y, chip_w, theme_.px(26), theme_.px(8), color.with_alpha(46));
    renderer_.draw_text(text, chip_x + theme_.px(9), info_y + theme_.px(3), color, theme_.font_small, true);
    chip_x += chip_w + theme_.px(8);
  };
  if (app.harbour_masters) chip("Harbour Masters", theme_.accent_alt);
  if (app.verified_entry) chip("Verificado", theme_.accent);
  if (app.installed) chip("Instalado", theme_.success);
  if (app.update_available) chip("Actualizacion disponible", theme_.warning);
  if (app.manually_managed()) chip("Manual", theme_.text_muted);
  for (const auto& tag : app.tags) chip(tag, theme_.accent);
  info_y += theme_.px(38);

  const std::string versions = str::format("Instalada: %s    Disponible: %s",
                                           app.installed_version.empty() ? "-" : app.installed_version.c_str(),
                                           app.latest_version.empty() ? "-" : app.latest_version.c_str());
  renderer_.draw_text(versions, info_x, info_y, theme_.text_dim, theme_.font_small);
  info_y += theme_.px(26);
  // homeDir = carpeta final de la app (donde vive el .nro); targetDir = raiz de extraccion.
  // Si no hay homeDir, la app vive en la raiz de extraccion.
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
                                         theme_.font_small, info_w, 3);
  }

  // Ficheros que debe aportar el usuario
  info_y += theme_.px(10);
  renderer_.draw_text("Ficheros del usuario", info_x, info_y, theme_.text, theme_.font_body, true);
  info_y += theme_.px(28);
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

  // Acciones disponibles
  const int footer_y = y + height - theme_.px(58);
  renderer_.fill_round_rect(x + theme_.px(20), footer_y, width - theme_.px(40), theme_.px(40), theme_.px(10),
                            theme_.panel_alt);
  int hint_x = x + theme_.px(36);
  auto hint = [&](Btn button, const std::string& label) {
    const int radius = theme_.px(12);
    renderer_.fill_circle(hint_x + radius, footer_y + theme_.px(20), radius, theme_.card);
    renderer_.draw_text_centered(button_glyph(button), hint_x + radius, footer_y + theme_.px(11), theme_.text,
                                 theme_.font_small, true);
    renderer_.draw_text(label, hint_x + radius * 2 + theme_.px(8), footer_y + theme_.px(10), theme_.text_dim,
                        theme_.font_small);
    hint_x += theme_.px(30) + renderer_.text_width(label, theme_.font_small) + theme_.px(28);
  };
  hint(Btn::A, app.installed ? "Lanzar" : "Instalar");
  hint(Btn::X, app.installed ? "Actualizar" : "Reinstalar");
  hint(Btn::B, "Cerrar");
}

}  // namespace delanx::ui
