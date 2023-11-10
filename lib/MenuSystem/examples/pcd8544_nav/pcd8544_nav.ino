/*
 * pcd8544_nav.ino - Example code using the menu system library
 *
 * This example shows using the menu system with a pcd8544 LCD display
 * (controlled over serial).
 *
 * Copyright (c) 2016 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#include <MenuSystem.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define PCD8544_CHAR_HEIGHT 8

// LCD

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
//Adafruit_PCD8544 lcd = Adafruit_PCD8544(7, 6, 5, 4, 3);

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 lcd = Adafruit_PCD8544(5, 4, 3);
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!

// Renderer

class MyRenderer : public MenuComponentRenderer
{
public:
    virtual void render(Menu const& menu) const
    {
        lcd.clearDisplay();
        menu.render(*this);
        menu.get_current_component()->render(*this);
        lcd.display();
    }

    virtual void render_menu_item(MenuItem const& menu_item) const
    {
        lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
        lcd.print(menu_item.get_name());
    }

    virtual void render_back_menu_item(BackMenuItem const& menu_item) const
    {
        lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
        lcd.print(menu_item.get_name());
    }

    virtual void render_numeric_menu_item(NumericMenuItem const& menu_item) const
    {
        lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
        lcd.print(menu_item.get_name());
    }

    virtual void render_menu(Menu const& menu) const
    {
        lcd.setCursor(0, 0 * PCD8544_CHAR_HEIGHT);
        lcd.print(menu.get_name());
    }
};
MyRenderer my_renderer;

// Menu callback function

void on_item1_selected(MenuItem* p_menu_item)
{
  lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
  lcd.print("Item1 Selectd");
  lcd.display();
  delay(1500); // so we can look the result on the LCD
}

void on_item2_selected(MenuItem* p_menu_item)
{
  lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
  lcd.print("Item2 Selectd");
  lcd.display();
  delay(1500); // so we can look the result on the LCD
}

void on_item3_selected(MenuItem* p_menu_item)
{
  lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
  lcd.print("Item3 Selectd");
  lcd.display();
  delay(1500); // so we can look the result on the LCD
}


// Menu variables

MenuSystem ms(my_renderer);
MenuItem mm_mi1("Lvl1-Item1(I)", &on_item1_selected);
MenuItem mm_mi2("Lvl1-Item2(I)", &on_item2_selected);
Menu mu1("Lvl1-Item3(M)");
MenuItem mu1_mi1("Lvl2-Item1(I)", &on_item3_selected);

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
    case 'a': // Back pressed
      ms.back();
      ms.display();
      break;
    case 'd': // Select pressed
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

  lcd.begin();
  lcd.setContrast(60);

#ifdef HIDE_SPLASH
  lcd.clearDisplay();
#else
  lcd.display();
  delay(2000);
#endif

  lcd.setTextSize(1);

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
