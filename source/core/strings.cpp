#include "core/strings.hpp"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "core/time.hpp"

namespace delanx::str {
namespace {

int lower_char(int c) { return std::tolower(static_cast<unsigned char>(c)); }

bool glob_match_impl(const char* p, const char* t) {
  while (*p) {
    if (*p == '*') {
      while (*(p + 1) == '*') ++p;
      if (!*(p + 1)) return true;
      for (const char* probe = t; *probe; ++probe) {
        if (glob_match_impl(p + 1, probe)) return true;
      }
      return glob_match_impl(p + 1, t + std::strlen(t));
    }
    if (!*t) return false;
    if (*p != '?' && lower_char(*p) != lower_char(*t)) return false;
    ++p;
    ++t;
  }
  return *t == '\0';
}

}  // namespace

std::string trim(const std::string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

std::string lower(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

std::string upper(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return out;
}

bool iequals(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (lower_char(a[i]) != lower_char(b[i])) return false;
  }
  return true;
}

bool icontains(const std::string& haystack, const std::string& needle) {
  if (needle.empty()) return true;
  if (needle.size() > haystack.size()) return false;
  const std::string h = lower(haystack);
  const std::string n = lower(needle);
  return h.find(n) != std::string::npos;
}

bool starts_with(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool istarts_with(const std::string& s, const std::string& prefix) {
  if (s.size() < prefix.size()) return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (lower_char(s[i]) != lower_char(prefix[i])) return false;
  }
  return true;
}

bool ends_with(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool iends_with(const std::string& s, const std::string& suffix) {
  if (s.size() < suffix.size()) return false;
  return iequals(s.substr(s.size() - suffix.size()), suffix);
}

bool glob_match(const std::string& pattern, const std::string& text) {
  return glob_match_impl(pattern.c_str(), text.c_str());
}

bool asset_matches_filter(const std::string& filter, const std::string& asset_name) {
  const std::string f = trim(filter);
  if (f.empty()) return true;
  if (f.find('*') != std::string::npos || f.find('?') != std::string::npos) {
    return glob_match(f, asset_name);
  }
  return icontains(asset_name, f);
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::string current;
  for (char c : s) {
    if (c == sep) {
      out.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  out.push_back(current);
  return out;
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) out += sep;
    out += parts[i];
  }
  return out;
}

std::string replace_all(std::string s, const std::string& from, const std::string& to) {
  if (from.empty()) return s;
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
  return s;
}

std::string format(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  va_list copy;
  va_copy(copy, args);
  const int needed = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);
  std::string out;
  if (needed > 0) {
    out.resize(static_cast<size_t>(needed));
    std::vsnprintf(&out[0], static_cast<size_t>(needed) + 1, fmt, args);
  }
  va_end(args);
  return out;
}

std::string url_encode(const std::string& s) {
  static const char* hex = "0123456789ABCDEF";
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('%');
      out.push_back(hex[c >> 4]);
      out.push_back(hex[c & 0x0F]);
    }
  }
  return out;
}

std::string human_size(uint64_t bytes) {
  const char* units[] = {"B", "KB", "MB", "GB"};
  double value = static_cast<double>(bytes);
  int unit = 0;
  while (value >= 1024.0 && unit < 3) {
    value /= 1024.0;
    ++unit;
  }
  if (unit == 0) return format("%llu B", static_cast<unsigned long long>(bytes));
  return format("%.1f %s", value, units[unit]);
}

std::vector<std::string> normalize_tags(const std::vector<std::string>& tags) {
  std::vector<std::string> out;
  for (const auto& raw : tags) {
    std::string tag = lower(trim(raw));
    if (tag.empty()) continue;
    if (std::find(out.begin(), out.end(), tag) == out.end()) out.push_back(tag);
  }
  return out;
}

std::vector<std::string> wrap_text(const std::string& text, size_t max_chars) {
  std::vector<std::string> lines;
  if (max_chars == 0) return lines;
  std::string line;
  size_t i = 0;
  while (i < text.size()) {
    if (text[i] == '\n') {
      lines.push_back(line);
      line.clear();
      ++i;
      continue;
    }
    size_t word_end = text.find_first_of(" \n", i);
    if (word_end == std::string::npos) word_end = text.size();
    std::string word = text.substr(i, word_end - i);
    if (!line.empty() && line.size() + 1 + word.size() > max_chars) {
      lines.push_back(line);
      line.clear();
    }
    if (word.size() > max_chars) {
      // Palabra mas larga que la linea: se corta a lo bruto.
      size_t offset = 0;
      while (offset < word.size()) {
        const size_t take = std::min(max_chars, word.size() - offset);
        lines.push_back(word.substr(offset, take));
        offset += take;
      }
    } else {
      if (!line.empty()) line.push_back(' ');
      line += word;
    }
    i = word_end;
    while (i < text.size() && text[i] == ' ') ++i;
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}

std::string relative_date(int64_t unix_seconds) {
  if (unix_seconds <= 0) return "-";
  const int64_t now = now_unix();
  int64_t delta = now - unix_seconds;
  if (delta < 0) delta = 0;
  if (delta < 3600) return format("%lld min", static_cast<long long>(delta / 60));
  if (delta < 86400) return format("%lld h", static_cast<long long>(delta / 3600));
  if (delta < 86400 * 30) return format("%lld d", static_cast<long long>(delta / 86400));
  if (delta < 86400 * 365) return format("%lld mes", static_cast<long long>(delta / (86400 * 30)));
  return format("%lld a", static_cast<long long>(delta / (86400 * 365)));
}

std::string initials(const std::string& text) {
  std::string out;
  for (char c : text) {
    if (std::isalpha(static_cast<unsigned char>(c))) {
      out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
      if (out.size() >= 2) break;
    }
  }
  if (out.empty()) out = "?";
  return out;
}

}  // namespace delanx::str
