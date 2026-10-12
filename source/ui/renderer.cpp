#include "ui/renderer.hpp"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <cstdio>

#include "core/strings.hpp"

namespace delanx::ui {
namespace {

SDL_Color to_sdl(const Color& color) { return SDL_Color{color.r, color.g, color.b, color.a}; }

uint32_t pack(const Color& color) {
  return (static_cast<uint32_t>(color.r) << 24) | (static_cast<uint32_t>(color.g) << 16) |
         (static_cast<uint32_t>(color.b) << 8) | static_cast<uint32_t>(color.a);
}

}  // namespace

// ---------------------------------------------------------------- Texture

Texture::~Texture() { reset(); }

Texture::Texture(Texture&& other) noexcept : texture_(other.texture_), width_(other.width_), height_(other.height_) {
  other.texture_ = nullptr;
  other.width_ = 0;
  other.height_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
  if (this != &other) {
    reset();
    texture_ = other.texture_;
    width_ = other.width_;
    height_ = other.height_;
    other.texture_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
  }
  return *this;
}

void Texture::reset() {
  if (texture_) SDL_DestroyTexture(texture_);
  texture_ = nullptr;
  width_ = 0;
  height_ = 0;
}

Texture Texture::adopt(SDL_Texture* raw, int width, int height) { return Texture(raw, width, height); }

bool Texture::load(SDL_Renderer* renderer, const std::string& path) {
  reset();
  SDL_Surface* surface = IMG_Load(path.c_str());
  if (!surface) return false;
  texture_ = SDL_CreateTextureFromSurface(renderer, surface);
  width_ = surface->w;
  height_ = surface->h;
  SDL_FreeSurface(surface);
  return texture_ != nullptr;
}

// ---------------------------------------------------------------- Renderer

bool Renderer::init(const std::string& window_title, int width, int height, bool fullscreen) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) != 0) {
    return false;
  }
  if (TTF_Init() != 0) return false;
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

  Uint32 flags = SDL_WINDOW_SHOWN;
  if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

  window_ = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                             flags);
  if (!window_) return false;

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) {
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!renderer_) return false;

  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
  SDL_RenderSetIntegerScale(renderer_, SDL_FALSE);

  int output_w = width;
  int output_h = height;
  if (SDL_GetRendererOutputSize(renderer_, &output_w, &output_h) == 0) {
    width_ = output_w;
    height_ = output_h;
  } else {
    width_ = width;
    height_ = height;
  }
  return true;
}

void Renderer::sync_output_size() {
  if (!renderer_) return;
  int output_w = width_;
  int output_h = height_;
  if (SDL_GetRendererOutputSize(renderer_, &output_w, &output_h) == 0) {
    if (output_w > 0) width_ = output_w;
    if (output_h > 0) height_ = output_h;
  }
  // Las texturas de texto se cachean por tamano de fuente, no por pantalla, pero la cache
  // completa se descarta para que nada quede dibujado con el tamano anterior.
  flush_text_cache();
}

void Renderer::shutdown() {
  flush_text_cache();
  texture_cache_.clear();
  if (font_regular_) TTF_CloseFont(font_regular_);
  if (font_bold_) TTF_CloseFont(font_bold_);
  if (font_brand_) TTF_CloseFont(font_brand_);
  if (font_condensed_) TTF_CloseFont(font_condensed_);
  font_regular_ = nullptr;
  font_bold_ = nullptr;
  font_brand_ = nullptr;
  font_condensed_ = nullptr;
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  IMG_Quit();
  TTF_Quit();
  SDL_Quit();
}

