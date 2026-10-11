// Delanx — capa fina sobre jansson (JSON) sin excepciones.
#pragma once

#include <jansson.h>

#include <optional>
#include <string>
#include <vector>

namespace delanx::json {

// Vista no propietaria sobre un nodo jansson. Solo es valida mientras viva el Doc que la creo.
class Value {
 public:
  Value() = default;
  explicit Value(json_t* v) : v_(v) {}

  bool valid() const { return v_ != nullptr; }
  json_t* raw() const { return v_; }
  bool is_object() const { return json_is_object(v_); }
  bool is_array() const { return json_is_array(v_); }
  bool is_string() const { return json_is_string(v_); }
  bool is_number() const { return json_is_number(v_); }
  bool is_bool() const { return json_is_boolean(v_); }
  bool is_null() const { return json_is_null(v_); }

  Value get(const char* key) const { return Value(json_object_get(v_, key)); }
  Value at(size_t index) const { return Value(json_array_get(v_, index)); }
  size_t size() const {
    if (json_is_array(v_)) return json_array_size(v_);
    if (json_is_object(v_)) return json_object_size(v_);
    return 0;
  }

  std::string as_string(const std::string& def = std::string()) const {
    return json_is_string(v_) ? std::string(json_string_value(v_)) : def;
  }
  int64_t as_integer(int64_t def = 0) const {
    if (json_is_integer(v_)) return json_integer_value(v_);
    if (json_is_number(v_)) return static_cast<int64_t>(json_number_value(v_));
    return def;
  }
  double as_number(double def = 0.0) const {
    return json_is_number(v_) ? json_number_value(v_) : def;
  }
  bool as_boolean(bool def = false) const {
    return json_is_boolean(v_) ? json_boolean_value(v_) != 0 : def;
  }

  // Accesos por clave con valor por defecto; toleran nodos ausentes o de otro tipo
  // (asi el parser acepta ficheros escritos por otras versiones o por Quiver).
  std::string string(const char* key, const std::string& def = std::string()) const;
  bool boolean(const char* key, bool def = false) const;
  int64_t integer(const char* key, int64_t def = 0) const;
  double number(const char* key, double def = 0.0) const;
  std::vector<std::string> string_array(const char* key) const;

 private:
  json_t* v_ = nullptr;
};

// Documento propietario. Mueve-only.
class Doc {
 public:
  Doc() = default;
  ~Doc();
  Doc(Doc&& other) noexcept;
  Doc& operator=(Doc&& other) noexcept;
  Doc(const Doc&) = delete;
  Doc& operator=(const Doc&) = delete;

  static std::optional<Doc> parse(const std::string& text);
  static std::optional<Doc> parse_file(const std::string& path);

  Value root() const { return Value(root_); }
  std::string dump(bool pretty = true) const;
  bool save_file(const std::string& path) const;

 private:
  void reset(json_t* root);
  json_t* root_ = nullptr;
};

// Constructor de objetos/arrays propietarios, para serializar sin fugas.
class Builder {
 public:
  Builder() = default;
  static Builder object();
  static Builder array();
  ~Builder();
  Builder(Builder&& other) noexcept;
  Builder& operator=(Builder&& other) noexcept;
  Builder(const Builder&) = delete;
  Builder& operator=(const Builder&) = delete;

  Builder& set(const char* key, const std::string& value);
  Builder& set(const char* key, bool value);
  Builder& set(const char* key, int64_t value);
  Builder& set(const char* key, double value);
  Builder& set_json(const char* key, Builder&& child);
  Builder& set_null(const char* key);
  Builder& set_strings(const char* key, const std::vector<std::string>& values);
  Builder& append(const std::string& value);
  Builder& append_json(Builder&& child);

  bool valid() const { return v_ != nullptr; }
  Value value() const { return Value(v_); }
  // Entrega la referencia (el llamante pasa a ser dueno) y deja el builder vacio.
  json_t* release();

 private:
  json_t* v_ = nullptr;
};

}  // namespace delanx::json
