#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <pango/pango.h>

#include <string>

#include "util/markup.hpp"

using waybar::util::escapeBareAmpersands;
using waybar::util::isPangoMarkup;
using waybar::util::prepareWorkspaceName;

namespace {

// What the label displays: pango-decoded output of set_markup when markup is
// enabled, the verbatim output of set_label when disabled.
std::string displayed(const std::string& prepared, bool disable_markup) {
  if (disable_markup) {
    return prepared;
  }
  char* text = nullptr;
  GError* error = nullptr;
  const bool ok = pango_parse_markup(prepared.c_str(), (gint)prepared.size(), '\0', nullptr, &text,
                                     nullptr, &error);
  // The prepared string must always parse: this is what keeps the module from
  // emitting Gtk-WARNING on every workspace update (the "Mail & Cal" symptom).
  REQUIRE(ok);
  std::string out = ok && text != nullptr ? std::string(text) : std::string();
  g_free(text);
  if (error != nullptr) {
    g_error_free(error);
  }
  return out;
}

bool renders_bold(const std::string& prepared) {
  PangoAttrList* attrs = nullptr;
  if (!pango_parse_markup(prepared.c_str(), (gint)prepared.size(), '\0', &attrs, nullptr, nullptr,
                          nullptr)) {
    return false;
  }
  if (attrs == nullptr) {
    return false;
  }
  bool bold = false;
  PangoAttrIterator* iter = pango_attr_list_get_iterator(attrs);
  do {
    const auto* weight =
        reinterpret_cast<const PangoAttrInt*>(pango_attr_iterator_get(iter, PANGO_ATTR_WEIGHT));
    if (weight != nullptr && weight->value == PANGO_WEIGHT_BOLD) {
      bold = true;
    }
  } while (pango_attr_iterator_next(iter));
  pango_attr_iterator_destroy(iter);
  pango_attr_list_unref(attrs);
  return bold;
}

}  // namespace

TEST_CASE("ampersand repair only rewrites what pango cannot decode", "[markup]") {
  REQUIRE(escapeBareAmpersands("A & B") == "A &amp; B");
  REQUIRE(escapeBareAmpersands("a && b") == "a &amp;&amp; b");
  // Entities pango understands are preserved.
  REQUIRE(escapeBareAmpersands("&amp;&lt;&gt;&quot;&apos;") == "&amp;&lt;&gt;&quot;&apos;");
  REQUIRE(escapeBareAmpersands("&#32;x&#x1F600;") == "&#32;x&#x1F600;");
  // Entity-shaped but unknown to pango: repairing beats failing the whole parse.
  REQUIRE(escapeBareAmpersands("R &D; Cal") == "R &amp;D; Cal");
  REQUIRE(escapeBareAmpersands("a &nbsp; b") == "a &amp;nbsp; b");
}

TEST_CASE("workspace names render as markup when markup is enabled", "[markup]") {
  constexpr bool disable_markup = false;

  SECTION("raw string renders as itself") {
    const std::string prepared = prepareWorkspaceName("Name", disable_markup);
    REQUIRE(prepared == "Name");
    REQUIRE(isPangoMarkup(prepared));
    REQUIRE(displayed(prepared, disable_markup) == "Name");
    REQUIRE_FALSE(renders_bold(prepared));
  }

  SECTION("valid pango markup renders bold, unescaped") {
    const std::string prepared = prepareWorkspaceName("<b>Name</b>", disable_markup);
    REQUIRE(prepared == "<b>Name</b>");  // markup preserved, not neutralized
    REQUIRE(displayed(prepared, disable_markup) == "Name");
    REQUIRE(renders_bold(prepared));
  }

  SECTION("unescaped ampersand renders as literal text") {
    const std::string prepared = prepareWorkspaceName("Name & Name", disable_markup);
    REQUIRE(isPangoMarkup(prepared));
    REQUIRE(displayed(prepared, disable_markup) == "Name & Name");
    REQUIRE_FALSE(renders_bold(prepared));
  }

  SECTION("unescaped ampersand inside valid markup keeps the markup") {
    const std::string prepared = prepareWorkspaceName("<b>Name & Name</b>", disable_markup);
    REQUIRE(prepared == "<b>Name &amp; Name</b>");
    REQUIRE(displayed(prepared, disable_markup) == "Name & Name");
    REQUIRE(renders_bold(prepared));
  }

  SECTION("escaped entities inside markup are decoded") {
    const std::string prepared = prepareWorkspaceName("<b>A &gt; B</b>", disable_markup);
    REQUIRE(prepared == "<b>A &gt; B</b>");  // known entity preserved
    REQUIRE(displayed(prepared, disable_markup) == "A > B");
    REQUIRE(renders_bold(prepared));
  }
}

TEST_CASE("workspace names render verbatim when markup is disabled", "[markup]") {
  constexpr bool disable_markup = true;

  SECTION("raw string renders as itself") {
    const std::string prepared = prepareWorkspaceName("Name", disable_markup);
    REQUIRE(prepared == "Name");
    REQUIRE(displayed(prepared, disable_markup) == "Name");
  }

  SECTION("markup renders as its own literal text") {
    const std::string prepared = prepareWorkspaceName("<b>Name</b>", disable_markup);
    REQUIRE(prepared == "<b>Name</b>");
    REQUIRE(displayed(prepared, disable_markup) == "<b>Name</b>");
  }

  SECTION("unescaped ampersand renders as literal text") {
    const std::string prepared = prepareWorkspaceName("Name & Name", disable_markup);
    REQUIRE(prepared == "Name & Name");  // no repair on the plain-text path
    REQUIRE(displayed(prepared, disable_markup) == "Name & Name");
  }

  SECTION("markup with unescaped ampersand renders literally") {
    const std::string prepared = prepareWorkspaceName("<b>Name & Name</b>", disable_markup);
    REQUIRE(prepared == "<b>Name & Name</b>");
    REQUIRE(displayed(prepared, disable_markup) == "<b>Name & Name</b>");
  }

  SECTION("escaped entities render as their own literal text") {
    const std::string prepared = prepareWorkspaceName("<b>A &gt; B</b>", disable_markup);
    REQUIRE(prepared == "<b>A &gt; B</b>");
    REQUIRE(displayed(prepared, disable_markup) == "<b>A &gt; B</b>");
  }
}

TEST_CASE("a tag spanning the name and the format string is escaped, not rendered", "[markup]") {
  // Kept limitation: validity is decided on the name alone.
  constexpr bool disable_markup = false;
  const std::string prepared = prepareWorkspaceName("<b>Name", disable_markup);
  REQUIRE(prepared == "&lt;b&gt;Name");
  REQUIRE(isPangoMarkup(prepared));
  REQUIRE(displayed(prepared, disable_markup) == "<b>Name");
  REQUIRE_FALSE(renders_bold(prepared));
}