TTF_Font* Renderer::font(int size, bool bold, FontFace face) {
  size = std::max(size, 8);
  TTF_Font** slot_ptr = nullptr;
  int* slot_size_ptr = nullptr;
  const char* candidates[4] = {nullptr};

  if (face == FontFace::Brand) {
    slot_ptr = &font_brand_;
    slot_size_ptr = &font_brand_size_;
    candidates[0] = "romfs:/fonts/CalSans-Regular.ttf";
    candidates[1] = "sdmc:/switch/Delanx/fonts/CalSans-Regular.ttf";
    candidates[2] = "romfs:/fonts/Inter.ttf";
  } else if (face == FontFace::Condensed) {
    slot_ptr = &font_condensed_;
    slot_size_ptr = &font_condensed_size_;
    candidates[0] = "romfs:/fonts/BarlowCondensed-SemiBold.ttf";
    candidates[1] = "sdmc:/switch/Delanx/fonts/BarlowCondensed-SemiBold.ttf";
    candidates[2] = "romfs:/fonts/Inter.ttf";
  } else {
    slot_ptr = bold ? &font_bold_ : &font_regular_;
    slot_size_ptr = bold ? &font_bold_size_ : &font_regular_size_;
    if (bold) {
      candidates[0] = "romfs:/fonts/Inter-SemiBold.ttf";
      candidates[1] = "sdmc:/switch/Delanx/fonts/Inter-SemiBold.ttf";
      candidates[2] = "romfs:/fonts/Inter.ttf";
    } else {
      candidates[0] = "romfs:/fonts/Inter.ttf";
      candidates[1] = "sdmc:/switch/Delanx/fonts/Inter.ttf";
    }
  }

  TTF_Font*& slot = *slot_ptr;
  int& slot_size = *slot_size_ptr;
  if (slot && slot_size == size) return slot;
  if (slot) {
    TTF_CloseFont(slot);
    slot = nullptr;
  }
  for (int i = 0; candidates[i]; ++i) {
    slot = TTF_OpenFont(candidates[i], size);
    if (slot) break;
  }
  if (!slot && bold) {
    for (int i = 0; candidates[i]; ++i) {
      slot = TTF_OpenFont(candidates[i], size);
      if (slot) {
        TTF_SetFontStyle(slot, TTF_STYLE_BOLD);
        break;
      }
    }
  }
  slot_size = size;
  if (slot) TTF_SetFontKerning(slot, 1);
  return slot;
}

bool Renderer::TextKey::operator<(const TextKey& other) const {
  if (text != other.text) return text < other.text;
  if (size != other.size) return size < other.size;
  if (bold != other.bold) return bold < other.bold;
  if (face != other.face) return face < other.face;
  return color < other.color;
}

void Renderer::flush_text_cache() { text_cache_.clear(); }

SDL_Texture* Renderer::cached_text(const std::string& text, int size, bool bold, FontFace face, const Color& color,
                                   int* out_w, int* out_h) {
  TextKey key{text, size, bold, static_cast<uint8_t>(face), pack(color)};
  auto it = text_cache_.find(key);
  if (it != text_cache_.end()) {
    *out_w = it->second->width();
    *out_h = it->second->height();
    return it->second->raw();
  }
  if (text_cache_.size() > 512) flush_text_cache();

  TTF_Font* handle = font(size, bold, face);
  if (!handle) {
    *out_w = 0;
    *out_h = 0;
    return nullptr;
  }
  SDL_Surface* surface = TTF_RenderUTF8_Blended(handle, text.c_str(), to_sdl(color));
  if (!surface) {
    *out_w = 0;
    *out_h = 0;
    return nullptr;
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
  const int w = surface->w;
  const int h = surface->h;
  SDL_FreeSurface(surface);
  if (!texture) {
    *out_w = 0;
    *out_h = 0;
    return nullptr;
  }
  auto holder = std::make_unique<Texture>(Texture::adopt(texture, w, h));
  text_cache_[key] = std::move(holder);
  *out_w = w;
  *out_h = h;
  return texture;
}

void Renderer::begin_frame() {
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void Renderer::end_frame() { SDL_RenderPresent(renderer_); }

void Renderer::clear(const Color& color) {
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer_);
}

void Renderer::fill_rect(int x, int y, int w, int h, const Color& color) {
  if (w <= 0 || h <= 0) return;
  SDL_Rect rect{x, y, w, h};
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer_, &rect);
}

void Renderer::fill_round_rect(int x, int y, int w, int h, int radius, const Color& color) {
  if (w <= 0 || h <= 0) return;
  radius = std::min(radius, std::min(w, h) / 2);
  if (radius <= 0) {
    fill_rect(x, y, w, h, color);
    return;
  }
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  SDL_Rect middle{x, y + radius, w, h - 2 * radius};
  SDL_RenderFillRect(renderer_, &middle);
  for (int row = 0; row < radius; ++row) {
    const double dy = static_cast<double>(radius - row) - 0.5;
    const double dx = std::sqrt(static_cast<double>(radius * radius) - dy * dy);
    const int inset = radius - static_cast<int>(dx + 0.5);
    SDL_Rect line_left{x + inset, y + row, w - 2 * inset, 1};
    SDL_RenderFillRect(renderer_, &line_left);
    SDL_Rect line_bottom{x + inset, y + h - 1 - row, w - 2 * inset, 1};
    SDL_RenderFillRect(renderer_, &line_bottom);
  }
}

