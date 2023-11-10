#include "CustomNumericMenuItem.h"

CustomNumericMenuItem::CustomNumericMenuItem(
        uint8_t width, const char* name, float value, float minValue,
        float maxValue, float increment, FormatValueFnPtr on_format_value)
: NumericMenuItem(name, nullptr, value, minValue, maxValue, increment,
                  on_format_value),
  _width(width)
{
}

uint8_t CustomNumericMenuItem::get_width() const
{
    return _width;
}

void CustomNumericMenuItem::render(MenuComponentRenderer const& renderer) const
{
    MyRenderer const& my_renderer = static_cast<MyRenderer const&>(renderer);
    my_renderer.render_custom_numeric_menu_item(*this);
}
