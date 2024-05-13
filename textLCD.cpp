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
        addCharacter(c);
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
        FT_Outline_Get_BBox(&g->outline, &cr.bbox); // Get bounding box
        // metrics :
        cr.advance_x = g->advance.x >> 6;
        cr.advance_y = g->advance.y >> 6;
        cr.bitmap = new unsigned char[cr.width * cr.height];
        cr.pitch = g->bitmap.pitch;
        cr.g = g;
        for (unsigned int i = 0; i < cr.width * cr.height; i++)
        {
            cr.bitmap[i] = g->bitmap.buffer[i];
        }
        _characters[c] = cr;
    }
    // TODO : https://stackoverflow.com/questions/53267905/calculating-position-for-freetype-glyph-rendering
}

void textLCD::drawText(std::wstring text, int x, int y, uint32_t color)
{
    for (auto c : text)
    {
        addCharacter(c);
    }
    for (auto c : text)
    {
        if (_characters.find(c) == _characters.end())
        {
            addCharacter(c);
        }
        charRepresentation cr = _characters[c];
        std::wcerr << "Drawing character " << c << " at " << x << ", " << y << " metrics : " << cr.width << "x" << cr.height << "y advance : " << cr.advance_x << "x" << cr.advance_y << "y offset" << std::endl;

        // Calculate the position based on glyph metrics and pen position
        // Draw the glyph
        // see https://kevinboone.me/fbtextdemo.html
        for (unsigned int i = 0; i < cr.width; i++)
        {
            for (unsigned int j = 0; j < cr.height; j++)
            {
                if (cr.bitmap[j * cr.width + i] > 0)
                {
                    // Calculate pixel position within the LCD screen
                    // see https://freetype.org/freetype2/docs/glyphs/glyphs-3.html
                    // TODO : fix this fing offset !
                    int pixel_x = x + cr.g->bitmap_left + i;
                    int pixel_y = y + cr.g->bitmap_top - (cr.bbox.yMin) + j; // Adjust the y position based on glyph metrics

                    uint32_t color_alpha = _lcd->alpha_blending(color, cr.bitmap[j * cr.pitch + i]);
                    _lcd->drawPixel(pixel_x, pixel_y, _lcd->color565(color_alpha));
                }
            }
        }
        x += (cr.advance_x);
        y += (cr.advance_y);
    }
}