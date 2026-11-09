#include "util/markup.hpp"

#include <glibmm/markup.h>
#include <pango/pango.h>

#include <regex>
#include <string>

namespace waybar::util {

bool isPangoMarkup(const std::string& text) {
  GError* error = nullptr;
  const bool valid = pango_parse_markup(text.c_str(), -1, '\0', nullptr, nullptr, nullptr, &error);
  if (error != nullptr) {
    g_error_free(error);
  }
  return valid;
}

std::string escapeBareAmpersands(const std::string& text) {
  static const std::regex bare_amp("&(?!amp;|lt;|gt;|quot;|apos;|#[0-9]+;|#[xX][0-9a-fA-F]+;)");
  return std::regex_replace(text, bare_amp, "&amp;");
}

std::string prepareWorkspaceName(const std::string& name, bool disable_markup) {
  if (disable_markup) {
    return name;
  }
  std::string repaired = escapeBareAmpersands(name);
  if (isPangoMarkup(repaired)) {
    return repaired;
  }
  // Escape the original name, not the repaired form
  return Glib::Markup::escape_text(name).raw();
}

}  // namespace waybar::util
