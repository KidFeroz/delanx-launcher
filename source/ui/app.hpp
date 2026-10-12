// Delanx — aplicacion: estado global, bucle principal y pantallas.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "model/app_entry.hpp"
#include "model/catalog.hpp"
#include "model/settings.hpp"
#include "services/quiver_catalog.hpp"
#include "ui/input.hpp"
#include "ui/renderer.hpp"
#include "ui/theme.hpp"

namespace delanx::ui {

enum class Tab { Library = 0, Catalog, MrShip, Homebrew, Downloads, Settings, Count };

// Fuente activa de la pestana Catalogo: Y (y el boton +) ciclan entre las tres.
//   Delanx  -> romfs:/catalog/switch-catalog.json (entradas verificadas)
//   Master  -> romfs:/catalog/ports-master.json  (catalogo maestro del usuario, sin verificar)
//   Quiver  -> api.quiverlauncher.com            (unica fuente online, con buscador)
enum class CatalogMode { Delanx = 0, Master, Quiver };

struct Toast {
  std::string text;
  bool error = false;
  uint32_t until_ms = 0;
};

struct DialogOption {
  std::string label;
  std::function<void()> action;
};

struct Dialog {
  bool open = false;
  std::string title;
  std::string body;
  std::vector<DialogOption> options;
  int focus = 0;
};

struct DownloadItem {
  std::string app_name;
  std::string version;
  std::string status;
  bool ok = false;
};

// Elemento generico de lista: lo rellenan las tres pantallas de navegacion.
struct ListItem {
  std::string title;
  std::string subtitle;
  std::string badge;    // "Instalado", "Actualiza", "Sin verificar"...
  std::string meta;     // versiones, tamano...
  std::string icon_url;
  bool installed = false;
  bool update = false;
  bool mrship = false;
  bool unavailable = false;
  bool verified = false;
  bool unverified = false;  // chapa "Sin verificar" (catalogo maestro) en color de aviso
  int library_index = -1;
  int catalog_index = -1;
  int mrship_index = -1;
  std::string unavailable_reason;
};

class App {
 public:
  bool init();
  int run();
  void shutdown();

  // Indicador de salida: 0 normal, 2 cuando hay que encadenar otro NRO al salir.
  bool launching() const { return launching_; }

 private:
  // ---- bucle ----
  void handle_input();
  void draw();
  void handle_global_buttons();

  // ---- estructura de la ventana ----
  void draw_background();
  void draw_topbar();
  void draw_tab_bar();
  void draw_sidebar();
  void draw_footer();
  void draw_toast();
  void draw_dialog();
  void draw_busy_overlay();
  void draw_details_overlay();
  void draw_controller_test_overlay();
  void draw_pointer();

  // ---- pantallas ----
  std::vector<ListItem> build_library_items();
  std::vector<ListItem> build_catalog_items();
  std::vector<ListItem> build_mrship_items();
  std::vector<ListItem> build_homebrew_items();
  std::vector<ListItem> build_update_items();
  bool has_switch_build(const CatalogEntry& entry) const;
  void draw_list_screen(const std::string& title, const std::string& subtitle,
                        const std::vector<ListItem>& items, const std::string& empty_text);
  void draw_downloads_screen();
  void draw_settings_screen();
  void draw_catalog_filters();
  void draw_library_filters();
  void activate_settings_item();
  void open_search();

  // ---- catalogo: fuente activa ----
  void cycle_catalog_mode();                 // Delanx -> Master -> Quiver -> Delanx
  std::string catalog_source_title() const;  // titulo corto ("Catalogo maestro")
  std::string catalog_source_origin() const; // de donde salen los datos (ruta o URL)
  std::string catalog_source_label() const;  // "Fuente: ..." para la barra de filtros
  std::string catalog_empty_text() const;    // aviso cuando la fuente no tiene entradas
  // Metadatos informativos (category/assetType/romSourceHint) de una app de la biblioteca,
  // buscandola en los catalogos cargados; nullptr si no esta en ninguno.
  const CatalogEntry* catalog_metadata_for(const AppEntry& app) const;

