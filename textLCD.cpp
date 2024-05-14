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
    FT_Set_Char_Size(_face, 0, pixel_size * 64, 300, 300); // * 64 to convert to 26.6 fixed point
    if (FT_Select_Charmap(_face, FT_ENCODING_UNICODE))
    {
        throw std::runtime_error("Error setting charmap");
    }
}

textLCD::~textLCD()
{
    FT_Done_Face(_face);
    FT_Done_FreeType(_ft);
}

void textLCD::drawText(std::wstring text, int x, int y, uint32_t color)

{
    unsigned int baseline = 0, height = 0, width = 0;
    FT_GlyphSlot g = _face->glyph;
    wchar_t previous_char = NULL;
    // taken from python code : #https://github.com/rougier/freetype-py/blob/master/examples/hello-world.py

    for (wchar_t c : text)
    {

        FT_Load_Char(_face, c, FT_LOAD_RENDER);
        baseline = std::max(baseline, std::max(0u, -(g->bitmap_top - g->bitmap.rows)));
        height = std::max(height, g->bitmap.rows + baseline);
        std::wcerr << "char " c << " height : " << height << " baseline : " << baseline << " bitmap_top : " << g->bitmap_top << " bitmap.rows : " << g->bitmap.rows << std::endl;
        if (previous_char != NULL)
        {
            FT_Vector kerning;
            auto left_glyph = FT_Get_Char_Index(_face, previous_char);
            auto right_glyph = FT_Get_Char_Index(_face, c);
            FT_Get_Kerning(_face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
            width += kerning.x >> 6;
        }
        width += g->advance.x >> 6;
    }
    std::cerr << "width : " << width << " height : " << height << "Allocating buffer" << std::endl;
    uint8_t *buffer = new uint8_t[width * height];
    memset(buffer, 0, width * height);
    int ct = 0;
    previous_char = NULL;
    for (wchar_t c : text)
    {
        FT_Load_Char(_face, c, FT_LOAD_RENDER);
        FT_Bitmap bitmap = g->bitmap;
        y = height - (g->bitmap_top - baseline);
        if (g->bitmap_left < 0 && ct == 0)
        {
            x += g->bitmap_left; // first char
        }
        if (ct > 0)
        {
            FT_Vector kerning;
            auto left_glyph = FT_Get_Char_Index(_face, previous_char);
            auto right_glyph = FT_Get_Char_Index(_face, c);
            FT_Get_Kerning(_face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
            x += kerning.x >> 6;
        }
        previous_char = c; // save previous char
        for (unsigned int i = 0; i < bitmap.rows; i++)
        {
            for (unsigned int j = 0; j < bitmap.width; j++)
            {
                buffer[(y + i) * g->bitmap.rows + (g->bitmap_left + x + j)] = bitmap.buffer[i * bitmap.rows + j];
            }
        }
        x += g->advance.x >> 6;
        ct++;
    }
    // TODO : line wrap !
    _lcd->drawBufferMono(buffer, color, ST7735_BLACK, height, width);
}