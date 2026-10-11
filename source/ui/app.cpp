// Delanx — nucleo de la aplicacion: arranque, bucle principal, estructura de ventana y acciones.
#include "ui/app.hpp"

#include <algorithm>
#include <cstdio>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "core/time.hpp"
#include "platform/switch_platform.hpp"
#include "platform/text_input.hpp"
#include "services/artwork.hpp"
#include "services/asset_selector.hpp"
#include "services/github.hpp"
#include "services/gitlab.hpp"
#include "services/http.hpp"
#include "services/installer.hpp"
#include "services/launcher.hpp"
#include "services/release_check.hpp"

namespace delanx::ui {
namespace {

constexpr uint32_t kToastMs = 4200;
constexpr int kExitCodeLaunchChain = 2;

}  // namespace

// ------------------------------------------------------------------ ciclo de vida

bool App::init() {
  platform::init();
  paths::ensure_layout();
  platform::log("Delanx %s arrancando (origen: %s)", DELANX_VERSION, platform::boot_origin().c_str());
  net::global_init();

  applet_mode_ = platform::is_applet_mode();
  theme_ = settings_.is_dark() ? Theme::sphaira_dark() : Theme::sphaira_light();

  if (!renderer_.init("Delanx", 1280, 720, true)) {
    platform::log("no se pudo crear la ventana SDL: %s", SDL_GetError());
    return false;
  }

  // La escala se ajusta al modo de arranque: en applet mode (Album) hay menos memoria y
  // conviene una interfaz algo mas compacta.
  float scale = static_cast<float>(renderer_.width()) / 1280.0f;
  if (scale < 1.0f) scale = 1.0f;
  theme_.apply_scale(scale);
  input_.set_screen_size(renderer_.width(), renderer_.height());
  grid_columns_ = renderer_.width() >= 1600 ? 6 : 4;
  if (settings_.DelanxGridColumns > 0) grid_columns_ = settings_.DelanxGridColumns;

  load_library_and_settings();
  load_local_catalogs();

  // El tema y la rejilla dependen de settings.json, que se acaba de leer: se reaplican ahora
  // para que el primer fotograma ya use el tema guardado.
  theme_ = settings_.is_dark() ? Theme::sphaira_dark() : Theme::sphaira_light();
  theme_.apply_scale(scale);
  grid_columns_ = renderer_.width() >= 1600 ? 6 : 4;
  if (settings_.DelanxGridColumns > 0) grid_columns_ = settings_.DelanxGridColumns;

  // Caratulas: se encolan y se descargan poco a poco, sin bloquear la interfaz.
  for (const auto& app : library_.apps) queue_artwork(app);

  if (settings_.FirstStartup) {
    toast("Bienvenido a Delanx. Pulsa + para los ajustes.");
    settings_.FirstStartup = false;
    save_settings();
  }
  return true;
}

void App::shutdown() {
  save_library();
  save_settings();
  icon_cache_.clear();
  renderer_.shutdown();
  input_.shutdown();
  net::global_shutdown();
  platform::shutdown();
}

int App::run() {
  while (!input_.quit_requested() && !launching_) {
    input_.update();
    handle_input();
    // Si la consola ha cambiado de modo (dock/portatil), se reajustan escala y rejilla.
    if (input_.screen_size_changed()) {
      input_.ack_screen_size_changed();
      renderer_.sync_output_size();
      input_.set_screen_size(renderer_.width(), renderer_.height());
      float scale = static_cast<float>(renderer_.width()) / 1280.0f;
      if (scale < 1.0f) scale = 1.0f;
      theme_.apply_scale(scale);
      grid_columns_ = renderer_.width() >= 1600 ? 6 : 4;
      if (settings_.DelanxGridColumns > 0) grid_columns_ = settings_.DelanxGridColumns;
      platform::log("pantalla: %dx%d", renderer_.width(), renderer_.height());
    }
    // Una caratula por fotograma: suficiente para llenar la rejilla sin bloquear la entrada.
    if (!busy_ && !dialog_.open && artwork_queue_size() > 0) {
      process_artwork_queue(1);
    }
    draw();
    renderer_.end_frame();
  }
  return launching_ ? kExitCodeLaunchChain : 0;
}

// ------------------------------------------------------------------ entrada

void App::handle_input() {
  handle_global_buttons();
  if (dialog_.open) {
    if (input_.pressed(Btn::Up) || input_.nav_y() < 0) {
      dialog_.focus = (dialog_.focus + static_cast<int>(dialog_.options.size()) - 1) %
                      static_cast<int>(std::max<size_t>(dialog_.options.size(), 1));
    }
    if (input_.pressed(Btn::Down) || input_.nav_y() > 0) {
      dialog_.focus = (dialog_.focus + 1) % static_cast<int>(std::max<size_t>(dialog_.options.size(), 1));
    }
    if (input_.pressed(Btn::A) && !dialog_.options.empty()) {
      auto action = dialog_.options[static_cast<size_t>(dialog_.focus)].action;
      close_dialog();
      if (action) action();
    }
    if (input_.pressed(Btn::B)) close_dialog();
    return;
  }

  if (details_open_) {
    if (input_.pressed(Btn::B)) {
      details_open_ = false;
      return;
    }
    if (input_.pressed(Btn::A)) on_accept();
    if (input_.pressed(Btn::X)) on_secondary();
    if (input_.pressed(Btn::Y)) on_tertiary();
    return;
  }

  switch (tab_) {
    case Tab::Library:
    case Tab::Catalog:
    case Tab::HarbourMasters: {
      // Pantalla tactil: el primer toque enfoca la tarjeta y el segundo la activa.
      if (input_.pressed(Btn::Touch)) {
        int hit = -1;
        for (const auto& target : hit_targets_) {
          const SDL_Rect& rect = target.second;
          if (input_.pointer_x() >= rect.x && input_.pointer_x() < rect.x + rect.w &&
              input_.pointer_y() >= rect.y && input_.pointer_y() < rect.y + rect.h) {
            hit = target.first;
            break;
          }
        }
        if (hit >= 0) {
          if (hit == focus_) {
            on_accept();
          } else {
            focus_ = hit;
          }
          return;
        }
      }
      const int columns = grid_columns_;
      const int dx = input_.nav_x();
      const int dy = input_.nav_y();
      if (dx != 0) focus_ = std::max(focus_ + dx, 0);
      if (dy != 0) focus_ = std::max(focus_ + dy * columns, 0);
      if (input_.pressed(Btn::A)) on_accept();
      if (input_.pressed(Btn::X)) on_secondary();
      if (input_.pressed(Btn::Y)) on_tertiary();
      break;
    }
    case Tab::Downloads: {
      const std::vector<ListItem> updates = build_update_items();
      if (input_.nav_y() != 0 && !updates.empty()) {
        focus_ = std::clamp(focus_ + input_.nav_y(), 0, static_cast<int>(updates.size()) - 1);
      }
      if (input_.pressed(Btn::Touch)) {
        int hit = -1;
        for (const auto& target : hit_targets_) {
          const SDL_Rect& rect = target.second;
          if (input_.pointer_x() >= rect.x && input_.pointer_x() < rect.x + rect.w &&
              input_.pointer_y() >= rect.y && input_.pointer_y() < rect.y + rect.h) {
            hit = target.first;
            break;
          }
        }
        if (hit >= 0 && hit < static_cast<int>(updates.size())) {
          if (hit == focus_) {
            install_app(updates[static_cast<size_t>(hit)].library_index, false);
          } else {
            focus_ = hit;
          }
          return;
        }
      }
      if (input_.pressed(Btn::A) && !updates.empty()) {
        const int index = std::clamp(focus_, 0, static_cast<int>(updates.size()) - 1);
        install_app(updates[static_cast<size_t>(index)].library_index, false);
      }
      if (input_.pressed(Btn::X)) check_updates();
      if (input_.pressed(Btn::Y)) update_all_pending();
      break;
    }
    case Tab::Settings:
      if (input_.nav_y() != 0) focus_ = std::max(focus_ + input_.nav_y(), 0);
      if (input_.pressed(Btn::Touch)) {
        int hit = -1;
        for (const auto& target : hit_targets_) {
          const SDL_Rect& rect = target.second;
          if (input_.pointer_x() >= rect.x && input_.pointer_x() < rect.x + rect.w &&
              input_.pointer_y() >= rect.y && input_.pointer_y() < rect.y + rect.h) {
            hit = target.first;
            break;
          }
        }
        if (hit >= 0) {
          if (hit == focus_) {
            activate_settings_item();
          } else {
            focus_ = hit;
          }
          return;
        }
      }
      if (input_.pressed(Btn::A)) activate_settings_item();
      break;
    default:
      break;
  }
  if (input_.pressed(Btn::Plus)) on_start();
}

void App::handle_global_buttons() {
  if (input_.pressed(Btn::Minus)) {
    open_search();
  }
  if (input_.pressed(Btn::L)) {
    int index = static_cast<int>(tab_) - 1;
    if (index < 0) index = static_cast<int>(Tab::Count) - 1;
    tab_ = static_cast<Tab>(index);
    focus_ = 0;
    scroll_ = 0;
    // Las zonas pulsables son del fotograma anterior: un toque simultaneo al cambio de
    // pestana no debe golpear los elementos de la pestana que acabamos de dejar.
    hit_targets_.clear();
  }
  if (input_.pressed(Btn::R)) {
    int index = (static_cast<int>(tab_) + 1) % static_cast<int>(Tab::Count);
    tab_ = static_cast<Tab>(index);
    focus_ = 0;
    scroll_ = 0;
    hit_targets_.clear();
  }
}

void App::on_start() {
  // + abre/cierra el menu rapido de acciones sobre el elemento enfocado.
  switch (tab_) {
    case Tab::Library:
      if (!library_.apps.empty()) {
        const size_t index = std::min<size_t>(static_cast<size_t>(focus_), library_.apps.size() - 1);
        details_index_ = static_cast<int>(index);
        details_open_ = true;
      }
      break;
    case Tab::Catalog:
      // + cicla la fuente igual que Y (el menu rapido de acciones no aplica aqui).
      cycle_catalog_mode();
      break;
    case Tab::HarbourMasters:
      toast("Harbour Masters: los ports requieren tu ROM y generar los assets en PC.");
      break;
    default:
      break;
  }
}

// ------------------------------------------------------------------ dibujo

void App::draw() {
  // Las zonas pulsables se rehacen en cada fotograma: si no, una pestaña sin rejilla
  // (ajustes, actualizaciones) reutilizaria las tarjetas del fotograma anterior.
  hit_targets_.clear();
  draw_background();
  draw_sidebar();
  draw_topbar();

  switch (tab_) {
    case Tab::Library:
      draw_list_screen("Biblioteca", "Delanx", build_library_items(), "Sin apps. Anade desde el catalogo.");
      draw_library_filters();
      break;    case Tab::Catalog:
      draw_list_screen(catalog_source_title(), catalog_source_origin(), build_catalog_items(),
                       catalog_empty_text());
      draw_catalog_filters();
      break;
    case Tab::HarbourMasters:
      draw_list_screen("Harbour Masters", "Ports de decompilacion (N64)", build_harbour_items(),
                       "Sin entradas de Harbour Masters.");
      break;
    case Tab::Downloads:
      draw_downloads_screen();
      break;
    case Tab::Settings:
      draw_settings_screen();
      break;
    default:
      break;
  }

  draw_footer();
  if (details_open_) draw_details_overlay();
  draw_busy_overlay();
  draw_dialog();
  draw_toast();
  draw_pointer();
}

void App::draw_pointer() {
  if (!input_.pointer_visible()) return;
  const int x = input_.pointer_x();
  const int y = input_.pointer_y();
  if (input_.held(Btn::Touch)) {
    // Toque activo: circulo con halo para saber donde estas pulsando.
    renderer_.fill_circle(x, y, theme_.px(20), theme_.accent.with_alpha(60));
    renderer_.fill_circle(x, y, theme_.px(9), theme_.accent);
  } else {
    // Cursor movido con el stick derecho.
    renderer_.fill_circle(x, y, theme_.px(11), theme_.text.with_alpha(50));
    renderer_.fill_circle(x, y, theme_.px(4), theme_.text);
  }
}

void App::draw_background() {
  renderer_.clear(theme_.bg);
  renderer_.vertical_gradient(0, 0, renderer_.width(), renderer_.height(), theme_.bg_gradient_top, theme_.bg, 18);
}

void App::draw_sidebar() {
  const int width = theme_.sidebar_width;
  const int height = renderer_.height();
  renderer_.fill_rect(0, 0, width, height, theme_.sidebar);

  // Cabecera de marca
  renderer_.fill_round_rect(theme_.px(18), theme_.px(20), theme_.px(44), theme_.px(44), theme_.px(12),
                            theme_.accent);
  renderer_.draw_text("D", theme_.px(18) + theme_.px(15), theme_.px(24), theme_.bg, theme_.font_header, true);
  renderer_.draw_text("Delanx", theme_.px(72), theme_.px(24), theme_.text, theme_.font_title, true);
  renderer_.draw_text("Decomp Launcher NX", theme_.px(72), theme_.px(54), theme_.text_muted, theme_.font_small);
  renderer_.draw_text(std::string("v") + DELANX_VERSION, theme_.px(72), theme_.px(74), theme_.text_muted,
                      theme_.font_small);

  struct Entry {
    Tab tab;
    const char* label;
    const char* hint;
  };
  const Entry entries[] = {
      {Tab::Library, "Biblioteca", "tus apps"},
      {Tab::Catalog, "Catalogo", "instalar"},
      {Tab::HarbourMasters, "Harbour Masters", "ports N64"},
      {Tab::Downloads, "Actualizaciones", "releases"},
      {Tab::Settings, "Ajustes", "consola"},
  };

  int y = theme_.px(120);
  for (const Entry& entry : entries) {
    const bool active = (tab_ == entry.tab);
    const int item_height = theme_.px(58);
    const int x = theme_.px(10);
    const int w = width - theme_.px(20);
    if (active) {
      renderer_.fill_round_rect(x, y, w, item_height - theme_.px(6), theme_.px(10),
                                theme_.panel_alt);
      renderer_.fill_round_rect(x, y, theme_.px(4), item_height - theme_.px(6), theme_.px(2), theme_.accent);
    }
    const Color label_color = active ? theme_.text : theme_.text_dim;
    renderer_.draw_text(entry.label, theme_.px(24), y + theme_.px(8), label_color, theme_.font_body, active);
    renderer_.draw_text(entry.hint, theme_.px(24), y + theme_.px(32), theme_.text_muted, theme_.font_small);
    y += item_height;
  }

  // Pie del sidebar: estado de arranque y red
  const int footer_y = height - theme_.px(96);
  renderer_.draw_text(boot_origin_description(), theme_.px(20), footer_y, theme_.text_muted,
                      theme_.font_small);
  const bool online = platform::network_connected();
  renderer_.fill_circle(theme_.px(26), footer_y + theme_.px(32), theme_.px(5),
                        online ? theme_.success : theme_.danger);
  renderer_.draw_text(online ? "En linea" : "Sin red", theme_.px(40), footer_y + theme_.px(23),
                      theme_.text_dim, theme_.font_small);
}

void App::draw_topbar() {
  const int x = theme_.sidebar_width;
  const int width = renderer_.width() - x;
  renderer_.fill_rect(x, 0, width, theme_.topbar_height, theme_.panel);

  // std::string (y no const char*) para poder usar el titulo de la fuente activa del catalogo.
  std::string title = "Delanx";
  switch (tab_) {
    case Tab::Library: title = "Biblioteca"; break;
    case Tab::Catalog: title = catalog_source_title(); break;
    case Tab::HarbourMasters: title = "Harbour Masters"; break;
    case Tab::Downloads: title = "Actualizaciones"; break;
    case Tab::Settings: title = "Ajustes"; break;
    default: break;
  }
  renderer_.draw_text(title, x + theme_.content_padding, theme_.px(16), theme_.text, theme_.font_title, true);

  // Reloj, bateria y firmware a la derecha
  const int right = renderer_.width() - theme_.content_padding;
  const std::string clock = clock_hhmm_local();
  renderer_.draw_text_right(clock, right, theme_.px(18), theme_.text, theme_.font_body, true);

  const int battery = platform::battery_percent();
  if (battery >= 0) {
    const std::string label = str::format("%d%%", battery);
    const int label_width = renderer_.text_width(label, theme_.font_small);
    const int battery_x = right - label_width - theme_.px(28);
    renderer_.stroke_round_rect(battery_x, theme_.px(22), theme_.px(22), theme_.px(12), theme_.px(3),
                               theme_.text_dim, 1);
    const int fill = std::max(1, theme_.px(18) * battery / 100);
    renderer_.fill_rect(battery_x + theme_.px(2), theme_.px(24), fill, theme_.px(8),
                        battery > 20 ? theme_.success : theme_.danger);
    renderer_.draw_text_right(label, right, theme_.px(18), theme_.text_dim, theme_.font_small);
  } else {
    renderer_.draw_text_right(platform::firmware_version(), right, theme_.px(18), theme_.text_muted,
                              theme_.font_small);
  }

  if (applet_mode_) {
    const std::string warning = "Modo Album (applet): memoria reducida";
    const int w = renderer_.text_width(warning, theme_.font_small) + theme_.px(20);
    const int wx = x + width - w - theme_.px(150);
    renderer_.fill_round_rect(wx, theme_.px(14), w, theme_.px(28), theme_.px(8), theme_.warning.with_alpha(40));
    renderer_.draw_text(warning, wx + theme_.px(10), theme_.px(19), theme_.warning, theme_.font_small);
  }
}

void App::draw_footer() {
  const int height = theme_.footer_height;
  const int y = renderer_.height() - height;
  renderer_.fill_rect(theme_.sidebar_width, y, renderer_.width() - theme_.sidebar_width, height, theme_.panel);

  int x = theme_.sidebar_width + theme_.content_padding;
  draw_button_hint(Btn::A, details_open_ ? "Accion" : "Detalles", x, y);
  x += theme_.px(150);
  draw_button_hint(Btn::X, "Instalar/Actualizar", x, y);
  x += theme_.px(230);
  draw_button_hint(Btn::Y, "Lanzar", x, y);
  x += theme_.px(120);
  draw_button_hint(Btn::B, details_open_ ? "Cerrar" : "Atras", x, y);
  x += theme_.px(120);
  draw_button_hint(Btn::Plus, "Menu", x, y);
  x += theme_.px(110);
  draw_button_hint(Btn::Minus, "Buscar", x, y);
  x += theme_.px(130);
  draw_button_hint(Btn::L, "R pestanas", x, y);

  const std::string status = str::format("%s | %s", input_.hint_text().c_str(),
                                         str::human_size(static_cast<uint64_t>(free_space_bytes())).c_str());
  renderer_.draw_text_right(status + " libres", renderer_.width() - theme_.content_padding,
                            y + theme_.px(14), theme_.text_muted, theme_.font_small);
}

void App::draw_button_hint(Btn button, const std::string& label, int x, int y) {
  const int center_y = y + theme_.footer_height / 2;
  const int radius = theme_.px(13);
  renderer_.fill_circle(x + radius, center_y, radius, theme_.panel_alt);
  renderer_.draw_text_centered(button_glyph(button), x + radius, center_y - theme_.px(9), theme_.text,
                               theme_.font_small, true);
  renderer_.draw_text(label, x + radius * 2 + theme_.px(8), center_y - theme_.px(10), theme_.text_dim,
                      theme_.font_small);
}

void App::draw_toast() {
  if (toast_.text.empty()) return;
  const uint32_t now = SDL_GetTicks();
  if (now > toast_.until_ms) {
    toast_.text.clear();
    return;
  }
  const int width = std::min(renderer_.text_width(toast_.text, theme_.font_body) + theme_.px(48),
                             renderer_.width() - theme_.px(80));
  const int x = (renderer_.width() - width) / 2;
  const int y = renderer_.height() - theme_.footer_height - theme_.px(72);
  renderer_.fill_round_rect(x, y, width, theme_.px(48), theme_.px(12),
                            toast_.error ? theme_.danger.with_alpha(230) : theme_.panel_alt.with_alpha(240));
  renderer_.draw_text_centered(toast_.text, renderer_.width() / 2, y + theme_.px(13),
                              toast_.error ? Color::rgb(255, 255, 255) : theme_.text, theme_.font_body);
}

void App::draw_dialog() {
  if (!dialog_.open) return;
  const int overlay_h = renderer_.height();
  renderer_.fill_rect(0, 0, renderer_.width(), overlay_h, theme_.overlay);

  const int width = std::min(theme_.px(760), renderer_.width() - theme_.px(120));
  const int body_lines = static_cast<int>(renderer_.wrap_text(dialog_.body, theme_.font_body,
                                                              width - theme_.px(56)).size());
  const int height = theme_.px(120) + body_lines * (renderer_.font_height(theme_.font_body) + 2) +
                     static_cast<int>(dialog_.options.size()) * theme_.px(46);
  const int x = (renderer_.width() - width) / 2;
  const int y = (renderer_.height() - height) / 2;

  renderer_.fill_round_rect(x, y, width, height, theme_.px(16), theme_.panel);
  renderer_.stroke_round_rect(x, y, width, height, theme_.px(16), theme_.border, 1);
  renderer_.draw_text(dialog_.title, x + theme_.px(28), y + theme_.px(22), theme_.text, theme_.font_title, true);
  renderer_.draw_text_wrapped(dialog_.body, x + theme_.px(28), y + theme_.px(64), theme_.text_dim,
                              theme_.font_body, width - theme_.px(56));

  int option_y = y + height - static_cast<int>(dialog_.options.size()) * theme_.px(46) - theme_.px(16);
  for (size_t index = 0; index < dialog_.options.size(); ++index) {
    const bool focused = static_cast<int>(index) == dialog_.focus;
    const int option_x = x + theme_.px(20);
    const int option_w = width - theme_.px(40);
    if (focused) {
      renderer_.fill_round_rect(option_x, option_y, option_w, theme_.px(40), theme_.px(10), theme_.card_focus);
      renderer_.fill_round_rect(option_x, option_y, theme_.px(4), theme_.px(40), theme_.px(2), theme_.accent);
    }
    renderer_.draw_text(dialog_.options[index].label, option_x + theme_.px(20), option_y + theme_.px(9),
                        focused ? theme_.text : theme_.text_dim, theme_.font_body, focused);
    option_y += theme_.px(46);
  }
}

void App::draw_busy_overlay() {
  if (!busy_) return;
  renderer_.fill_rect(0, 0, renderer_.width(), renderer_.height(), theme_.overlay);
  const int width = theme_.px(520);
  const int height = theme_.px(140);
  const int x = (renderer_.width() - width) / 2;
  const int y = (renderer_.height() - height) / 2;
  renderer_.fill_round_rect(x, y, width, height, theme_.px(16), theme_.panel);
  renderer_.draw_text_centered(busy_label_, renderer_.width() / 2, y + theme_.px(34), theme_.text,
                               theme_.font_body, true);

  // Puntos animados: feedback de que la app sigue viva durante una llamada de red.
  const uint32_t elapsed = SDL_GetTicks() - busy_started_ms_;
  const int dots = static_cast<int>((elapsed / 350) % 4);
  std::string spinner;
  for (int i = 0; i < dots; ++i) spinner += ".";
  renderer_.draw_text_centered(spinner, renderer_.width() / 2, y + theme_.px(68), theme_.accent,
                               theme_.font_title, true);
  renderer_.draw_text_centered("Pulsa B para cancelar", renderer_.width() / 2, y + theme_.px(104),
                               theme_.text_muted, theme_.font_small);
}

// ------------------------------------------------------------------ utilidades de UI

void App::toast(const std::string& text, bool error) {
  toast_.text = text;
  toast_.error = error;
  toast_.until_ms = SDL_GetTicks() + kToastMs;
}

void App::open_dialog(const std::string& title, const std::string& body, std::vector<DialogOption> options) {
  dialog_.open = true;
  dialog_.title = title;
  dialog_.body = body;
  dialog_.options = std::move(options);
  dialog_.focus = 0;
}

void App::close_dialog() {
  dialog_.open = false;
  dialog_.options.clear();
}

void App::show_confirm(const std::string& title, const std::string& body, const std::string& accept_label,
                       std::function<void()> on_accept_action) {
  std::vector<DialogOption> options;
  options.push_back(DialogOption{accept_label, std::move(on_accept_action)});
  options.push_back(DialogOption{"Cancelar", nullptr});
  open_dialog(title, body, std::move(options));
}

void App::begin_busy(const std::string& label) {
  busy_ = true;
  busy_label_ = label;
  busy_started_ms_ = SDL_GetTicks();
  draw();
  renderer_.end_frame();
}

void App::end_busy() { busy_ = false; }

Texture* App::icon_for(const AppEntry& app) {
  const std::string key = !app.folder_name.empty() ? app.folder_name : app.name;
  if (key.empty()) return nullptr;

  auto it = icon_cache_.find(key);
  if (it != icon_cache_.end()) return it->second.get();

  // Si tiene URL remota, encolar descarga en segundo plano para cachear en SD
  if (!app.app_icon_url.empty()) {
    queue_artwork(app);
  }

  // 1. Intentar cargar desde la tarjeta SD (Cache/artwork/<folderName>.png)
  const std::string cached = app.icon_cache_path();
  if (paths::exists(cached)) {
    auto holder = std::make_unique<Texture>();
    if (holder->load(renderer_.sdl(), cached)) {
      Texture* raw = holder.get();
      icon_cache_[key] = std::move(holder);
      return raw;
    }
  }

  // 2. Fallback local RomFS (modo avion, sin conexion o fallo de descarga)
  std::vector<std::string> romfs_candidates;
  if (!app.folder_name.empty()) {
    romfs_candidates.push_back("romfs:/icons/" + app.folder_name + ".png");
  }
  if (!app.catalog_entry_id.empty()) {
    romfs_candidates.push_back("romfs:/icons/" + app.catalog_entry_id + ".png");
  }
  romfs_candidates.push_back("romfs:/icons/default.png");

  for (const auto& romfs_path : romfs_candidates) {
    if (paths::exists(romfs_path)) {
      auto holder = std::make_unique<Texture>();
      if (holder->load(renderer_.sdl(), romfs_path)) {
        Texture* raw = holder.get();
        icon_cache_[key] = std::move(holder);
        return raw;
      }
    }
  }

  return nullptr;
}

std::string App::display_name(const AppEntry& app) const {
  return compose_display_name(app, settings_.NameStyle);
}

// ------------------------------------------------------------------ datos

void App::load_library_and_settings() {
  settings_ = Settings::load(paths::settings_file());
  settings_.ensure_initialized();
  auto library = Library::load(paths::library_file());
  if (library) {
    library_ = std::move(*library);
  } else {
    // Primera ejecucion: la biblioteca arranca con el catalogo Harbour Masters local.
    library_ = Library{};
  }
  for (auto& app : library_.apps) app.refresh_install_state();
}

void App::load_local_catalogs() {
  auto catalog = Catalog::load_romfs("catalog/switch-catalog.json");
  if (catalog) local_catalog_ = std::move(*catalog);
  auto harbour = Catalog::load_romfs("catalog/harbourmasters.json");
  if (harbour) harbour_catalog_ = std::move(*harbour);

  // Tercera fuente: catalogo maestro del usuario. Lo genera otro proceso, asi que puede no
  // estar todavia: en ese caso la pestana queda vacia y solo se deja constancia en el log.
  auto master = Catalog::load_romfs("catalog/ports-master.json");
  if (master) {
    master_catalog_ = std::move(*master);
  } else {
    master_catalog_ = Catalog{};
    platform::log("catalogo maestro no disponible (romfs:/catalog/ports-master.json): la fuente "
                  "'Catalogo maestro' queda vacia");
  }

  platform::log("catalogo local: %lu entradas; maestro: %lu; harbour masters: %lu (%lu no disponibles)",
                static_cast<unsigned long>(local_catalog_.entries.size()),
                static_cast<unsigned long>(master_catalog_.entries.size()),
                static_cast<unsigned long>(harbour_catalog_.entries.size()),
                static_cast<unsigned long>(harbour_catalog_.unavailable.size()));
}

void App::save_library() {
  if (!library_.save(paths::library_file())) {
    platform::log("no se pudo guardar apps.json");
  }
}

void App::save_settings() { settings_.save(paths::settings_file()); }

}  // namespace delanx::ui
