// Delanx — nucleo de la aplicacion: arranque, bucle principal, estructura de
// ventana y acciones.
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

// Adopta en la biblioteca las entradas de catalogo cuyo .nro ya existe en la SD
// (instaladas a mano o por otra herramienta). Dos entradas que comparten
// carpeta (p. ej. Shipwright y Shipwright-Switch en /switch/soh) solo se
// adoptan una vez.
int adopt_installed(Library &library, const Catalog &catalog) {
  int added = 0;
  for (const CatalogEntry &source : catalog.entries) {
    AppEntry entry = source.to_app_entry();
    if (entry.folder_name.empty())
      entry.folder_name = entry.name;
    const std::string dir = str::lower(entry.install_dir());
    // Raices compartidas (SD o /switch) darian falsos positivos con cualquier
    // .nro.
    if (dir == str::lower(paths::to_sd_path("/")) ||
        dir == str::lower(paths::to_sd_path("/switch")) ||
        dir == str::lower(paths::to_sd_path("/switch/")))
      continue;
    entry.refresh_install_state();
    if (!entry.installed)
      continue;
    const std::string key = entry.identity_key();
    bool known = false;
    for (const AppEntry &app : library.apps) {
      if (app.identity_key() == key || str::lower(app.install_dir()) == dir) {
        known = true;
        break;
      }
    }
    if (known)
      continue;
    library.apps.push_back(entry);
    ++added;
  }
  return added;
}

} // namespace

// ------------------------------------------------------------------ ciclo de
// vida

