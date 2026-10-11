#include "services/launch_args.hpp"

namespace delanx {

std::string quote_launch_arg(const std::string& arg) {
  bool needs_quotes = false;
  for (char ch : arg) {
    if (ch == ' ' || ch == '\t' || ch == '"' || ch == '\\') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) return arg;

  std::string out;
  out.reserve(arg.size() + 4);
  out.push_back('"');
  for (char ch : arg) {
    if (ch == '"' || ch == '\\') out.push_back('\\');
    out.push_back(ch);
  }
  out.push_back('"');
  return out;
}

std::string build_launch_argv(const std::string& nro_path, const std::vector<std::string>& extra_args,
                              bool* truncated) {
  if (truncated != nullptr) *truncated = false;

  std::string out = quote_launch_arg(nro_path);
  for (const std::string& arg : extra_args) {
    const std::string quoted = quote_launch_arg(arg);
    if (out.size() + 1 + quoted.size() > kNextLoadArgvMaxBytes) {
      if (truncated != nullptr) *truncated = true;
      break;
    }
    out.push_back(' ');
    out += quoted;
  }
  // Si ni la propia ruta cabe, se recorta: el llamante lo detecta por el tamano.
  if (out.size() > kNextLoadArgvMaxBytes) {
    out.resize(kNextLoadArgvMaxBytes);
    if (truncated != nullptr) *truncated = true;
  }
  return out;
}

}  // namespace delanx
