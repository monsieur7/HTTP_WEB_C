#ifndef ST7735_HPP
#define ST7735_HPP
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
// libgpiod :
#include <gpiod.h>
// Defines
#define BG_SPI_CS_BACK 0
#define BG_SPI_CS_FRONT 1

#define SPI_CLOCK_HZ 16000000

#define ST7735_TFTWIDTH 80
#define ST7735_TFTHEIGHT 160

#define ST7735_COLS 132
#define ST7735_ROWS 162

#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID 0x04
#define ST7735_RDDST 0x09

#define ST7735_SLPIN 0x10
#define ST7735_SLPOUT 0x11
#define ST7735_PTLON 0x12
#define ST7735_NORON 0x13

#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29

#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR 0x30
#define ST7735_MADCTL 0x36
#define ST7735_COLMOD 0x3A

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_RDID1 0xDA
#define ST7735_RDID2 0xDB
#define ST7735_RDID3 0xDC
#define ST7735_RDID4 0xDD

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define ST7735_PWCTR6 0xFC

// Colours
#define ST7735_BLACK 0x0000
#define ST7735_BLUE 0x001F
#define ST7735_GREEN 0x07E0
#define ST7735_RED 0xF800
#define ST7735_CYAN 0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW 0xFFE0
#define ST7735_WHITE 0xFFFF

class ST7735
{
private:
    int _spi_fd;
    gpiod_line *_dc_line;
    gpiod_line *_rst_line;
    gpiod_line *_bl_line;
    gpiod_chip *_chip;
    uint8_t _spi_mode;
    uint8_t _spi_bits_per_word;
    uint32_t _spi_speed;
    int _width;
    int _height;
    int _offset_x;
    int _offset_y;

public:
    ST7735(std::string spidev, std::string chipname, uint8_t spi_bits_per_word, uint32_t spi_speed, uint32_t dc_line, uint32_t rst_line, uint32_t bl_line, int width, int height);
    ~ST7735();
    void writeCommand(uint8_t command);
    void writeData(uint8_t data);
    void init();
    void setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
    void fillScreen(uint16_t colour);
    void drawPixel(uint8_t x, uint8_t y, uint16_t colour);
    void drawFullScreen(uint16_t *buffer);
};

#endif // ST7735_HPP