bool App::init() {
  platform::init();
  paths::ensure_layout();
  platform::log("Delanx %s arrancando (origen: %s)", DELANX_VERSION,
                platform::boot_origin().c_str());
  net::global_init();

  applet_mode_ = platform::is_applet_mode();
  theme_ = settings_.is_dark() ? Theme::delanx_dark() : Theme::delanx_light();

  if (!renderer_.init("Delanx", 1280, 720, true)) {
    platform::log("no se pudo crear la ventana SDL: %s", SDL_GetError());
    return false;
  }

  // La escala se ajusta al modo de arranque: en applet mode (Album) hay menos
  // memoria y conviene una interfaz algo mas compacta.
  float scale = static_cast<float>(renderer_.width()) / 1280.0f;
  if (scale < 1.0f)
    scale = 1.0f;
  theme_.apply_scale(scale);
  input_.set_screen_size(renderer_.width(), renderer_.height());
  grid_columns_ = renderer_.width() >= 1600 ? 6 : 4;
  if (settings_.DelanxGridColumns > 0)
    grid_columns_ = settings_.DelanxGridColumns;

  load_library_and_settings();
  load_local_catalogs();
  {
    // HarbourMasters primero: es la fuente mas precisa para los ports N64.
    const int adopted = adopt_installed(library_, mrship_catalog_) +
                        adopt_installed(library_, local_catalog_) +
                        adopt_installed(library_, master_catalog_);
    if (adopted > 0)
      save_library();
  }
  // El tema y la rejilla dependen de settings.json, que se acaba de leer: se
  // reaplican ahora para que el primer fotograma ya use el tema guardado.
  theme_ = settings_.is_dark() ? Theme::delanx_dark() : Theme::delanx_light();
  theme_.apply_scale(scale);
  grid_columns_ = renderer_.width() >= 1600 ? 6 : 4;
  if (settings_.DelanxGridColumns > 0)
    grid_columns_ = settings_.DelanxGridColumns;

  // Caratulas: se encolan y se descargan poco a poco, sin bloquear la interfaz.
  for (const auto &app : library_.apps)
    queue_artwork(app);

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
    // Si la consola ha cambiado de modo (dock/portatil), se reajustan escala y
    // rejilla.
    if (input_.screen_size_changed()) {
      input_.ack_screen_size_changed();
      renderer_.sync_output_size();
      input_.set_screen_size(renderer_.width(), renderer_.height());
      float scale = static_cast<float>(renderer_.width()) / 1280.0f;
      if (scale < 1.0f)
        scale = 1.0f;
      theme_.apply_scale(scale);
      grid_columns_ = renderer_.width() >= 1600 ? 6 : 4;
      if (settings_.DelanxGridColumns > 0)
        grid_columns_ = settings_.DelanxGridColumns;
      platform::log("pantalla: %dx%d", renderer_.width(), renderer_.height());
    }
    // Una caratula por fotograma: suficiente para llenar la rejilla sin
    // bloquear la entrada.
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
  if (controller_test_open_) {
    if (input_.pressed(Btn::B) || input_.pressed(Btn::Plus)) {
      controller_test_open_ = false;
    }
    return;
  }
  handle_global_buttons();
  if (dialog_.open) {
    if (input_.pressed(Btn::Up) || input_.nav_y() < 0) {
      dialog_.focus =
          (dialog_.focus + static_cast<int>(dialog_.options.size()) - 1) %
          static_cast<int>(std::max<size_t>(dialog_.options.size(), 1));
    }
    if (input_.pressed(Btn::Down) || input_.nav_y() > 0) {
      dialog_.focus =
          (dialog_.focus + 1) %
          static_cast<int>(std::max<size_t>(dialog_.options.size(), 1));
    }
    if (input_.pressed(Btn::A) && !dialog_.options.empty()) {
      auto action = dialog_.options[static_cast<size_t>(dialog_.focus)].action;
      close_dialog();
      if (action)
        action();
    }
    if (input_.pressed(Btn::B))
      close_dialog();
    return;
  }

  if (details_open_) {
    if (input_.pressed(Btn::B)) {
      details_open_ = false;
      details_is_homebrew_ = false;
      return;
    }
    if (input_.pressed(Btn::A))
      on_accept();
    if (input_.pressed(Btn::X))
      on_secondary();
    if (input_.pressed(Btn::Y))
      on_tertiary();
    return;
  }

  // Comprobacion tactil prioritaria de pestanas superiores y botones inferiores
  if (input_.pressed(Btn::Touch)) {
    const int px = input_.pointer_x();
    const int py = input_.pointer_y();

    // 1. Toque en pestanas horizontales
    for (const auto &tab_hit : hit_tabs_) {
      const SDL_Rect &r = tab_hit.second;
      if (px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h) {
        if (tab_ != tab_hit.first) {
          tab_ = tab_hit.first;
          focus_ = 0;
          scroll_ = 0;
          in_tab_bar_ = false;
          hit_targets_.clear();
        }
        return;
      }
    }

    // 2. Toque en indicadores de botones del pie (ejecutan la accion directa al
    // primer tap)
    for (const auto &hint_hit : hit_footer_hints_) {
      const SDL_Rect &r = hint_hit.second;
      if (px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h) {
        const Btn b = hint_hit.first;
        if (b == Btn::A)
          on_accept();
        else if (b == Btn::B) {
          if (details_open_) {
            details_open_ = false;
            details_is_homebrew_ = false;
          }
        } else if (b == Btn::X)
          on_secondary();
        else if (b == Btn::Y)
          on_tertiary();
        else if (b == Btn::L) {
          int index = static_cast<int>(tab_) - 1;
          if (index < 0)
            index = static_cast<int>(Tab::Count) - 1;
          tab_ = static_cast<Tab>(index);
          focus_ = 0;
          scroll_ = 0;
          in_tab_bar_ = false;
        } else if (b == Btn::R) {
          int index =
              (static_cast<int>(tab_) + 1) % static_cast<int>(Tab::Count);
          tab_ = static_cast<Tab>(index);
          focus_ = 0;
          scroll_ = 0;
          in_tab_bar_ = false;
        } else if (b == Btn::Minus) {
          show_confirm("Salir de Delanx",
                       "¿Deseas salir y regresar al menú principal (hbmenu)?",
                       "Salir", [this]() { input_.request_quit(); });
        }
        return;
      }
    }
  }

  // Navegacion cuando estamos enfocando la barra de pestanas horizontal con
  // mando/stick
  if (in_tab_bar_) {
    const int dx = input_.nav_x();
    const int dy = input_.nav_y();
    if (dx != 0) {
      const int tab_count = static_cast<int>(Tab::Count);
      tab_hover_ = (tab_hover_ + dx + tab_count) % tab_count;
    }
    // Stick o cruceta hacia abajo vuelve a la lista de contenido
    if (dy > 0 || input_.pressed(Btn::Down) || input_.pressed(Btn::B)) {
      in_tab_bar_ = false;
      return;
    }
    // Pulsar A confirma y cambia inmediatamente a la pestana seleccionada
    if (input_.pressed(Btn::A)) {
      tab_ = static_cast<Tab>(tab_hover_);
      focus_ = 0;
      scroll_ = 0;
      in_tab_bar_ = false;
      return;
    }
    if (input_.pressed(Btn::Plus))
      on_start();
    return;
  }

  // Navegacion en las pantallas de contenido
  switch (tab_) {
  case Tab::Library:
  case Tab::Catalog:
  case Tab::MrShip:
  case Tab::Homebrew: {
    // Pantalla tactil: toque directo en las tarjetas.
    // Si tocas la tarjeta enfocada se activa de inmediato (on_accept), o enfoca
    // con respuesta instantanea.
    if (input_.pressed(Btn::Touch)) {
      int hit = -1;
      for (const auto &target : hit_targets_) {
        const SDL_Rect &rect = target.second;
        if (input_.pointer_x() >= rect.x &&
            input_.pointer_x() < rect.x + rect.w &&
            input_.pointer_y() >= rect.y &&
            input_.pointer_y() < rect.y + rect.h) {
          hit = target.first;
          break;
        }
      }
      if (hit >= 0) {
        // Un solo toque: enfoca y activa exactamente la tarjeta tocada.
        focus_ = hit;
        on_accept();
        return;
      }
    }
    const int columns = grid_columns_;
    const int dx = input_.nav_x();
    const int dy = input_.nav_y();
    // Si estamos en la fila superior y se pulsa arriba, sube a la barra de
    // pestañas
    if (dy < 0 && focus_ < columns) {
      in_tab_bar_ = true;
      tab_hover_ = static_cast<int>(tab_);
      return;
    }
    const int count = current_item_count();
    if (dx != 0)
      focus_ += dx;
    if (dy != 0)
      focus_ += dy * columns;
    focus_ = count > 0 ? std::clamp(focus_, 0, count - 1) : 0;
    if (input_.pressed(Btn::A))
      on_accept();
    if (input_.pressed(Btn::X))
      on_secondary();
    if (input_.pressed(Btn::Y))
      on_tertiary();
    break;
  }
  case Tab::Downloads: {
    const std::vector<ListItem> updates = build_update_items();
    const int dy = input_.nav_y();
    if (dy < 0 && focus_ == 0) {
      in_tab_bar_ = true;
      tab_hover_ = static_cast<int>(tab_);
      return;
    }
    if (dy != 0 && !updates.empty()) {
      focus_ = std::clamp(focus_ + dy, 0, static_cast<int>(updates.size()) - 1);
    }
    if (input_.pressed(Btn::Touch)) {
      int hit = -1;
      for (const auto &target : hit_targets_) {
        const SDL_Rect &rect = target.second;
        if (input_.pointer_x() >= rect.x &&
            input_.pointer_x() < rect.x + rect.w &&
            input_.pointer_y() >= rect.y &&
            input_.pointer_y() < rect.y + rect.h) {
          hit = target.first;
          break;
        }
      }
      if (hit >= 0 && hit < static_cast<int>(updates.size())) {
        focus_ = hit;
        install_app(updates[static_cast<size_t>(hit)].library_index, false);
        return;
      }
    }
    if (input_.pressed(Btn::A) && !updates.empty()) {
      const int index =
          std::clamp(focus_, 0, static_cast<int>(updates.size()) - 1);
      install_app(updates[static_cast<size_t>(index)].library_index, false);
    }
    if (input_.pressed(Btn::X))
      check_updates();
    if (input_.pressed(Btn::Y))
      update_all_pending();
    break;
  }
  case Tab::Settings: {
    const int dy = input_.nav_y();
    if (dy < 0 && focus_ == 0) {
      in_tab_bar_ = true;
      tab_hover_ = static_cast<int>(tab_);
      return;
    }
    if (dy != 0)
      focus_ = std::clamp(focus_ + dy, 0, 12);
    if (input_.pressed(Btn::Touch)) {
      int hit = -1;
      for (const auto &target : hit_targets_) {
        const SDL_Rect &rect = target.second;
        if (input_.pointer_x() >= rect.x &&
            input_.pointer_x() < rect.x + rect.w &&
            input_.pointer_y() >= rect.y &&
            input_.pointer_y() < rect.y + rect.h) {
          hit = target.first;
          break;
        }
      }
      if (hit >= 0) {
        focus_ = hit;
        activate_settings_item();
        return;
      }
    }
    if (input_.pressed(Btn::A))
      activate_settings_item();
    break;
  }
  default:
    break;
  }
  if (input_.pressed(Btn::Plus))
    on_start();
}

