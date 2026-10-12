#include "ui/input.hpp"

#include <algorithm>
#include <cmath>

#if defined(__SWITCH__)
#include <switch.h>

#include "platform/switch_platform.hpp"
#endif

namespace delanx::ui {
namespace {

constexpr int kStickDeadzone = 9000;
constexpr float kPointerSpeed = 0.9f;

}  // namespace

int Input::index_of(Btn button) const { return static_cast<int>(button); }

bool Input::pointer_visible() const {
  if (!pointer_visible_) return false;
  const uint32_t now = SDL_GetTicks();
  if (pointer_last_moved_ms_ > 0 && (now - pointer_last_moved_ms_) > 2500) {
    return false;
  }
  return true;
}

void Input::set_pointer_visible(bool visible) {
  pointer_visible_ = visible;
  if (visible) pointer_last_moved_ms_ = SDL_GetTicks();
}

void Input::set_screen_size(int width, int height) {
  screen_w_ = width > 0 ? width : screen_w_;
  screen_h_ = height > 0 ? height : screen_h_;
  pointer_x_ = std::clamp(pointer_x_, 0, screen_w_ - 1);
  pointer_y_ = std::clamp(pointer_y_, 0, screen_h_ - 1);
}

void Input::open_controller(int device_index) {
  if (controller_) return;
  if (!SDL_IsGameController(device_index)) return;
  controller_ = SDL_GameControllerOpen(device_index);
  if (controller_) {
    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller_);
    controller_id_ = SDL_JoystickInstanceID(joystick);
  }
}

void Input::shutdown() {
  if (controller_) {
    SDL_GameControllerClose(controller_);
    controller_ = nullptr;
  }
}

void Input::set_button(Btn button, bool down) {
  ButtonState& state = buttons_[index_of(button)];
  if (down && !state.down) {
    state.down_since = SDL_GetTicks();
    state.last_repeat = 0;
  }
  state.down = down;
}

int Input::axis_with_deadzone(int axis) const {
  if (!controller_) return 0;
  const int value = SDL_GameControllerGetAxis(controller_, static_cast<SDL_GameControllerAxis>(axis));
  if (std::abs(value) < kStickDeadzone) return 0;
  return value;
}

void Input::update() {
  const uint32_t now = SDL_GetTicks();
#if !defined(__SWITCH__)
  const float delta = last_update_ms_ == 0 ? 0.016f : static_cast<float>(now - last_update_ms_) / 1000.0f;
#endif
  last_update_ms_ = now;

  for (auto& state : buttons_) state.down_last = state.down;
  // Eventos
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        quit_requested_ = true;
        break;
      case SDL_WINDOWEVENT:
        // El puerto SDL2 de Switch emite este evento al dockear/desdockear. Todas las
        // coordenadas (tactil y puntero) viven en el espacio del renderer, asi que hay que
        // seguir el cambio o los toques se descolocarian.
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          const int new_w = event.window.data1 > 0 ? event.window.data1 : screen_w_;
          const int new_h = event.window.data2 > 0 ? event.window.data2 : screen_h_;
          if (new_w != screen_w_ || new_h != screen_h_) {
            screen_w_ = new_w;
            screen_h_ = new_h;
            screen_size_changed_ = true;
            pointer_x_ = std::clamp(pointer_x_, 0, screen_w_ - 1);
            pointer_y_ = std::clamp(pointer_y_, 0, screen_h_ - 1);
          }
        }
        break;
      case SDL_CONTROLLERDEVICEADDED:
        open_controller(event.cdevice.which);
        break;
      case SDL_CONTROLLERDEVICEREMOVED:
        if (controller_ && event.cdevice.which == controller_id_) {
          SDL_GameControllerClose(controller_);
          controller_ = nullptr;
          controller_id_ = -1;
        }
        break;
      case SDL_JOYDEVICEADDED:
        open_controller(event.jdevice.which);
        break;
      case SDL_FINGERDOWN:
      case SDL_FINGERMOTION: {
        pointer_x_ = static_cast<int>(event.tfinger.x * static_cast<float>(screen_w_));
        pointer_y_ = static_cast<int>(event.tfinger.y * static_cast<float>(screen_h_));
        if (event.type == SDL_FINGERDOWN) set_button(Btn::Touch, true);
        break;
      }
      case SDL_FINGERUP:
        set_button(Btn::Touch, false);
        break;
      case SDL_MOUSEMOTION:
        pointer_x_ = event.motion.x;
        pointer_y_ = event.motion.y;
        break;
      case SDL_MOUSEBUTTONDOWN:
        set_button(Btn::Touch, true);
        break;
      case SDL_MOUSEBUTTONUP:
        set_button(Btn::Touch, false);
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) quit_requested_ = true;
        break;
      default:
        break;
    }
  }

