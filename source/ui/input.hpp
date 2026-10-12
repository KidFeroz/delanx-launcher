// Delanx — entrada: mando de Switch (SDL_GameController), sticks con autorrepeticion y tactil.
#pragma once

#include <SDL2/SDL.h>

#include <cstdint>
#include <string>

namespace delanx::ui {

enum class Btn {
  A, B, X, Y,
  Plus, Minus,
  L, R, ZL, ZR,
  Up, Down, Left, Right,
  StickLPress, StickRPress,
  Home, Capture,
  Touch,
  Quit,
  Count
};

class Input {
 public:
  void update();
  void shutdown();

  bool pressed(Btn button) const;
  bool released(Btn button) const;
  bool held(Btn button) const;
  // Autorrepeticion: util para navegar listas con la cruceta o el stick.
  bool repeat(Btn button, uint32_t first_delay_ms = 320, uint32_t repeat_ms = 90);

  int nav_x();  // -1, 0, 1
  int nav_y();

  // Segundo stick / tactil: mueve un cursor en pantalla (con auto-ocultacion por inactividad).
  bool pointer_visible() const;
  int pointer_x() const { return pointer_x_; }
  int pointer_y() const { return pointer_y_; }
  void set_pointer_visible(bool visible);
  void set_screen_size(int width, int height);
  // El puerto SDL2 de Switch cambia el tamano de la ventana al dockear/desdockear: la
  // aplicacion debe reaplicar escala y columnas cuando esto ocurra.
  bool screen_size_changed() const { return screen_size_changed_; }
  void ack_screen_size_changed() { screen_size_changed_ = false; }

  bool quit_requested() const { return quit_requested_; }
  void request_quit() { quit_requested_ = true; }
  bool controller_connected() const { return controller_ != nullptr || hid_active_; }
  std::string hint_text() const;

 private:
  struct ButtonState {
    bool down = false;
    bool down_last = false;
    uint32_t down_since = 0;
    uint32_t last_repeat = 0;
  };

  void open_controller(int device_index);
  int index_of(Btn button) const;
  void set_button(Btn button, bool down);
  int axis_with_deadzone(int axis) const;
  // En Switch la entrada se lee del HID de libnx (SDL2 no expone SDL_GameController alli).
  void update_from_hid();

  SDL_GameController* controller_ = nullptr;
  SDL_JoystickID controller_id_ = -1;
  bool hid_active_ = false;  // entrada leida por libnx (Switch)
  ButtonState buttons_[static_cast<int>(Btn::Count)];
  bool quit_requested_ = false;
  int screen_w_ = 1280;
  int screen_h_ = 720;
  bool screen_size_changed_ = false;
  bool pointer_visible_ = false;
  int pointer_x_ = 640;
  int pointer_y_ = 360;
  float pointer_vx_ = 0.0f;
  float pointer_vy_ = 0.0f;
  uint32_t pointer_last_moved_ms_ = 0;
  uint32_t last_update_ms_ = 0;
};

const char* button_glyph(Btn button);

}  // namespace delanx::ui
