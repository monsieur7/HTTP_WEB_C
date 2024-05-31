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
void textLCD::drawText(std::wstring text, int x, int y, uint16_t color, uint16_t bg_color)
// two step rendering :
// 1st : calculate the text bounding box
// 2nd : render the text in the bounding box
{
    // for all calculations see https://github.com/rougier/freetype-py/blob/master/examples/hello-world.py as a reference (its a good starting point !)
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
        // get bbox :
        FT_BBox bbox;
        FT_Glyph glyph;
        FT_Get_Glyph(g, &glyph);
        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &bbox);

        baseline = std::max(baseline, (FT_Int)(-(bbox.yMin))); // - because yMin is negative and y axis is in reverse
        height = std::max(height, (FT_Int)(bbox.yMax - bbox.yMin));
        std::wcerr << "char " << c << "height : " << height << " width : " << width << " baseline : " << baseline << " yMax : " << bbox.yMax << " yMin : " << bbox.yMin << std::endl;
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
        std::wcerr << "char " << c << " y_offset : " << y_offset << " bitmap_top : " << g->bitmap_top << " bitmap_left : " << g->bitmap_left << std::endl;
        if (y_offset < 0)
        {
            y_offset = 0;
            std::cerr << "y_offset < 0" << std::endl;
        }
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

    _lcd->drawBufferMono(buffer, color, bg_color, height, width);

    delete[] buffer;
}
