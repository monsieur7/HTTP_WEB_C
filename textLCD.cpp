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
    FT_Set_Char_Size(_face, 0, pixel_size << 6, 0, 0);
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
    FT_Int baseline = 0, height = 0, width = 0;
    FT_GlyphSlot g = _face->glyph;
    wchar_t previous_char = NULL;

    // Calculate baseline, height, and width
    for (wchar_t c : text)
    {
        if (FT_Load_Char(_face, c, FT_LOAD_RENDER))
        {
            throw std::runtime_error("Error loading character");
        }

        baseline = std::max(baseline, std::max((FT_Int)0, -(g->bitmap_top - (FT_Int)g->bitmap.rows)));
        height = std::max(height, (FT_Int)g->bitmap.rows + baseline);

        if (previous_char != NULL)
        {
            FT_Vector kerning;
            FT_UInt left_glyph = FT_Get_Char_Index(_face, previous_char);
            FT_UInt right_glyph = FT_Get_Char_Index(_face, c);
            FT_Get_Kerning(_face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
            width += kerning.x >> 6;
        }
        width += g->advance.x >> 6;
        previous_char = c;
    }

    uint8_t *buffer = new uint8_t[width * height];
    memset(buffer, 0, width * height);

    int current_x = 0;
    previous_char = NULL;

    for (wchar_t c : text)
    {
        if (FT_Load_Char(_face, c, FT_LOAD_RENDER))
        {
            throw std::runtime_error("Error loading character");
        }

        FT_Bitmap &bitmap = g->bitmap;
        int y_offset = height - baseline - g->bitmap_top;

        if (previous_char != NULL)
        {
            FT_Vector kerning;
            FT_UInt left_glyph = FT_Get_Char_Index(_face, previous_char);
            FT_UInt right_glyph = FT_Get_Char_Index(_face, c);
            FT_Get_Kerning(_face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
            current_x += kerning.x >> 6;
        }

        for (unsigned int i = 0; i < bitmap.rows; ++i)
        {
            for (unsigned int j = 0; j < bitmap.width; ++j)
            {
                if (bitmap.buffer[i * bitmap.width + j])
                {
                    buffer[(y_offset + i) * width + current_x + g->bitmap_left + j] = bitmap.buffer[i * bitmap.width + j];
                }
            }
        }

        current_x += g->advance.x >> 6;
        previous_char = c;
    }

    _lcd->drawBufferMono(buffer, color, ST7735_BLACK, height, width);

    delete[] buffer;
}
