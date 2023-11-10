/*
 * lcd_nav.ino - Example code using the menu system library
 *
 * This example shows using the menu system with a 16x2 LCD display
 * (controled over serial).
 *
 * Copyright (c) 2015 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#include <MenuSystem.h>
#include <LiquidCrystal.h>

// renderer

// The LCD circuit:
//    * LCD RS pin to digital pin 8
//    * LCD Enable pin to digital pin 9
//    * LCD D4 pin to digital pin 4
//    * LCD D5 pin to digital pin 5
//    * LCD D6 pin to digital pin 6
//    * LCD D7 pin to digital pin 7
//    * LCD R/W pin to ground
LiquidCrystal lcd = LiquidCrystal(8, 9, 4, 5, 6, 7);

class MyRenderer : public MenuComponentRenderer
{
public:
    virtual void render(Menu const& menu) const
    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(menu.get_name());
        lcd.setCursor(0,1);
        menu.get_current_component()->render(*this);
    }

    virtual void render_menu_item(MenuItem const& menu_item) const
    {
        lcd.print(menu_item.get_name());
    }

    virtual void render_back_menu_item(BackMenuItem const& menu_item) const
    {
        lcd.print(menu_item.get_name());
    }

    virtual void render_numeric_menu_item(NumericMenuItem const& menu_item) const
    {
        lcd.print(menu_item.get_name());
    }

    virtual void render_menu(Menu const& menu) const
    {
        lcd.print(menu.get_name());
    }
};
MyRenderer my_renderer;

// Forward declarations

void on_item1_selected(MenuItem* p_menu_item);
void on_item2_selected(MenuItem* p_menu_item);
void on_item3_selected(MenuItem* p_menu_item);

// Menu variables

MenuSystem ms(my_renderer);
MenuItem mm_mi1("Level 1 - Item 1 (Item)", &on_item1_selected);
MenuItem mm_mi2("Level 1 - Item 2 (Item)", &on_item2_selected);
Menu mu1("Level 1 - Item 3 (Menu)");
MenuItem mu1_mi1("Level 2 - Item 1 (Item)", on_item3_selected);

// Menu callback function

void on_item1_selected(MenuItem* p_menu_item)
{
  lcd.setCursor(0,1);
  lcd.print("Item1 Selected  ");
  delay(1500); // so we can look the result on the LCD
}

void on_item2_selected(MenuItem* p_menu_item)
{
  lcd.setCursor(0,1);
  lcd.print("Item2 Selected  ");
  delay(1500); // so we can look the result on the LCD
}

void on_item3_selected(MenuItem* p_menu_item)
{
  lcd.setCursor(0,1);
  lcd.print("Item3 Selected  ");
  delay(1500); // so we can look the result on the LCD
}

void serialPrintHelp() {
  Serial.println("***************");
  Serial.println("w: go to previus item (up)");
  Serial.println("s: go to next item (down)");
  Serial.println("a: go back (right)");
  Serial.println("d: select \"selected\" item");
  Serial.println("?: print this help");
  Serial.println("h: print this help");
  Serial.println("***************");
}

void serialHandler() {
  char inChar;
  if((inChar = Serial.read())>0) {
    switch (inChar) {
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
    case '?':
    case 'h': // Display help
      serialPrintHelp();
      break;
    default:
      break;
    }
  }
}

// Standard arduino functions

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);

  serialPrintHelp();

  ms.get_root_menu().add_item(&mm_mi1);
  ms.get_root_menu().add_item(&mm_mi2);
  ms.get_root_menu().add_menu(&mu1);
  mu1.add_item(&mu1_mi1);

  ms.display();
}

void loop()
{
  serialHandler();
}
