#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include <codecvt>

#include <map>
#include "ST7735.hpp"
#include <iostream>
#pragma once

class textLCD
{
private:
    FT_Library _ft;
    FT_Face _face;
    std::map<wchar_t, struct charRepresentation> _characters;
    void addCharacter(wchar_t c);
    ST7735 *_lcd;

public:
    textLCD(std::string font_path, int pixel_size, ST7735 *lcd);
    ~textLCD();
    void drawText(std::wstring text, int x, int y, uint32_t color);
};

struct charRepresentation
{
    unsigned int width;
    unsigned int height;
    unsigned char *bitmap;
    FT_BBox bbox;
    unsigned int advance_x;
    unsigned int advance_y;

    unsigned int pitch;
};
