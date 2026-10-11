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
  Color bg = Color::rgb(18, 20, 24);
  Color bg_gradient_top = Color::rgb(26, 30, 38);
  Color panel = Color::rgb(30, 33, 39);
  Color panel_alt = Color::rgb(38, 42, 50);
  Color card = Color::rgb(42, 47, 56);
  Color card_focus = Color::rgb(52, 58, 69);
  Color border = Color::rgb(51, 57, 66);
  Color text = Color::rgb(242, 244, 248);
  Color text_dim = Color::rgb(155, 163, 176);
  Color text_muted = Color::rgb(110, 118, 130);
  Color accent = Color::rgb(0, 195, 227);       // azul neon Switch
  Color accent_alt = Color::rgb(255, 69, 84);   // rojo neon Switch
  Color success = Color::rgb(91, 212, 122);
  Color warning = Color::rgb(255, 194, 75);
  Color danger = Color::rgb(255, 92, 92);
  Color focus_ring = Color::rgb(0, 195, 227);
  Color overlay = Color::rgb(0, 0, 0, 165);
  Color sidebar = Color::rgb(24, 27, 33);
  Color sidebar_active = Color::rgb(0, 195, 227);

  // Metricas (pensadas para 1280x720; se escalan con `scale`)
  int sidebar_width = 232;
  int topbar_height = 58;
  int footer_height = 46;
  int card_radius = 12;
  int gap = 14;
  int content_padding = 22;
  int icon_size = 96;
  int list_row_height = 96;
  int scrollbar_width = 6;
  int font_small = 17;
  int font_body = 21;
  int font_title = 26;
  int font_header = 32;

  float scale = 1.0f;
  bool dark = true;
  std::string font_regular = "romfs:/fonts/Inter-Regular.ttf";
  std::string font_bold = "romfs:/fonts/Inter-SemiBold.ttf";

  // Tema por defecto (oscuro).
  static Theme sphaira_dark();
  // Deriva un tema claro a partir del oscuro.
  static Theme sphaira_light();

  void apply_scale(float factor);
  int px(int value) const;  // aplica la escala a una metrica
};

}  // namespace delanx::ui
