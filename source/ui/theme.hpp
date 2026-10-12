// Delanx — tema visual (estilo Sphaira: paneles oscuros, acento neon de Switch).
#pragma once

#include <cstdint>
#include <string>

namespace delanx::ui {

struct Color {
  uint8_t r = 0, g = 0, b = 0, a = 255;
  static Color rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { return Color{r, g, b, a}; }
  static Color hex(const std::string& value, Color fallback = Color::rgb(255, 255, 255));
  Color with_alpha(uint8_t alpha) const { return Color{r, g, b, alpha}; }
  Color mix(const Color& other, float t) const;
};

struct Theme {
  // Paleta
  Color bg = Color::rgb(14, 16, 22);
  Color bg_gradient_top = Color::rgb(20, 24, 32);
  Color panel = Color::rgb(24, 28, 38);
  Color panel_alt = Color::rgb(33, 38, 52);
  Color card = Color::rgb(27, 32, 44);
  Color card_focus = Color::rgb(38, 46, 64);
  Color border = Color::rgb(44, 52, 70);
  Color text = Color::rgb(245, 247, 250);
  Color text_dim = Color::rgb(160, 170, 186);
  Color text_muted = Color::rgb(112, 122, 140);
  Color accent = Color::rgb(0, 210, 240);       // azul neon Switch
  Color accent_alt = Color::rgb(255, 69, 84);   // rojo neon Switch
  Color success = Color::rgb(61, 220, 132);
  Color warning = Color::rgb(255, 184, 48);
  Color danger = Color::rgb(255, 82, 82);
  Color focus_ring = Color::rgb(0, 210, 240);
  Color overlay = Color::rgb(8, 10, 14, 225);   // fondo modal profundo
  Color sidebar = Color::rgb(17, 20, 27);
  Color sidebar_active = Color::rgb(0, 210, 240);

  // Metricas (pensadas para 1280x720; se escalan con `scale`)
  int sidebar_width = 0;
  int topbar_height = 52;
  int tab_bar_height = 44;
  int footer_height = 46;
  int card_radius = 12;
  int gap = 16;
  int content_padding = 28;
  int icon_size = 96;
  int list_row_height = 96;
  int scrollbar_width = 6;
  int font_small = 15;
  int font_body = 19;
  int font_title = 24;
  int font_header = 30;

  float scale = 1.0f;
  bool dark = true;
  std::string font_regular = "romfs:/fonts/Inter-Regular.ttf";
  std::string font_bold = "romfs:/fonts/Inter-SemiBold.ttf";

  // Temas Delanx
  static Theme delanx_dark();
  static Theme delanx_light();
  static Theme sphaira_dark() { return delanx_dark(); }
  static Theme sphaira_light() { return delanx_light(); }

  void apply_scale(float factor);
  int px(int value) const;  // aplica la escala a una metrica
};

}  // namespace delanx::ui
