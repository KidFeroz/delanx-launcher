#include "core/json.hpp"

#include <cstdio>
#include <utility>

#include "core/paths.hpp"

namespace delanx::json {

std::string Value::string(const char* key, const std::string& def) const {
  return get(key).as_string(def);
}

bool Value::boolean(const char* key, bool def) const { return get(key).as_boolean(def); }

int64_t Value::integer(const char* key, int64_t def) const { return get(key).as_integer(def); }

double Value::number(const char* key, double def) const { return get(key).as_number(def); }

std::vector<std::string> Value::string_array(const char* key) const {
  std::vector<std::string> out;
  const Value node = get(key);
  if (!node.is_array()) return out;
  const size_t count = node.size();
  out.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const Value item = node.at(i);
    if (item.is_string()) out.push_back(item.as_string());
  }
  return out;
}

Doc::~Doc() { reset(nullptr); }

Doc::Doc(Doc&& other) noexcept : root_(other.root_) { other.root_ = nullptr; }

Doc& Doc::operator=(Doc&& other) noexcept {
  if (this != &other) {
    reset(other.root_);
    other.root_ = nullptr;
  }
  return *this;
}

void Doc::reset(json_t* root) {
  if (root_) json_decref(root_);
  root_ = root;
}

std::optional<Doc> Doc::parse(const std::string& text) {
  json_error_t error;
  json_t* root = json_loadb(text.data(), text.size(), JSON_DECODE_ANY | JSON_ALLOW_NUL, &error);
  if (!root) return std::nullopt;
  Doc doc;
  doc.reset(root);
  return doc;
}

std::optional<Doc> Doc::parse_file(const std::string& path) {
  const auto text = paths::read_file(path);
  if (!text) return std::nullopt;
  return parse(*text);
}

std::string Doc::dump(bool pretty) const {
  if (!root_) return {};
  char* out = json_dumps(root_, pretty ? JSON_INDENT(2) | JSON_PRESERVE_ORDER : JSON_COMPACT);
  if (!out) return {};
  std::string result(out);
  free(out);
  return result;
}

bool Doc::save_file(const std::string& path) const {
  const std::string text = dump(true);
  if (text.empty()) return false;
  return paths::write_file_atomic(path, text + "\n");
}

Builder Builder::object() {
  Builder b;
  b.v_ = json_object();
  return b;
}

Builder Builder::array() {
  Builder b;
  b.v_ = json_array();
  return b;
}

Builder::~Builder() {
  if (v_) json_decref(v_);
}

Builder::Builder(Builder&& other) noexcept : v_(other.v_) { other.v_ = nullptr; }

Builder& Builder::operator=(Builder&& other) noexcept {
  if (this != &other) {
    if (v_) json_decref(v_);
    v_ = other.v_;
    other.v_ = nullptr;
  }
  return *this;
}

Builder& Builder::set(const char* key, const std::string& value) {
  if (v_) json_object_set_new(v_, key, json_stringn(value.c_str(), value.size()));
  return *this;
}

Builder& Builder::set(const char* key, bool value) {
  if (v_) json_object_set_new(v_, key, json_boolean(value));
  return *this;
}

Builder& Builder::set(const char* key, int64_t value) {
  if (v_) json_object_set_new(v_, key, json_integer(value));
  return *this;
}

Builder& Builder::set(const char* key, double value) {
  if (v_) json_object_set_new(v_, key, json_real(value));
  return *this;
}

Builder& Builder::set_json(const char* key, Builder&& child) {
  json_t* released = child.release();
  if (v_ && released) {
    json_object_set_new(v_, key, released);
  } else if (released) {
    json_decref(released);
  }
  return *this;
}

Builder& Builder::set_null(const char* key) {
  if (v_) json_object_set_new(v_, key, json_null());
  return *this;
}

Builder& Builder::set_strings(const char* key, const std::vector<std::string>& values) {
  if (!v_) return *this;
  json_t* array = json_array();
  for (const auto& value : values) {
    json_array_append_new(array, json_stringn(value.c_str(), value.size()));
  }
  json_object_set_new(v_, key, array);
  return *this;
}

Builder& Builder::append(const std::string& value) {
  if (v_ && json_is_array(v_)) json_array_append_new(v_, json_stringn(value.c_str(), value.size()));
  return *this;
}

Builder& Builder::append_json(Builder&& child) {
  json_t* released = child.release();
  if (v_ && json_is_array(v_) && released) {
    json_array_append_new(v_, released);
  } else if (released) {
    json_decref(released);
  }
  return *this;
}

json_t* Builder::release() {
  json_t* out = v_;
  v_ = nullptr;
  return out;
}

}  // namespace delanx::json
