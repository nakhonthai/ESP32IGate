/*
 * An example of a custom NumericMenuItem.
 * It tries to display some ASCII graphics in edit mode.
 * This can be useful if you want to give the end user an overview of the value limits.
 * 
 * Copyright (c) 2016 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#ifndef _CUSTOMNUMERICMENUITEM_H
#define _CUSTOMNUMERICMENUITEM_H

#include <MenuSystem.h>
#include "MyRenderer.h"

class MyRenderer;

class CustomNumericMenuItem : public NumericMenuItem
{
public:
    /**
     * @param width the width of the edit mode 'ASCII graphics', must be > 1
     * @param name The name of the menu item.
     * @param value Default value.
     * @param minValue The minimum value.
     * @param maxValue The maximum value.
     * @param increment How much the value should be incremented by.
     * @param valueFormatter The custom formatter. If nullptr the String float
     *                       formatter will be used.
     */
    CustomNumericMenuItem(uint8_t width, const char* name, float value,
                          float minValue, float maxValue, float increment=1.0,
                          FormatValueFnPtr on_format_value=nullptr);

    uint8_t get_width() const;

    virtual void render(MenuComponentRenderer const& renderer) const;

private:
    const uint8_t _width;
};

#endif // _CUSTOMNUMERICMENUITEM_H
