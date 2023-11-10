/*
 * led_matrix_animated.ino - Example code using the menu system library.
 *
 * This example demonstrates an animated menu on a Sure Electronics 32x16
 * LED matrix.
 *
 * Copyright (c) 2015 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#include <ht1632c.h>
#include <MenuSystem.h>

// Display constants

#define PIN_LED_DATA  8
#define PIN_LED_WR    10
#define PIN_LED_CLOCK 11
#define PIN_LED_CS    12

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

        const uint8_t prev_comp_num = menu.get_previous_component_num();
        _p_prev_comp = menu.get_menu_component(prev_comp_num);
        auto cp_m_comp = menu.get_current_component();
        cp_m_comp->render(*this);
    }

    virtual void render_menu_item(MenuItem const& menu_item) const
    {
        auto prev_name = _p_prev_comp->get_name();
        auto curr_name = menu_item.get_name();
        _fade(prev_name, curr_name);
    }

    virtual void render_back_menu_item(BackMenuItem const& menu_item) const
    {
        auto prev_name = _p_prev_comp->get_name();
        auto curr_name = menu_item.get_name();
        _fade(prev_name, curr_name);
    }

    virtual void render_numeric_menu_item(NumericMenuItem const& menu_item) const
    {
        auto prev_name = _p_prev_comp->get_name();
        auto curr_name = menu_item.get_name();
        _fade(prev_name, curr_name);
    }

    virtual void render_menu(Menu const& menu) const
    {
        auto prev_name = _p_prev_comp->get_name();
        auto curr_name = menu.get_name();
        _fade(prev_name, curr_name);
    }

private:
    enum VSlideDirection { VSLIDE_UP, VSLIDE_DOWN };

    void _vslide(char const* menu1, char const* menu2, VSlideDirection d) const
    {
        // Calculate vertical position
        int menu1_start_y = (_led_height / 2) - (_font_height / 2);
        int menu2_target_y = menu1_start_y;
        int menu2_start_y;
        if (d == VSLIDE_UP) {
            menu2_start_y = menu1_start_y + _led_height;
        } else {
            menu2_start_y = menu1_start_y - _led_height;
        }
        int menu1_y = menu1_start_y;
        int menu2_y = menu2_start_y;

        // Calculate horizontal position
        int menu1_text_width = _font_width * strlen(menu1);
        int menu1_pixel_spare = _led_width - menu1_text_width;
        int menu1_x_idnt = (int) floor(menu1_pixel_spare / 2);

        int menu2_text_width = _font_width * strlen(menu2);
        int menu2_pixel_spare = _led_width - menu2_text_width;
        int menu2_x_idnt = (int) floor(menu2_pixel_spare / 2);

        while (1) {
            ledMatrix.clear();
            for (size_t i = 0; i < strlen(menu1); i++) {
                ledMatrix.putchar(
                    (i * _font_width) + menu1_x_idnt,
                    menu1_y, menu1[i], _color
                );
            }
            for (size_t i = 0; i < strlen(menu2); i++) {
                ledMatrix.putchar(
                    (i * _font_width) + menu2_x_idnt,
                    menu2_y, menu2[i], _color
                );
            }
            ledMatrix.sendframe();

            if (menu2_y == menu2_target_y) {
                break;
            }

            if (d == VSLIDE_UP) {
                menu1_y -= 1;
                menu2_y -= 1;
            } else {
                menu1_y += 1;
                menu2_y += 1;
            }

            delay(5);
        }
    }

    enum HSlideDirection { HSLIDE_LEFT, HSLIDE_RIGHT };

    void _hslide(char const* menu1, char const* menu2, HSlideDirection d) const
    {
        // Calculate vertical position
        int y_idnt = (_led_height / 2) - (_font_height / 2);

        // Calculate horizontal position
        int menu1_text_width = _font_width * strlen(menu1);
        int menu1_pixel_spare = _led_width - menu1_text_width;
        int menu1_start_x = (int) floor(menu1_pixel_spare / 2);
        int menu2_text_width = _font_width * strlen(menu2);
        int menu2_pixel_spare = _led_width - menu2_text_width;
        int menu2_target_x = (int) floor(menu2_pixel_spare / 2);
        int menu2_start_x;
        if (d == HSLIDE_LEFT) {
            menu2_start_x = menu1_start_x + _led_width;
        } else {
            menu2_start_x = menu1_start_x - _led_width;
        }
        int menu1_x = menu1_start_x;
        int menu2_x = menu2_start_x;

        while (1) {
            ledMatrix.clear();
            for (size_t i = 0; i < strlen(menu1); i++) {
                ledMatrix.putchar(
                    (i * _font_width) + menu1_x,
                    y_idnt, menu1[i], _color
                );
            }
            for (size_t i = 0; i < strlen(menu2); i++) {
                ledMatrix.putchar(
                    (i * _font_width) + menu2_x,
                    y_idnt, menu2[i], _color
                );
            }
            ledMatrix.sendframe();

            if (menu2_x == menu2_target_x) {
                break;
            }

            if (d == HSLIDE_LEFT) {
                menu1_x -= 1;
                menu2_x -= 1;
            } else {
                menu1_x += 1;
                menu2_x += 1;
            }

            delay(5);
        }
    }

    void _fade(char const* menu1, char const* menu2) const
    {
        int y_idnt = (_led_height / 2) - (_font_height / 2);

        int menu1_text_width = _font_width * strlen(menu1);
        int menu1_pixel_spare = _led_width - menu1_text_width;
        int menu1_x_idnt = (int) floor(menu1_pixel_spare / 2);
        for (size_t i = 0; i < strlen(menu1); i++) {
            ledMatrix.putchar(
                (i * _font_width) + menu1_x_idnt,
                y_idnt, menu1[i], _color
            );
        }
        ledMatrix.sendframe();

        int brightness = 10;
        while (brightness > 0) {
            ledMatrix.pwm(--brightness);
            delay(30);
        }
        ledMatrix.clear();

        int menu2_text_width = _font_width * strlen(menu2);
        int menu2_pixel_spare = _led_width - menu2_text_width;
        int menu2_x_idnt = (int) floor(menu2_pixel_spare / 2);
        for (size_t i = 0; i < strlen(menu2); i++) {
            ledMatrix.putchar(
                (i * _font_width) + menu2_x_idnt,
                y_idnt, menu2[i], _color
            );
        }
        ledMatrix.sendframe();
        delay(30);
        while (brightness < 10) {
            ledMatrix.pwm(++brightness);
            delay(30);
        }
    }

private:
    const uint8_t _led_height;
    const uint8_t _led_width;
    const uint8_t _font_width;
    const uint8_t _font_height;
    const uint8_t _color;

    mutable MenuComponent const* _p_prev_comp;
};
MyRenderer my_renderer;

// Menu variables

MenuSystem ms(my_renderer);
MenuItem mi_one("ONE", nullptr);
MenuItem mi_two("TWO", nullptr);
MenuItem mi_three("THREE", nullptr);

// Standard arduino functions

void setup() {
    Serial.begin(9600);

    ledMatrix.clear();
    ledMatrix.pwm(10);
    ledMatrix.setfont(FONT_5x7);

    ms.get_root_menu().add_item(&mi_one);
    ms.get_root_menu().add_item(&mi_two);
    ms.get_root_menu().add_item(&mi_three);
}

void loop() {
    ms.next(true);
    ms.display();
    delay(1000);
}