void Renderer::stroke_round_rect(int x, int y, int w, int h, int radius, const Color& color, int thickness) {
  if (w <= 0 || h <= 0 || thickness <= 0) return;
  radius = std::min(radius, std::min(w, h) / 2);
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  for (int t = 0; t < thickness; ++t) {
    const int rx = x + t;
    const int ry = y + t;
    const int rw = w - 2 * t;
    const int rh = h - 2 * t;
    if (rw <= 0 || rh <= 0) break;
    SDL_Rect top{rx + radius, ry, rw - 2 * radius, 1};
    SDL_Rect bottom{rx + radius, ry + rh - 1, rw - 2 * radius, 1};
    SDL_Rect left{rx, ry + radius, 1, rh - 2 * radius};
    SDL_Rect right{rx + rw - 1, ry + radius, 1, rh - 2 * radius};
    SDL_RenderFillRect(renderer_, &top);
    SDL_RenderFillRect(renderer_, &bottom);
    SDL_RenderFillRect(renderer_, &left);
    SDL_RenderFillRect(renderer_, &right);
    const int corner_radius = std::max(radius - t, 0);
    if (corner_radius <= 0) continue;
    for (int row = 0; row < corner_radius; ++row) {
      const double dy = static_cast<double>(corner_radius - row) - 0.5;
      const double dx = std::sqrt(static_cast<double>(corner_radius * corner_radius) - dy * dy);
      const int inset = corner_radius - static_cast<int>(dx + 0.5);
      SDL_Rect tl{rx + inset, ry + row, 1, 1};
      SDL_Rect tr{rx + rw - 1 - inset, ry + row, 1, 1};
      SDL_Rect bl{rx + inset, ry + rh - 1 - row, 1, 1};
      SDL_Rect br{rx + rw - 1 - inset, ry + rh - 1 - row, 1, 1};
      SDL_RenderFillRect(renderer_, &tl);
      SDL_RenderFillRect(renderer_, &tr);
      SDL_RenderFillRect(renderer_, &bl);
      SDL_RenderFillRect(renderer_, &br);
    }
  }
}

void Renderer::fill_circle(int cx, int cy, int radius, const Color& color) {
  if (radius <= 0) return;
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  for (int row = -radius; row <= radius; ++row) {
    const double dx = std::sqrt(static_cast<double>(radius * radius - row * row));
    SDL_Rect line{cx - static_cast<int>(dx), cy + row, static_cast<int>(dx) * 2, 1};
    SDL_RenderFillRect(renderer_, &line);
  }
}

void Renderer::stroke_circle(int cx, int cy, int radius, const Color& color, int thickness) {
  if (radius <= 0) return;
  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  for (int t = 0; t < thickness; ++t) {
    const int r = radius - t;
    if (r <= 0) break;
    int x = r;
    int y = 0;
    int err = 0;
    while (x >= y) {
      SDL_RenderDrawPoint(renderer_, cx + x, cy + y);
      SDL_RenderDrawPoint(renderer_, cx + y, cy + x);
      SDL_RenderDrawPoint(renderer_, cx - y, cy + x);
      SDL_RenderDrawPoint(renderer_, cx - x, cy + y);
      SDL_RenderDrawPoint(renderer_, cx - x, cy - y);
      SDL_RenderDrawPoint(renderer_, cx - y, cy - x);
      SDL_RenderDrawPoint(renderer_, cx + y, cy - x);
      SDL_RenderDrawPoint(renderer_, cx + x, cy - y);
      if (err <= 0) {
        y += 1;
        err += 2 * y + 1;
      }
      if (err > 0) {
        x -= 1;
        err -= 2 * x + 1;
      }
    }
  }
}

