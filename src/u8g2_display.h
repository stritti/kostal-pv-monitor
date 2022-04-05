#pragma once

#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <U8g2_for_Adafruit_GFX.h>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

extern const uint8_t u8g2_font_streamline_ecology_t[] U8G2_FONT_SECTION("u8g2_font_streamline_ecology_t");
extern const uint8_t u8g2_font_streamline_interface_essential_home_menu_t[] U8G2_FONT_SECTION(
    "u8g2_font_streamline_interface_essential_home_menu_t");
extern const uint8_t
    u8g2_font_streamline_interface_essential_wifi_t[] U8G2_FONT_SECTION("u8g2_font_streamline_interface_essential_wifi_t");

enum {
  GxEPD_ALIGN_RIGHT,
  GxEPD_ALIGN_LEFT,
  GxEPD_ALIGN_CENTER,
};

// e-ink display of TTGO T5
GxIO_Class            io(SPI, ELINK_SS, ELINK_DC, ELINK_RESET);  // arbitrary selection of rst=17, busy=16
GxEPD_Class           display(io, ELINK_RESET, ELINK_BUSY);      // arbitrary selection of (16), 4
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

/**
 * @brief
 *
 * @param str
 * @param y
 * @param align
 * @param offset
 */
static void displayText(const char* str, int16_t y, uint8_t align, int16_t offset = 0) {
  int16_t  x  = 0;
  int16_t  x1 = 0, y1 = 0;
  uint16_t w = 0, h = 0;
  display.setCursor(x, y);
  display.getTextBounds(str, x, y, &x1, &y1, &w, &h);
  switch (align) {
  case GxEPD_ALIGN_RIGHT:
    display.setCursor(display.width() - w - x1 - offset, y);
    break;
  case GxEPD_ALIGN_LEFT:
    display.setCursor(0 + offset, y);
    break;
  case GxEPD_ALIGN_CENTER:
    display.setCursor(display.width() / 2 - ((w + x1) / 2), y);
    break;
  default:
    break;
  }
  display.println(str);
}