  // ---- acciones ----
  void on_accept();      // A
  void on_secondary();   // X
  void on_tertiary();    // Y
  void on_start();       // +

  void install_app(int library_index, bool force_unverified);
  void update_all_pending();
  void launch_app(int library_index);
  void launch_nro(const std::string& nro_path);
  void uninstall_app(int library_index);
  void add_catalog_entry_to_library(int catalog_index);
  void check_updates();
  void refresh_catalog_page(bool reset);
  void load_local_catalogs();
  void load_library_and_settings();
  void save_library();
  void save_settings();

  // Descarga + instalacion con barra de progreso (bloquea el hilo de UI dibujando cada paso).
  bool run_install(const AppEntry& app, const Release& release, const ReleaseAsset& asset,
                   const std::string& expected_sha256);

  // Consulta de release con modal de espera. Devuelve false y rellena error si falla.
  bool fetch_candidate_release(const AppEntry& app, Release* release, ReleaseAsset* asset, std::string* error);

  // ---- utilidades de dibujo ----
  void toast(const std::string& text, bool error = false);
  void open_dialog(const std::string& title, const std::string& body, std::vector<DialogOption> options);
  void close_dialog();
  void show_confirm(const std::string& title, const std::string& body, const std::string& accept_label,
                    std::function<void()> on_accept);
  void begin_busy(const std::string& label);
  void end_busy();
  void draw_button_hint(Btn button, const std::string& label, int x, int y);
  Texture* icon_for(const AppEntry& app);
  std::string display_name(const AppEntry& app) const;

  // ---- estado ----
  Renderer renderer_;
  Input input_;
  Theme theme_;
  Settings settings_;
  Library library_;
  Catalog local_catalog_;
  Catalog master_catalog_;  // romfs:/catalog/ports-master.json (puede no existir)
  Catalog mrship_catalog_;

  Tab tab_ = Tab::Library;
  int focus_ = 0;
  int scroll_ = 0;
  int grid_columns_ = 4;
  bool in_tab_bar_ = false;
  int tab_hover_ = 0;
  bool hide_unsupported_switch_ = true;
  bool details_open_ = false;
  int details_index_ = -1;
  bool controller_test_open_ = false;

  // Catalogo online (Quiver) y filtros
  std::vector<quiver::CatalogApp> catalog_online_;
  std::string catalog_cursor_;
  bool catalog_done_ = false;
  CatalogMode catalog_mode_ = CatalogMode::Master;
  std::string catalog_console_ = "switch";
  std::string catalog_project_type_;
  std::string catalog_sort_ = "added";
  std::string catalog_search_;
  int catalog_filter_focus_ = 0;
  bool catalog_show_favorites_ = false;

  // Favoritos (por identidad: repository o name)
  bool is_favorite(const std::string& identity) const;
  void toggle_favorite(const std::string& identity);

  // Biblioteca: filtros
  int library_filter_ = 0;  // 0 todos, 1 instalados, 2 actualizaciones, 3 mrship

  std::vector<DownloadItem> downloads_;

  Toast toast_;
  Dialog dialog_;
  bool busy_ = false;
  std::string busy_label_;
  uint32_t busy_started_ms_ = 0;

  bool launching_ = false;
  std::string launch_error_;
  bool applet_mode_ = false;
  uint32_t last_frame_ms_ = 0;
  std::map<std::string, std::unique_ptr<Texture>> icon_cache_;
  // Zonas pulsables dibujadas en el ultimo fotograma (indice de elemento -> rectangulo),
  // para que la pantalla tactil pueda enfocar y activar tarjetas.
  std::vector<std::pair<int, SDL_Rect>> hit_targets_;
  std::vector<std::pair<Tab, SDL_Rect>> hit_tabs_;
  std::vector<std::pair<Btn, SDL_Rect>> hit_footer_hints_;
};

}  // namespace delanx::ui
