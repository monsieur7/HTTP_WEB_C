#include "textLCD.hpp"

textLCD::textLCD(std::string font_path, int pixel_size, ST7735 *lcd) : _lcd(lcd)
{
    if (FT_Init_FreeType(&_ft))
    {
        throw std::runtime_error("Error initializing freetype");
    }
    if (FT_New_Face(_ft, font_path.c_str(), 0, &_face))
    {
        throw std::runtime_error("Error loading font");
    }
    FT_Set_Pixel_Sizes(_face, 0, pixel_size);
    for (wchar_t c = 0; c < 128; c++)
    {
        if (FT_Load_Char(_face, c, FT_LOAD_RENDER))
        {
            throw std::runtime_error("Error loading character");
        }
        FT_GlyphSlot g = _face->glyph;
        charRepresentation cr;
        cr.width = g->bitmap.width;
        cr.height = g->bitmap.rows;
        cr.bearing_x = g->bitmap_left;
        cr.bearing_y = g->bitmap_top;
        cr.advance = g->advance.x >> 6;
        cr.bitmap = new unsigned char[cr.width * cr.height];
        for (unsigned int i = 0; i < cr.width * cr.height; i++)
        {
            cr.bitmap[i] = g->bitmap.buffer[i];
        }
        _characters[c] = cr;
    }
}

textLCD::~textLCD()
{
    FT_Done_Face(_face);
    FT_Done_FreeType(_ft);
    for (auto &c : _characters)
    {
        delete[] c.second.bitmap;
    }
}

void textLCD::addCharacter(wchar_t c)
{
    if (_characters.find(c) == _characters.end())
    {
        if (FT_Load_Char(_face, c, FT_LOAD_RENDER))
        {
            throw std::runtime_error("Error loading character");
        }
        FT_GlyphSlot g = _face->glyph;
        charRepresentation cr;
        cr.width = g->bitmap.width;
        cr.height = g->bitmap.rows;
        cr.bearing_x = g->bitmap_left;
        cr.bearing_y = g->bitmap_top;
        cr.advance_x = g->advance.x >> 6; // because it is in 1/64th of a pixel
        cr.advance_y = g->advance.y >> 6;
        cr.bitmap = new unsigned char[cr.width * cr.height];
        for (unsigned int i = 0; i < cr.width * cr.height; i++)
        {
            cr.bitmap[i] = g->bitmap.buffer[i];
        }
        _characters[c] = cr;
    }
}

void textLCD::drawText(std::wstring text, int x, int y, uint32_t color)
{
    for (auto c : text)
    {
        addCharacter(c);
    }
    int offset_x = 0, offset_y = 0;
    int current_x = x, current_y = y;
    for (auto c : text)
    {
        if (_characters.find(c) == _characters.end())
        {
            addCharacter(c);
        }
        charRepresentation cr = _characters[c];
        for (unsigned int i = 0; i < cr.width; i++)
        {
            for (unsigned int j = 0; j < cr.height; j++)
            {
                if (cr.bitmap[j * cr.width + i] > 0)
                {
                    current_x = x + offset_x + i + cr.bearing_x;
                    current_y = y + offset_y + j - (cr.height - cr.bearing_y); // see https://learnopengl.com/In-Practice/Text-Rendering
                    uint32_t color_alpha = _lcd->alpha_blending(color, cr.bitmap[j * cr.width + i]);
                    _lcd->drawPixel(current_x, current_y, _lcd->color565(color_alpha));
                }
            }
        }
        offset_x += (cr.advance_x);
        offset_y += (cr.advance_y);
    }
}