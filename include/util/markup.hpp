#pragma once
#include <string>

namespace waybar::util {

// True when the string parses as valid pango markup (tags balanced, entities
// terminated, and all named entities known to pango).
bool isPangoMarkup(const std::string& text);

// Escape ampersands that are not part of an entity pango understands
// (&amp; &lt; &gt; &quot; &apos; and the numeric forms).
std::string escapeBareAmpersands(const std::string& text);

// Workspace name prepared for display: markup when it parses, escaped when it
// does not, verbatim when markup is disabled. Validity is decided on the name
// alone; a tag meant to be closed by the format string gets escaped.
std::string prepareWorkspaceName(const std::string& name, bool disable_markup);

}  // namespace waybar::util