#if !defined(__SWITCH__)
  if (!controller_) {
    // Sin mando: se busca uno disponible (solo en compilaciones de PC).
    for (int index = 0; index < SDL_NumJoysticks() && !controller_; ++index) {
      open_controller(index);
    }
  }

  if (controller_) {
    auto button_state = [&](SDL_GameControllerButton code) {
      return SDL_GameControllerGetButton(controller_, code) != 0;
    };
    auto axis_state = [&](SDL_GameControllerAxis axis) {
      return SDL_GameControllerGetAxis(controller_, axis) > kStickDeadzone;
    };

    set_button(Btn::A, button_state(SDL_CONTROLLER_BUTTON_A));
    set_button(Btn::B, button_state(SDL_CONTROLLER_BUTTON_B));
    set_button(Btn::X, button_state(SDL_CONTROLLER_BUTTON_X));
    set_button(Btn::Y, button_state(SDL_CONTROLLER_BUTTON_Y));
    set_button(Btn::Plus, button_state(SDL_CONTROLLER_BUTTON_START));
    set_button(Btn::Minus, button_state(SDL_CONTROLLER_BUTTON_BACK));
    set_button(Btn::L, button_state(SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    set_button(Btn::R, button_state(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
    set_button(Btn::ZL, axis_state(SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    set_button(Btn::ZR, axis_state(SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    set_button(Btn::Up, button_state(SDL_CONTROLLER_BUTTON_DPAD_UP));
    set_button(Btn::Down, button_state(SDL_CONTROLLER_BUTTON_DPAD_DOWN));
    set_button(Btn::Left, button_state(SDL_CONTROLLER_BUTTON_DPAD_LEFT));
    set_button(Btn::Right, button_state(SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
    set_button(Btn::StickLPress, button_state(SDL_CONTROLLER_BUTTON_LEFTSTICK));
    set_button(Btn::StickRPress, button_state(SDL_CONTROLLER_BUTTON_RIGHTSTICK));
    set_button(Btn::Home, button_state(SDL_CONTROLLER_BUTTON_GUIDE));
#ifdef SDL_CONTROLLER_BUTTON_MISC1
    set_button(Btn::Capture, button_state(SDL_CONTROLLER_BUTTON_MISC1));
#endif

    // Stick izquierdo como cruceta adicional.
    const int stick_x = axis_with_deadzone(SDL_CONTROLLER_AXIS_LEFTX);
    const int stick_y = axis_with_deadzone(SDL_CONTROLLER_AXIS_LEFTY);
    if (stick_x != 0) {
      set_button(Btn::Right, buttons_[index_of(Btn::Right)].down || stick_x > 0);
      set_button(Btn::Left, buttons_[index_of(Btn::Left)].down || stick_x < 0);
    }
    if (stick_y != 0) {
      set_button(Btn::Down, buttons_[index_of(Btn::Down)].down || stick_y > 0);
      set_button(Btn::Up, buttons_[index_of(Btn::Up)].down || stick_y < 0);
    }

    // Stick derecho: mueve el cursor.
    const int pointer_x = axis_with_deadzone(SDL_CONTROLLER_AXIS_RIGHTX);
    const int pointer_y = axis_with_deadzone(SDL_CONTROLLER_AXIS_RIGHTY);
    const float speed = kPointerSpeed * delta * (static_cast<float>(screen_w_) / 1280.0f) * 900.0f;
    pointer_vx_ = static_cast<float>(pointer_x) / 32767.0f * speed;
    pointer_vy_ = static_cast<float>(pointer_y) / 32767.0f * speed;
    if (pointer_x != 0 || pointer_y != 0) {
      pointer_visible_ = true;
      pointer_x_ = std::clamp(pointer_x_ + static_cast<int>(pointer_vx_), 0, screen_w_ - 1);
      pointer_y_ = std::clamp(pointer_y_ + static_cast<int>(pointer_vy_), 0, screen_h_ - 1);
    }
  }
#endif  // !__SWITCH__

  update_from_hid();
}

void Input::update_from_hid() {
#if defined(__SWITCH__)
  platform::PadSnapshot pad;
  if (platform::pad_poll(&pad)) {
    constexpr int kStick = 9000;
    auto held = [&](uint64_t mask) { return (pad.down & mask) != 0; };

    set_button(Btn::A, held(HidNpadButton_A));
    set_button(Btn::B, held(HidNpadButton_B));
    set_button(Btn::X, held(HidNpadButton_X));
    set_button(Btn::Y, held(HidNpadButton_Y));
    set_button(Btn::Plus, held(HidNpadButton_Plus));
    set_button(Btn::Minus, held(HidNpadButton_Minus));
    set_button(Btn::L, held(HidNpadButton_L));
    set_button(Btn::R, held(HidNpadButton_R));
    set_button(Btn::ZL, held(HidNpadButton_ZL));
    set_button(Btn::ZR, held(HidNpadButton_ZR));
    set_button(Btn::StickLPress, held(HidNpadButton_StickL));
    set_button(Btn::StickRPress, held(HidNpadButton_StickR));
    // Home y Capture son botones de sistema: no se usan en la navegacion.

    // Cruceta y stick izquierdo comparten los botones de direccion.
    // En libnx, stick_ly es POSITIVO hacia arriba y NEGATIVO hacia abajo.
    set_button(Btn::Up, held(HidNpadButton_Up) || pad.stick_ly > kStick);
    set_button(Btn::Down, held(HidNpadButton_Down) || pad.stick_ly < -kStick);
    set_button(Btn::Left, held(HidNpadButton_Left) || pad.stick_lx < -kStick);
    set_button(Btn::Right, held(HidNpadButton_Right) || pad.stick_lx > kStick);

    // Stick derecho: mueve el cursor en pantalla (Y en pantalla crece hacia abajo).
    if (std::abs(pad.stick_rx) > kStick || std::abs(pad.stick_ry) > kStick) {
      pointer_visible_ = true;
      pointer_last_moved_ms_ = SDL_GetTicks();
      const int step_x = pad.stick_rx * 18 / 32767;
      const int step_y = pad.stick_ry * 18 / 32767;
      pointer_x_ = std::clamp(pointer_x_ + step_x, 0, screen_w_ - 1);
      pointer_y_ = std::clamp(pointer_y_ - step_y, 0, screen_h_ - 1);
    } else if (pad.down != 0) {
      // Si se pulsa cualquier boton fisico (A, B, cruceta, etc.), ocultar el cursor
      pointer_visible_ = false;
    }

    // El HID de libnx responde siempre en Switch: se marca para que la interfaz lo sepa.
    hid_active_ = true;
  }

  platform::TouchSnapshot touch;
  if (platform::touch_poll(&touch)) {
    if (touch.active) {
      // El panel tactil informa en su propio espacio (1280x720 en portatil); se pasa al
      // tamano real del renderer por si la salida no coincide (p. ej. 1080p en dock).
      constexpr int kTouchPanelWidth = 1280;
      constexpr int kTouchPanelHeight = 720;
      const int scaled_x = touch.x * screen_w_ / kTouchPanelWidth;
      const int scaled_y = touch.y * screen_h_ / kTouchPanelHeight;
      pointer_visible_ = true;
      pointer_last_moved_ms_ = SDL_GetTicks();
      pointer_x_ = std::clamp(scaled_x, 0, screen_w_ - 1);
      pointer_y_ = std::clamp(scaled_y, 0, screen_h_ - 1);
      set_button(Btn::Touch, true);
    } else {
      set_button(Btn::Touch, false);
    }
  }
#endif
}

bool Input::pressed(Btn button) const {
  const ButtonState& state = buttons_[index_of(button)];
  return state.down && !state.down_last;
}

bool Input::released(Btn button) const {
  const ButtonState& state = buttons_[index_of(button)];
  return !state.down && state.down_last;
}

bool Input::held(Btn button) const { return buttons_[index_of(button)].down; }

bool Input::repeat(Btn button, uint32_t first_delay_ms, uint32_t repeat_ms) {
  ButtonState& state = buttons_[index_of(button)];
  if (!state.down) return false;
  // En el fotograma exacto en que se presiona (tap rapido), responde de inmediato.
  if (!state.down_last) {
    return true;
  }
  const uint32_t now = SDL_GetTicks();
  if (state.down_since == 0) state.down_since = now;
  const uint32_t elapsed = now - state.down_since;
  if (elapsed < first_delay_ms) return false;
  if (state.last_repeat == 0) {
    state.last_repeat = now;
    return true;
  }
  if (now - state.last_repeat >= repeat_ms) {
    state.last_repeat = now;
    return true;
  }
  return false;
}

int Input::nav_x() {
  int direction = 0;
  if (repeat(Btn::Left)) direction -= 1;
  if (repeat(Btn::Right)) direction += 1;
  return direction;
}

int Input::nav_y() {
  int direction = 0;
  if (repeat(Btn::Up)) direction -= 1;
  if (repeat(Btn::Down)) direction += 1;
  return direction;
}

const char* button_glyph(Btn button) {
  switch (button) {
    case Btn::A: return "A";
    case Btn::B: return "B";
    case Btn::X: return "X";
    case Btn::Y: return "Y";
    case Btn::Plus: return "+";
    case Btn::Minus: return "-";
    case Btn::L: return "L";
    case Btn::R: return "R";
    case Btn::ZL: return "ZL";
    case Btn::ZR: return "ZR";
    case Btn::Up: return "Arriba";
    case Btn::Down: return "Abajo";
    case Btn::Left: return "Izquierda";
    case Btn::Right: return "Derecha";
    default: return "";
  }
}

std::string Input::hint_text() const {
  if (hid_active_) return "Mando Switch (HID)";
  return controller_ ? "Mando conectado" : "Sin mando";
}

}  // namespace delanx::ui