void App::handle_global_buttons() {
  // L + R pulsados a la vez abren la busqueda
  if ((input_.held(Btn::L) && input_.pressed(Btn::R)) ||
      (input_.held(Btn::R) && input_.pressed(Btn::L))) {
    open_search();
    return;
  }

  // Boton Menos (-) muestra confirmacion para salir al menu principal
  if (input_.pressed(Btn::Minus)) {
    show_confirm("Salir de Delanx",
                 "¿Deseas salir y regresar al menú principal (hbmenu)?",
                 "Salir", [this]() { input_.request_quit(); });
    return;
  }

  if (input_.pressed(Btn::L)) {
    int index = static_cast<int>(tab_) - 1;
    if (index < 0)
      index = static_cast<int>(Tab::Count) - 1;
    tab_ = static_cast<Tab>(index);
    focus_ = 0;
    scroll_ = 0;
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

int App::current_item_count() {
  switch (tab_) {
  case Tab::Library:
    return static_cast<int>(build_library_items().size());
  case Tab::Catalog:
    return static_cast<int>(build_catalog_items().size());
  case Tab::MrShip:
    return static_cast<int>(build_mrship_items().size());
  case Tab::Homebrew:
    return static_cast<int>(build_homebrew_items().size());
  default:
    return 0;
  }
}

void App::on_start() {
  // + abre/cierra el menu rapido de acciones sobre el elemento enfocado.
  switch (tab_) {
  case Tab::Library: {
    // + abre la ficha de la tarjeta enfocada (no de la posicion cruda en
    // apps.json).
    const std::vector<ListItem> items = build_library_items();
    if (focus_ >= 0 && focus_ < static_cast<int>(items.size())) {
      details_index_ = items[static_cast<size_t>(focus_)].library_index;
      details_open_ = true;
    }
  } break;
  case Tab::Catalog:
    // + cicla la fuente igual que Y (el menu rapido de acciones no aplica
    // aqui).
    cycle_catalog_mode();
    break;
  case Tab::MrShip:
    toast("Ports N64: los ports requieren tu ROM y generar los assets en "
          "PC.");
    break;
  case Tab::Homebrew:
    if (focus_ >= 0 && focus_ < static_cast<int>(homebrew_cache_.size())) {
      open_details_for_homebrew(homebrew_cache_[static_cast<size_t>(focus_)]);
    }
    break;
  default:
    break;
  }
}

// ------------------------------------------------------------------ dibujo

void App::draw() {
  hit_targets_.clear();
  hit_tabs_.clear();
  hit_footer_hints_.clear();
  draw_background();
  draw_sidebar();
  draw_topbar();
  draw_tab_bar();

  switch (tab_) {
  case Tab::Library:
    draw_list_screen("Biblioteca", "Delanx", build_library_items(),
                     "Sin apps. Anade desde el catalogo.");
    draw_library_filters();
    break;
  case Tab::Catalog:
    draw_list_screen(catalog_source_title(), catalog_source_origin(),
                     build_catalog_items(), catalog_empty_text());
    draw_catalog_filters();
    break;
  case Tab::MrShip:
    draw_list_screen("MrShip / Ports", "Ports de decompilacion (N64)",
                     build_mrship_items(), "Sin entradas de ports.");
    break;
  case Tab::Homebrew:
    draw_list_screen("Homebrew", "Aplicaciones homebrew instaladas en /switch",
                     build_homebrew_items(), "Sin apps homebrew en /switch.");
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
  if (details_open_)
    draw_details_overlay();
  if (controller_test_open_)
    draw_controller_test_overlay();
  draw_busy_overlay();
  draw_dialog();
  draw_toast();
  draw_pointer();
}

void App::draw_pointer() {
  if (!input_.pointer_visible())
    return;
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
  renderer_.vertical_gradient(0, 0, renderer_.width(), renderer_.height(),
                              theme_.bg_gradient_top, theme_.bg, 18);
}

void App::draw_sidebar() {
  // Con la barra horizontal de pestañas arriba del catálogo, el sidebar queda
  // colapsado (ancho 0)
}

void App::draw_topbar() {
  const int x = theme_.sidebar_width;
  const int width = renderer_.width() - x;
  renderer_.fill_rect(x, 0, width, theme_.topbar_height, theme_.panel);
  renderer_.fill_rect(x, theme_.topbar_height - 1, width, 1, theme_.border);

  // Logo gráfico oficial: estrictamente el isotipo en PNG
  Texture *logo = renderer_.texture("romfs:/icons/delanx-icon.png");

  int title_x = x + theme_.content_padding;
  if (logo) {
    const int logo_size = theme_.px(34);
    const int logo_y = (theme_.topbar_height - logo_size) / 2;
    renderer_.draw_texture_fit(logo, title_x, logo_y, logo_size, logo_size);
    title_x += logo_size + theme_.px(12);
  }

  // Título de la app con font de marca
  renderer_.draw_text("Delanx", title_x, theme_.px(12), theme_.text,
                      theme_.font_title, true, FontFace::Brand);
  const int delanx_w =
      renderer_.text_width("Delanx", theme_.font_title, true, FontFace::Brand);

  // Subtítulo con tamaño más legible
  const std::string tag = "Decomp Launcher NX";
  renderer_.draw_text(tag, title_x + delanx_w + theme_.px(14), theme_.px(14),
                      theme_.text_dim, theme_.font_body, false,
                      FontFace::Brand);

  // Reloj y bateria a la derecha: se posicionan de derecha a izquierda sin
  // solapamiento
  int right_cursor = renderer_.width() - theme_.content_padding;

  // 1. Reloj con tamaño destacado y legible
  const std::string clock = clock_hhmm_local();
  renderer_.draw_text_right(clock, right_cursor, theme_.px(12), theme_.text,
                            theme_.font_title, true, FontFace::Brand);
  right_cursor -=
      renderer_.text_width(clock, theme_.font_title, true, FontFace::Brand) +
      theme_.px(24);

  // 2. Bateria con indicador de carga y animacion sutil
  const int battery = platform::battery_percent();
  const bool is_charging = platform::charging();
  if (battery >= 0) {
    const std::string label = is_charging ? str::format("%d%%+", battery)
                                          : str::format("%d%%", battery);
    const int label_w = renderer_.text_width(label, theme_.font_body,
                                             is_charging, FontFace::Condensed);
    renderer_.draw_text_right(label, right_cursor, theme_.px(15),
                              is_charging ? theme_.success : theme_.text_dim,
                              theme_.font_body, is_charging,
                              FontFace::Condensed);
    right_cursor -= label_w + theme_.px(10);

    const int batt_w = theme_.px(26);
    const int batt_h = theme_.px(15);
    const int batt_x = right_cursor - batt_w;
    const int batt_y = theme_.px(18);

    // Contorno cuerpo de bateria
    renderer_.stroke_round_rect(batt_x, batt_y, batt_w, batt_h, theme_.px(3),
                                is_charging ? theme_.success : theme_.text_dim,
                                1);
    // Terminal positivo
    renderer_.fill_rect(batt_x + batt_w, batt_y + theme_.px(4), theme_.px(2),
                        batt_h - theme_.px(8),
                        is_charging ? theme_.success : theme_.text_dim);

    // Relleno de carga con animacion de pulso si esta cargando
    const int fill_max = batt_w - theme_.px(4);
    const int fill = std::clamp(fill_max * battery / 100, 1, fill_max);
    Color batt_color = battery > 20 ? theme_.success : theme_.danger;
    if (is_charging) {
      const uint32_t ticks = SDL_GetTicks();
      const float pulse = 0.5f + 0.5f * sinf(ticks * 0.007f);
      batt_color =
          theme_.success.with_alpha(static_cast<uint8_t>(170 + 85 * pulse));
    }
    renderer_.fill_round_rect(batt_x + theme_.px(2), batt_y + theme_.px(2),
                              fill, batt_h - theme_.px(4), theme_.px(2),
                              batt_color);
    right_cursor = batt_x - theme_.px(24);
  } else {
    const std::string fw = platform::firmware_version();
    if (!fw.empty()) {
      renderer_.draw_text_right(fw, right_cursor, theme_.px(18),
                                theme_.text_muted, theme_.font_small, false,
                                FontFace::Condensed);
      right_cursor -= renderer_.text_width(fw, theme_.font_small, false,
                                           FontFace::Condensed) +
                      theme_.px(20);
    }
  }

  if (applet_mode_) {
    const std::string warning = "Modo Album (applet): memoria reducida";
    const int w = renderer_.text_width(warning, theme_.font_small, false,
                                       FontFace::Condensed) +
                  theme_.px(20);
    const int wx = right_cursor - w - theme_.px(10);
    renderer_.fill_round_rect(wx, theme_.px(13), w, theme_.px(26), theme_.px(8),
                              theme_.warning.with_alpha(40));
    renderer_.stroke_round_rect(wx, theme_.px(13), w, theme_.px(26),
                                theme_.px(8), theme_.warning.with_alpha(90), 1);
    renderer_.draw_text(warning, wx + theme_.px(10), theme_.px(17),
                        theme_.warning, theme_.font_small, false,
                        FontFace::Condensed);
  }
}

void App::draw_footer() {
  // Ocultar la barra inferior de fondo si hay modales o pantallas flotantes
  // abiertas
  if (details_open_ || dialog_.open || busy_ || controller_test_open_) {
    return;
  }

  const int height = theme_.footer_height;
  const int y = renderer_.height() - height;
  renderer_.fill_rect(theme_.sidebar_width, y,
                      renderer_.width() - theme_.sidebar_width, height,
                      theme_.panel);
  renderer_.fill_rect(theme_.sidebar_width, y,
                      renderer_.width() - theme_.sidebar_width, 1,
                      theme_.border);

  int x = theme_.sidebar_width + theme_.content_padding;
  auto add_hint = [&](Btn btn, const std::string &label) {
    const int hint_w = theme_.px(28) +
                       renderer_.text_width(label, theme_.font_small, false,
                                            FontFace::Condensed) +
                       theme_.px(8);
    const SDL_Rect hint_rect{x, y, hint_w, height};
    hit_footer_hints_.push_back({btn, hint_rect});
    draw_button_hint(btn, label, x, y);
    x += hint_w + theme_.px(14);
  };

  switch (tab_) {
  case Tab::Library:
    add_hint(Btn::A, "Detalles / Lanzar");
    add_hint(Btn::X, "Instalar / Actualizar");
    add_hint(Btn::Y, "Filtro");
    add_hint(Btn::Minus, "Salir");
    break;
  case Tab::Catalog:
    add_hint(Btn::A, "Añadir / Ver");
    add_hint(Btn::X, "Favorito ★");
    add_hint(Btn::Y, "Filtrar");
    add_hint(Btn::Minus, "Salir");
    break;
  case Tab::MrShip:
    add_hint(Btn::A, "Ficha / Instalar");
    add_hint(Btn::Y, "Ayuda ROMs");
    add_hint(Btn::Minus, "Salir");
    break;
  case Tab::Homebrew:
    add_hint(Btn::A, "Lanzar Homebrew");
    add_hint(Btn::Minus, "Salir");
    break;
  case Tab::Downloads:
    add_hint(Btn::A, "Actualizar seleccion");
    add_hint(Btn::Y, "Actualizar todo");
    add_hint(Btn::X, "Buscar updates");
    add_hint(Btn::Minus, "Salir");
    break;
  case Tab::Settings:
    add_hint(Btn::A, "Modificar");
    add_hint(Btn::Minus, "Salir");
    break;
  default:
    add_hint(Btn::A, "Aceptar");
    add_hint(Btn::B, "Atrás");
    add_hint(Btn::Minus, "Salir");
    break;
  }

  // Estado y espacio libre en SD solo si hay espacio disponible
  const std::string free_str =
      str::human_size(static_cast<uint64_t>(free_space_bytes())) + " libres";
  const int free_w = renderer_.text_width(free_str, theme_.font_small, false,
                                          FontFace::Condensed);
  const int right_bound = renderer_.width() - theme_.content_padding;
  if (x + theme_.px(20) < right_bound - free_w) {
    renderer_.draw_text_right(free_str, right_bound, y + theme_.px(14),
                              theme_.text_muted, theme_.font_small, false,
                              FontFace::Condensed);
  }
}

void App::draw_button_hint(Btn button, const std::string &label, int x, int y) {
  const int center_y = y + theme_.footer_height / 2;
  const int radius = theme_.px(12);
  renderer_.fill_circle(x + radius, center_y, radius, theme_.card_focus);
  renderer_.stroke_circle(x + radius, center_y, radius, theme_.border, 1);
  renderer_.draw_text_centered(button_glyph(button), x + radius,
                               center_y - theme_.px(9), theme_.text,
                               theme_.font_small, true, FontFace::Brand);
  renderer_.draw_text(label, x + radius * 2 + theme_.px(8),
                      center_y - theme_.px(10), theme_.text_dim,
                      theme_.font_small, false, FontFace::Condensed);
}

void App::draw_toast() {
  if (toast_.text.empty())
    return;
  const uint32_t now = SDL_GetTicks();
  if (now > toast_.until_ms) {
    toast_.text.clear();
    return;
  }
  const int width = std::min(
      renderer_.text_width(toast_.text, theme_.font_body) + theme_.px(48),
      renderer_.width() - theme_.px(80));
  const int x = (renderer_.width() - width) / 2;
  const int y = renderer_.height() - theme_.footer_height - theme_.px(72);
  renderer_.fill_round_rect(x, y, width, theme_.px(48), theme_.px(12),
                            toast_.error ? theme_.danger.with_alpha(230)
                                         : theme_.panel_alt.with_alpha(240));
  renderer_.draw_text_centered(
      toast_.text, renderer_.width() / 2, y + theme_.px(13),
      toast_.error ? Color::rgb(255, 255, 255) : theme_.text, theme_.font_body);
}

void App::draw_dialog() {
  if (!dialog_.open)
    return;
  const int overlay_h = renderer_.height();
  renderer_.fill_rect(0, 0, renderer_.width(), overlay_h, theme_.overlay);

  const int width =
      std::min(theme_.px(760), renderer_.width() - theme_.px(120));
  const int body_lines = static_cast<int>(
      renderer_.wrap_text(dialog_.body, theme_.font_body, width - theme_.px(56))
          .size());
  const int height =
      theme_.px(120) +
      body_lines * (renderer_.font_height(theme_.font_body) + 2) +
      static_cast<int>(dialog_.options.size()) * theme_.px(46);
  const int x = (renderer_.width() - width) / 2;
  const int y = (renderer_.height() - height) / 2;

  renderer_.fill_round_rect(x, y, width, height, theme_.px(16), theme_.panel);
  renderer_.stroke_round_rect(x, y, width, height, theme_.px(16), theme_.border,
                              1);
  renderer_.draw_text(dialog_.title, x + theme_.px(28), y + theme_.px(22),
                      theme_.text, theme_.font_title, true);
  renderer_.draw_text_wrapped(dialog_.body, x + theme_.px(28),
                              y + theme_.px(64), theme_.text_dim,
                              theme_.font_body, width - theme_.px(56));

  int option_y = y + height -
                 static_cast<int>(dialog_.options.size()) * theme_.px(46) -
                 theme_.px(16);
  for (size_t index = 0; index < dialog_.options.size(); ++index) {
    const bool focused = static_cast<int>(index) == dialog_.focus;
    const int option_x = x + theme_.px(20);
    const int option_w = width - theme_.px(40);
    if (focused) {
      renderer_.fill_round_rect(option_x, option_y, option_w, theme_.px(40),
                                theme_.px(10), theme_.card_focus);
      renderer_.fill_round_rect(option_x, option_y, theme_.px(4), theme_.px(40),
                                theme_.px(2), theme_.accent);
    }
    renderer_.draw_text(dialog_.options[index].label, option_x + theme_.px(20),
                        option_y + theme_.px(9),
                        focused ? theme_.text : theme_.text_dim,
                        theme_.font_body, focused);
    option_y += theme_.px(46);
  }
}

void App::draw_busy_overlay() {
  if (!busy_)
    return;
  renderer_.fill_rect(0, 0, renderer_.width(), renderer_.height(),
                      theme_.overlay);
  const int width = theme_.px(520);
  const int height = theme_.px(140);
  const int x = (renderer_.width() - width) / 2;
  const int y = (renderer_.height() - height) / 2;
  renderer_.fill_round_rect(x, y, width, height, theme_.px(16), theme_.panel);
  renderer_.draw_text_centered(busy_label_, renderer_.width() / 2,
                               y + theme_.px(34), theme_.text, theme_.font_body,
                               true);

  // Barra de progreso animada con desplazamiento continuo
  const uint32_t elapsed = SDL_GetTicks() - busy_started_ms_;
  const int busy_bar_w = width - theme_.px(120);
  const int busy_bar_h = theme_.px(6);
  const int busy_bar_x = x + theme_.px(60);
  const int busy_bar_y = y + theme_.px(74);
  renderer_.fill_round_rect(busy_bar_x, busy_bar_y, busy_bar_w, busy_bar_h, busy_bar_h / 2, theme_.panel_alt);
  const int busy_sweep_w = theme_.px(90);
  const int busy_sweep_range = busy_bar_w + busy_sweep_w;
  const int busy_sweep_x = busy_bar_x - busy_sweep_w + static_cast<int>((elapsed / 2) % std::max(busy_sweep_range, 1));
  renderer_.push_clip(busy_bar_x, busy_bar_y, busy_bar_w, busy_bar_h);
  renderer_.fill_round_rect(busy_sweep_x, busy_bar_y, busy_sweep_w, busy_bar_h, busy_bar_h / 2, theme_.accent);
  renderer_.pop_clip();

  renderer_.draw_text_centered("Pulsa B para cancelar", renderer_.width() / 2,
                               y + theme_.px(104), theme_.text_muted,
                               theme_.font_small);
}

// ------------------------------------------------------------------ utilidades
// de UI

void App::toast(const std::string &text, bool error) {
  toast_.text = text;
  toast_.error = error;
  toast_.until_ms = SDL_GetTicks() + kToastMs;
}

void App::open_dialog(const std::string &title, const std::string &body,
                      std::vector<DialogOption> options) {
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

void App::show_confirm(const std::string &title, const std::string &body,
                       const std::string &accept_label,
                       std::function<void()> on_accept_action) {
  std::vector<DialogOption> options;
  options.push_back(DialogOption{accept_label, std::move(on_accept_action)});
  options.push_back(DialogOption{"Cancelar", nullptr});
  open_dialog(title, body, std::move(options));
}

void App::begin_busy(const std::string &label) {
  busy_ = true;
  busy_label_ = label;
  busy_started_ms_ = SDL_GetTicks();
  draw();
  renderer_.end_frame();
}

void App::end_busy() { busy_ = false; }

Texture *App::icon_for(const AppEntry &app) {
  const std::string key = !app.folder_name.empty() ? app.folder_name : app.name;
  if (key.empty())
    return nullptr;

  auto it = icon_cache_.find(key);
  if (it != icon_cache_.end())
    return it->second.get();

  // Si tiene URL remota, encolar descarga en segundo plano para cachear en SD
  if (!app.app_icon_url.empty()) {
    queue_artwork(app);
  }

  // 1. Si la app esta instalada en la SD, priorizar el icono oficial de su
  // carpeta (/switch/<app>/icon.png)
  const std::string target_dir = app.install_dir();
  if (!target_dir.empty()) {
    const std::string official_icon = paths::join(target_dir, "icon.png");
    if (paths::exists(official_icon)) {
      auto holder = std::make_unique<Texture>();
      if (holder->load(renderer_.sdl(), official_icon)) {
        Texture *raw = holder.get();
        icon_cache_[key] = std::move(holder);
        return raw;
      }
    }
  }

  // 2. Intentar cargar desde la cache de la tarjeta SD
  // (Cache/artwork/<folderName>.png)
  const std::string cached = app.icon_cache_path();
  if (paths::exists(cached)) {
    auto holder = std::make_unique<Texture>();
    if (holder->load(renderer_.sdl(), cached)) {
      Texture *raw = holder.get();
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

  for (const auto &romfs_path : romfs_candidates) {
    if (paths::exists(romfs_path)) {
      auto holder = std::make_unique<Texture>();
      if (holder->load(renderer_.sdl(), romfs_path)) {
        Texture *raw = holder.get();
        icon_cache_[key] = std::move(holder);
        return raw;
      }
    }
  }

  return nullptr;
}

std::string App::display_name(const AppEntry &app) const {
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
    // Primera ejecucion: la biblioteca arranca con el catalogo Harbour Masters
    // local.
    library_ = Library{};
  }
  for (auto &app : library_.apps)
    app.refresh_install_state();
}

void App::load_local_catalogs() {
  auto catalog = Catalog::load_romfs("catalog/switch-catalog.json");
  if (catalog)
    local_catalog_ = std::move(*catalog);
  auto harbour = Catalog::load_romfs("catalog/harbourmasters.json");
  if (harbour)
    mrship_catalog_ = std::move(*harbour);

  // Tercera fuente: catalogo maestro del usuario. Lo genera otro proceso, asi
  // que puede no estar todavia: en ese caso la pestana queda vacia y solo se
  // deja constancia en el log.
  auto master = Catalog::load_romfs("catalog/ports-master.json");
  if (master) {
    master_catalog_ = std::move(*master);
  } else {
    master_catalog_ = Catalog{};
    platform::log("catalogo maestro no disponible "
                  "(romfs:/catalog/ports-master.json): la fuente "
                  "'Catalogo maestro' queda vacia");
  }

  platform::log("catalogo local: %lu entradas; maestro: %lu; mrship: %lu (%lu "
                "no disponibles)",
                static_cast<unsigned long>(local_catalog_.entries.size()),
                static_cast<unsigned long>(master_catalog_.entries.size()),
                static_cast<unsigned long>(mrship_catalog_.entries.size()),
                static_cast<unsigned long>(mrship_catalog_.unavailable.size()));
}

void App::save_library() {
  if (!library_.save(paths::library_file())) {
    platform::log("no se pudo guardar apps.json");
  }
}

void App::save_settings() { settings_.save(paths::settings_file()); }

} // namespace delanx::ui
