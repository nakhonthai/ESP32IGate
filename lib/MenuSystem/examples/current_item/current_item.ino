/*
 * current_item.ino - Example code using the menu system library.
 *
 * This example shows only the current menu item.
 *
 * Copyright (c) 2015 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#include <MenuSystem.h>

// renderer

class MyRenderer : public MenuComponentRenderer
{
public:
    virtual void render(Menu const& menu) const
    {
        menu.get_current_component()->render(*this);
    }

    virtual void render_menu_item(MenuItem const& menu_item) const
    {
        Serial.println(menu_item.get_name());
    }

    virtual void render_back_menu_item(BackMenuItem const& menu_item) const
    {
        Serial.println(menu_item.get_name());
    }

    virtual void render_numeric_menu_item(NumericMenuItem const& menu_item) const
    {
        Serial.println(menu_item.get_name());
    }

    virtual void render_menu(Menu const& menu) const
    {
        Serial.println(menu.get_name());
    }
};
MyRenderer my_renderer;

// forward declarations

void on_item1_selected(MenuItem* p_menu_item);
void on_item2_selected(MenuItem* p_menu_item);
void on_item3_selected(MenuItem* p_menu_item);
void on_item4_selected(MenuItem* p_menu_item);

// Menu variables

MenuSystem ms(my_renderer);

MenuItem mm_mi1("Level 1 - Item 1 (Item)", &on_item1_selected);
MenuItem mm_mi2("Level 1 - Item 2 (Item)", &on_item2_selected);
Menu mu1("Level 1 - Item 3 (Menu)");
MenuItem mu1_mi1("Level 2 - Item 1 (Item)", &on_item3_selected);
MenuItem mu1_mi2("Level 2 - Item 2 (Item)", &on_item4_selected);

// Menu callback function

bool bRanCallback = false;

void on_item1_selected(MenuItem* p_menu_item)
{
  Serial.println("Item1 Selected");
  bRanCallback = true;
}

void on_item2_selected(MenuItem* p_menu_item)
{
  Serial.println("Item2 Selected");
  bRanCallback = true;
}

void on_item3_selected(MenuItem* p_menu_item)
{
  Serial.println("Item3 Selected");
  bRanCallback = true;
}

void on_item4_selected(MenuItem* p_menu_item)
{
  Serial.println("Item4 Selected");
  bRanCallback = false;
  ms.reset();
}

// Standard arduino functions

void setup()
{
  Serial.begin(9600);

  ms.get_root_menu().add_item(&mm_mi1);
  ms.get_root_menu().add_item(&mm_mi2);
  ms.get_root_menu().add_menu(&mu1);
  mu1.add_item(&mu1_mi1);
  mu1.add_item(&mu1_mi2);
}

void loop()
{
  ms.display();
  ms.select();
  if (bRanCallback)
  {
      ms.next();
      bRanCallback = false;
  }
  delay(2000);
}