void Renderer::vertical_gradient(int x, int y, int w, int h, const Color& top, const Color& bottom, int steps) {
  if (w <= 0 || h <= 0) return;
  steps = std::max(steps, 1);
  const int band = std::max(h / steps, 1);
  for (int offset = 0; offset < h; offset += band) {
    const float t = static_cast<float>(offset) / static_cast<float>(h);
    fill_rect(x, y + offset, w, std::min(band, h - offset), top.mix(bottom, t));
  }
}

void Renderer::horizontal_gradient(int x, int y, int w, int h, const Color& left, const Color& right, int steps) {
  if (w <= 0 || h <= 0) return;
  steps = std::max(steps, 1);
  const int band = std::max(w / steps, 1);
  for (int offset = 0; offset < w; offset += band) {
    const float t = static_cast<float>(offset) / static_cast<float>(w);
    fill_rect(x + offset, y, std::min(band, w - offset), h, left.mix(right, t));
  }
}

void Renderer::draw_text(const std::string& text, int x, int y, const Color& color, int size, bool bold,
                         FontFace face) {
  if (text.empty()) return;
  int w = 0;
  int h = 0;
  SDL_Texture* texture = cached_text(text, size, bold, face, color, &w, &h);
  if (!texture) return;
  SDL_Rect dst{x, y, w, h};
  SDL_RenderCopy(renderer_, texture, nullptr, &dst);
}

void Renderer::draw_text_centered(const std::string& text, int center_x, int y, const Color& color, int size,
                                  bool bold, FontFace face) {
  const int w = text_width(text, size, bold, face);
  draw_text(text, center_x - w / 2, y, color, size, bold, face);
}

void Renderer::draw_text_right(const std::string& text, int right_x, int y, const Color& color, int size,
                               bool bold, FontFace face) {
  const int w = text_width(text, size, bold, face);
  draw_text(text, right_x - w, y, color, size, bold, face);
}

int Renderer::text_width(const std::string& text, int size, bool bold, FontFace face) {
  if (text.empty()) return 0;
  int w = 0;
  int h = 0;
  cached_text(text, size, bold, face, Color::rgb(255, 255, 255), &w, &h);
  return w;
}

int Renderer::font_height(int size, bool bold, FontFace face) {
  TTF_Font* handle = font(size, bold, face);
  if (!handle) return size;
  return TTF_FontHeight(handle);
}

std::vector<std::string> Renderer::wrap_text(const std::string& text, int size, int max_width, bool bold,
                                             FontFace face) {
  std::vector<std::string> lines;
  if (max_width <= 0) return lines;
  std::string current;
  size_t index = 0;
  while (index <= text.size()) {
    const size_t space = text.find_first_of(" \n", index);
    const size_t end = (space == std::string::npos) ? text.size() : space;
    const std::string word = text.substr(index, end - index);
    const bool newline = (space != std::string::npos && text[space] == '\n');
    if (!word.empty()) {
      if (text_width(word, size, bold, face) > max_width) {
        if (!current.empty()) {
          lines.push_back(current);
          current.clear();
        }
        std::string chunk;
        for (char ch : word) {
          if (text_width(chunk + ch, size, bold, face) > max_width && !chunk.empty()) {
            lines.push_back(chunk);
            chunk.clear();
          }
          chunk += ch;
        }
        current = chunk;
      } else {
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (text_width(candidate, size, bold, face) > max_width && !current.empty()) {
          lines.push_back(current);
          current = word;
        } else {
          current = candidate;
        }
      }
    }
    if (newline && !current.empty()) {
      lines.push_back(current);
      current.clear();
    }
    if (space == std::string::npos) break;
    index = space + 1;
  }
  if (!current.empty()) lines.push_back(current);
  return lines;
}

int Renderer::draw_text_wrapped(const std::string& text, int x, int y, const Color& color, int size,
                                int max_width, int max_lines, bool bold, FontFace face) {
  const std::vector<std::string> lines = wrap_text(text, size, max_width, bold, face);
  const int line_height = font_height(size, bold, face) + 2;
  int drawn = 0;
  for (const auto& line : lines) {
    if (max_lines > 0 && drawn >= max_lines) break;
    draw_text(line, x, y + drawn * line_height, color, size, bold, face);
    ++drawn;
  }
  return drawn * line_height;
}

