// Delanx — envoltorio de SDL2 para dibujar (formas, texto con cache, texturas).
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ui/theme.hpp"

namespace delanx::ui {

class Texture {
 public:
  Texture() = default;
  ~Texture();
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&& other) noexcept;
  Texture& operator=(Texture&& other) noexcept;

  bool load(SDL_Renderer* renderer, const std::string& path);
  // Se apropia de una textura ya creada (la usa la cache de texto).
  static Texture adopt(SDL_Texture* raw, int width, int height);
  bool valid() const { return texture_ != nullptr; }
  int width() const { return width_; }
  int height() const { return height_; }
  SDL_Texture* raw() const { return texture_; }

 private:
  Texture(SDL_Texture* raw, int width, int height) : texture_(raw), width_(width), height_(height) {}
  void reset();
  SDL_Texture* texture_ = nullptr;
  int width_ = 0;
  int height_ = 0;
};

class Renderer {
 public:
  bool init(const std::string& window_title, int width, int height, bool fullscreen = true);
  void shutdown();

  int width() const { return width_; }
  int height() const { return height_; }
  // Vuelve a preguntar al renderer su tamano real (tras un cambio de modo dock/portatil).
  void sync_output_size();
  SDL_Renderer* sdl() const { return renderer_; }

  void begin_frame();
  void end_frame();
  void clear(const Color& color);
  void fill_rect(int x, int y, int w, int h, const Color& color);
  void fill_round_rect(int x, int y, int w, int h, int radius, const Color& color);
  void stroke_round_rect(int x, int y, int w, int h, int radius, const Color& color, int thickness = 2);
  void fill_circle(int cx, int cy, int radius, const Color& color);
  void vertical_gradient(int x, int y, int w, int h, const Color& top, const Color& bottom, int steps = 24);

  void draw_text(const std::string& text, int x, int y, const Color& color, int size, bool bold = false);
  void draw_text_centered(const std::string& text, int center_x, int y, const Color& color, int size,
                          bool bold = false);
  void draw_text_right(const std::string& text, int right_x, int y, const Color& color, int size,
                       bool bold = false);
  int text_width(const std::string& text, int size, bool bold = false);
  int font_height(int size, bool bold = false);
  std::vector<std::string> wrap_text(const std::string& text, int size, int max_width, bool bold = false);
  // Devuelve la altura consumida. max_lines = 0 significa sin limite.
  int draw_text_wrapped(const std::string& text, int x, int y, const Color& color, int size, int max_width,
                        int max_lines = 0, bool bold = false);

  Texture* texture(const std::string& path);
  void draw_texture(Texture* texture, int x, int y, int w, int h, uint8_t alpha = 255);
  void draw_nro_icon(Texture* texture, int x, int y, int size, uint8_t alpha = 255);

  void push_clip(int x, int y, int w, int h);
  void pop_clip();

 private:
  struct TextKey {
    std::string text;
    int size = 0;
    bool bold = false;
    uint32_t color = 0;
    bool operator<(const TextKey& other) const;
  };

  TTF_Font* font(int size, bool bold);
  SDL_Texture* cached_text(const std::string& text, int size, bool bold, const Color& color, int* out_w,
                           int* out_h);
  void flush_text_cache();

  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  int width_ = 1280;
  int height_ = 720;
  TTF_Font* font_regular_ = nullptr;
  TTF_Font* font_bold_ = nullptr;
  int font_regular_size_ = 0;
  int font_bold_size_ = 0;
  std::map<TextKey, std::unique_ptr<Texture>> text_cache_;
  std::map<std::string, std::unique_ptr<Texture>> texture_cache_;
  std::vector<SDL_Rect> clip_stack_;
};

}  // namespace delanx::ui
