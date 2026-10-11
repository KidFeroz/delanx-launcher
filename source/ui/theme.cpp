#include "ui/theme.hpp"

#include <algorithm>

#include "core/strings.hpp"

namespace delanx::ui {

Color Color::hex(const std::string& value, Color fallback) {
  std::string text = str::trim(value);
  if (text.empty() || text[0] != '#') return fallback;
  text = text.substr(1);
  auto component = [&](size_t offset) -> uint8_t {
    if (offset + 2 > text.size()) return 0;
    return static_cast<uint8_t>(strtol(text.substr(offset, 2).c_str(), nullptr, 16));
  };
  if (text.size() >= 6) {
    return Color::rgb(component(0), component(2), component(4));
  }
  if (text.size() >= 3) {
    auto nibble = [&](size_t index) -> uint8_t {
      const char c = text[index];
      const int value_nibble = (c >= '0' && c <= '9') ? c - '0' : (std::tolower(c) - 'a' + 10);
      return static_cast<uint8_t>(value_nibble * 17);
    };
    return Color::rgb(nibble(0), nibble(1), nibble(2));
  }
  return fallback;
}

Color Color::mix(const Color& other, float t) const {
  t = std::clamp(t, 0.0f, 1.0f);
  auto blend = [&](uint8_t a, uint8_t b) {
    return static_cast<uint8_t>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t);
  };
  return Color::rgb(blend(r, other.r), blend(g, other.g), blend(b, other.b), blend(a, other.a));
}

Theme Theme::sphaira_dark() { return Theme{}; }

Theme Theme::sphaira_light() {
  Theme theme;
  theme.dark = false;
  theme.bg = Color::rgb(238, 240, 244);
  theme.bg_gradient_top = Color::rgb(248, 249, 251);
  theme.panel = Color::rgb(255, 255, 255);
  theme.panel_alt = Color::rgb(244, 246, 249);
  theme.card = Color::rgb(255, 255, 255);
  theme.card_focus = Color::rgb(228, 240, 246);
  theme.border = Color::rgb(210, 215, 222);
  theme.text = Color::rgb(24, 27, 33);
  theme.text_dim = Color::rgb(88, 96, 108);
  theme.text_muted = Color::rgb(130, 138, 150);
  theme.accent = Color::rgb(0, 140, 175);
  theme.accent_alt = Color::rgb(214, 48, 62);
  theme.sidebar = Color::rgb(228, 232, 238);
  theme.focus_ring = Color::rgb(0, 140, 175);
  theme.overlay = Color::rgb(20, 22, 26, 140);
  return theme;
}

void Theme::apply_scale(float factor) {
  scale = std::clamp(factor, 0.7f, 1.6f);
  const float s = scale;
  sidebar_width = static_cast<int>(232 * s);
  topbar_height = static_cast<int>(58 * s);
  footer_height = static_cast<int>(46 * s);
  card_radius = static_cast<int>(12 * s);
  gap = static_cast<int>(14 * s);
  content_padding = static_cast<int>(22 * s);
  icon_size = static_cast<int>(96 * s);
  list_row_height = static_cast<int>(96 * s);
  scrollbar_width = static_cast<int>(6 * s);
  font_small = static_cast<int>(17 * s);
  font_body = static_cast<int>(21 * s);
  font_title = static_cast<int>(26 * s);
  font_header = static_cast<int>(32 * s);
}

int Theme::px(int value) const { return static_cast<int>(static_cast<float>(value) * scale); }

}  // namespace delanx::ui