Texture* Renderer::texture(const std::string& path) {
  auto it = texture_cache_.find(path);
  if (it != texture_cache_.end()) return it->second.get();
  auto holder = std::make_unique<Texture>();
  if (!holder->load(renderer_, path)) return nullptr;
  Texture* raw = holder.get();
  texture_cache_[path] = std::move(holder);
  return raw;
}

void Renderer::draw_texture(Texture* texture, int x, int y, int w, int h, uint8_t alpha) {
  if (!texture || !texture->valid()) return;
  SDL_SetTextureAlphaMod(texture->raw(), alpha);
  SDL_Rect dst{x, y, w, h};
  SDL_RenderCopy(renderer_, texture->raw(), nullptr, &dst);
  SDL_SetTextureAlphaMod(texture->raw(), 255);
}

void Renderer::draw_texture_fit(Texture* texture, int x, int y, int w, int h, uint8_t alpha) {
  if (!texture || !texture->valid() || w <= 0 || h <= 0) return;
  const float img_w = static_cast<float>(texture->width());
  const float img_h = static_cast<float>(std::max(texture->height(), 1));
  const float ratio = img_w / img_h;
  const float box_ratio = static_cast<float>(w) / static_cast<float>(h);
  int draw_w = w;
  int draw_h = h;
  if (ratio > box_ratio) {
    draw_w = w;
    draw_h = std::max(1, static_cast<int>(static_cast<float>(w) / ratio));
  } else {
    draw_h = h;
    draw_w = std::max(1, static_cast<int>(static_cast<float>(h) * ratio));
  }
  const int dst_x = x + (w - draw_w) / 2;
  const int dst_y = y + (h - draw_h) / 2;
  draw_texture(texture, dst_x, dst_y, draw_w, draw_h, alpha);
}

void Renderer::draw_texture_cover(Texture* texture, int x, int y, int w, int h, uint8_t alpha) {
  if (!texture || !texture->valid() || w <= 0 || h <= 0) return;
  const float img_w = static_cast<float>(texture->width());
  const float img_h = static_cast<float>(std::max(texture->height(), 1));
  const float ratio = img_w / img_h;
  const float box_ratio = static_cast<float>(w) / static_cast<float>(h);
  SDL_Rect src{0, 0, texture->width(), texture->height()};
  if (ratio > box_ratio) {
    const int crop_w = std::max(1, static_cast<int>(img_h * box_ratio));
    src.x = (texture->width() - crop_w) / 2;
    src.w = crop_w;
  } else {
    const int crop_h = std::max(1, static_cast<int>(img_w / box_ratio));
    src.y = (texture->height() - crop_h) / 2;
    src.h = crop_h;
  }
  SDL_SetTextureAlphaMod(texture->raw(), alpha);
  SDL_Rect dst{x, y, w, h};
  SDL_RenderCopy(renderer_, texture->raw(), &src, &dst);
  SDL_SetTextureAlphaMod(texture->raw(), 255);
}

void Renderer::draw_nro_icon(Texture* texture, int x, int y, int size, uint8_t alpha) {
  if (!texture || !texture->valid()) return;
  const float source_ratio = static_cast<float>(texture->width()) / static_cast<float>(std::max(texture->height(), 1));
  int draw_w = size;
  int draw_h = size;
  if (source_ratio > 1.0f) {
    draw_h = static_cast<int>(static_cast<float>(size) / source_ratio);
  } else if (source_ratio < 1.0f) {
    draw_w = static_cast<int>(static_cast<float>(size) * source_ratio);
  }
  draw_texture(texture, x + (size - draw_w) / 2, y + (size - draw_h) / 2, draw_w, draw_h, alpha);
}

void Renderer::push_clip(int x, int y, int w, int h) {
  SDL_Rect rect{x, y, w, h};
  clip_stack_.push_back(rect);
  SDL_RenderSetClipRect(renderer_, &clip_stack_.back());
}

void Renderer::pop_clip() {
  if (!clip_stack_.empty()) clip_stack_.pop_back();
  if (clip_stack_.empty()) {
    SDL_RenderSetClipRect(renderer_, nullptr);
  } else {
    SDL_RenderSetClipRect(renderer_, &clip_stack_.back());
  }
}

}  // namespace delanx::ui
