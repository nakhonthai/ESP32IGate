/*
 * current_item.ino - Example code using the menu system library.
 *
 * This example demonstrates using the library with a Sure Electronics 32x16
 * LED matrix.
 *
 * Copyright (c) 2015 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#include <ht1632c.h>
#include <MenuSystem.h>

// Display constants

#define PIN_LED_DATA  0
#define PIN_LED_WR    3
#define PIN_LED_CLOCK 5
#define PIN_LED_CS    4

// Renderer

ht1632c ledMatrix = ht1632c(&PORTB, PIN_LED_DATA, PIN_LED_WR, PIN_LED_CLOCK, PIN_LED_CS, GEOM_32x16, 2);

class MyRenderer : public MenuComponentRenderer
{
public:
    MyRenderer()
    : _led_height(16),
      _led_width(32),
      _font_width(5),
      _font_height(7),
      _color(RED)
    {
    }

    virtual void render(Menu const& menu) const
    {
        ledMatrix.clear();
        MenuComponent const* cp_m_comp = menu.get_current_component();
        cp_m_comp->render(*this);
    }

    virtual void render_menu_item(MenuItem const& menu_item) const
    {
        char const* name = menu_item.get_name();
        _render_text_center(name);
    }

    virtual void render_back_menu_item(BackMenuItem const& menu_item) const
    {
        char const* name = menu_item.get_name();
        _render_text_center(name);
    }

    virtual void render_numeric_menu_item(NumericMenuItem const& menu_item) const
    {
        char const* name = menu_item.get_name();
        _render_text_center(name);
    }

    virtual void render_menu(Menu const& menu) const
    {
        char const* name = menu.get_name();
        _render_text_center(name);
    }

private:
    void _render_text_center(char const* name) const
    {
        uint8_t x_idnt = _get_x_indent(name);
        uint8_t y_idnt = _get_y_indent();

        for (size_t i = 0; i < strlen(name); i++)
            ledMatrix.putchar((i * _font_width) + x_idnt, y_idnt, name[i], _color);
        ledMatrix.sendframe();
    }

    uint8_t _get_x_indent(char const* name) const
    {
        uint8_t text_width = _font_width * strlen(name);
        uint8_t pixel_spare = _led_width - text_width;
        return (uint8_t) floor(pixel_spare / 2);
    }

    uint8_t _get_y_indent() const
    {
        return (_led_width/ 2) - (_led_height / 2);
    }

private:
    const uint8_t _led_height;
    const uint8_t _led_width;
    const uint8_t _font_width;
    const uint8_t _font_height;
    const uint8_t _color;
};
MyRenderer my_renderer;

// Menu callback functions

void on_time_selected(MenuItem* p_menu_item)
{
    Serial.println("Time selected");
}

void on_date_selected(MenuItem* p_menu_item)
{
    Serial.println("Date selected");
}

void on_brightness_selected(MenuItem* p_menu_item)
{
    Serial.println("Brightness selected");
}

void on_color_selected(MenuItem* p_menu_item)
{
    Serial.println("Color selected");
}

// Menu variables

MenuSystem ms(my_renderer);
MenuItem mi_time("TIME", &on_time_selected);
MenuItem mi_date("DATE", &on_date_selected);
Menu mu_disp("DISP*");
MenuItem mi_brightness("BRTNS", &on_brightness_selected);
MenuItem mi_color("COLOR", &on_color_selected);

// Functions

void serial_handler()
{
    char inChar;
    if ((inChar = Serial.read()) > 0)
    {
        switch (inChar)
        {
            case 'w': // Previus item
                ms.prev();
                ms.display();
                break;
            case 's': // Next item
                ms.next();
                ms.display();
                break;
            case 'a': // Back presed
                ms.back();
                ms.display();
                break;
            case 'd': // Select presed
                ms.select();
                ms.display();
                break;
            default:
                break;
        }
    }
}

// Standard arduino functions

void setup() {
    Serial.begin(9600);

    ledMatrix.clear();
    ledMatrix.setfont(FONT_5x7);

    ms.get_root_menu().add_item(&mi_time);
    ms.get_root_menu().add_item(&mi_date);
    ms.get_root_menu().add_menu(&mu_disp);
    mu_disp.add_item(&mi_brightness);
    mu_disp.add_item(&mi_color);
    ms.display();
}

void loop() {
    serial_handler();
}
