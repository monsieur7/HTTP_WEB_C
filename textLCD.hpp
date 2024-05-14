#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BBOX_H
#include <codecvt>
#include <algorithm>
#include <map>
#include "ST7735.hpp"
#include <iostream>
#pragma once

class textLCD
{
private:
    FT_Library _ft;
    FT_Face _face;
    ST7735 *_lcd;

public:
    textLCD(std::string font_path, int pixel_size, ST7735 *lcd);
    ~textLCD();
    void drawText(std::wstring text, int x, int y, uint16_t color, uint16_t bg_color = ST7735_BLACK);
};
