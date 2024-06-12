#include "ST7735.hpp"
// based ion pimoroni ST7735python library
ST7735::ST7735(std::string spidev, std::string chipname, uint8_t spi_bits_per_word, uint32_t spi_speed, uint32_t dc_line, uint32_t rst_line, uint32_t bl_line, int width, int height)
{
    this->_spi_fd = open(spidev.c_str(), O_RDWR);
    if (this->_spi_fd < 0)
    {
        throw std::runtime_error("Error opening SPI device");
    }
    // mode 0
    this->_spi_mode = SPI_MODE_0;
    if (ioctl(this->_spi_fd, SPI_IOC_WR_MODE, &this->_spi_mode) < 0)
    {
        throw std::runtime_error("Error setting SPI mode");
    }
    // set lsb first to false
    uint8_t lsb_first = 0;
    if (ioctl(this->_spi_fd, SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0)
    {
        throw std::runtime_error("Error setting SPI lsb first");
    }
    // same to read
    if (ioctl(this->_spi_fd, SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
    {
        throw std::runtime_error("Error setting SPI lsb first");
    }
    // bits per word
    this->_spi_bits_per_word = spi_bits_per_word;
    if (ioctl(this->_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &this->_spi_bits_per_word) < 0)
    {
        throw std::runtime_error("Error setting SPI bits per word");
    }
    // speed
    this->_spi_speed = spi_speed;
    if (ioctl(this->_spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &this->_spi_speed) < 0)
    {
        throw std::runtime_error("Error setting SPI speed");
    }
    // chip
    this->_chip = gpiod_chip_open_by_name(chipname.c_str());
    if (!this->_chip)
    {
        throw std::runtime_error("Error opening GPIO chip");
    }
    // dc
    this->_dc_line = gpiod_chip_get_line(this->_chip, dc_line);
    if (!this->_dc_line)
    {
        throw std::runtime_error("Error getting GPIO line");
    }
    if (gpiod_line_request_output(this->_dc_line, "dc", 0) < 0)
    {
        throw std::runtime_error("Error requesting GPIO line");
    }
    // rst
    if (rst_line != -1)
    {
        this->_rst_line = gpiod_chip_get_line(this->_chip, rst_line);
        if (!this->_rst_line)
        {
            throw std::runtime_error("Error getting GPIO line");
        }
        if (gpiod_line_request_output(this->_rst_line, "rst", 0) < 0)
        {
            throw std::runtime_error("Error requesting GPIO line");
        }
    }
    else
    {
        this->_rst_line = NULL;
    }
    // bl
    this->_bl_line = gpiod_chip_get_line(this->_chip, bl_line);
    if (!this->_bl_line)
    {
        throw std::runtime_error("Error getting GPIO line");
    }
    if (gpiod_line_request_output(this->_bl_line, "bl", 0) < 0)
    {
        throw std::runtime_error("Error requesting GPIO line");
    }
    // set backlight on :
    gpiod_line_set_value(this->_bl_line, 1);

    this->_width = width;
    this->_height = height;
    this->_offset_x = (ST7735_COLS - width) / 2;  // OFFSET LEFT
    this->_offset_y = (ST7735_ROWS - height) / 2; // OFFSET TOP

    init();
}

ST7735::~ST7735()
{
    close(this->_spi_fd);
    gpiod_line_release(this->_dc_line);
    if (this->_rst_line != NULL)
    {
        gpiod_line_release(this->_rst_line);
    }
    gpiod_line_release(this->_bl_line);
    gpiod_chip_close(this->_chip);
}
void ST7735::writeCommand(uint8_t command)
{
    uint8_t tx[1] = {command};
    uint8_t rx[1] = {0};
    struct spi_ioc_transfer tr = {0}; // Zero-initialize all members
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 1;
    tr.speed_hz = this->_spi_speed;
    tr.delay_usecs = 0;
    tr.bits_per_word = this->_spi_bits_per_word;

    // set DC pin low for command :
    gpiod_line_set_value(this->_dc_line, 0);
    // write command :
    if (ioctl(this->_spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1)
    {
        throw std::runtime_error("Error writing SPI message");
    }
    //
}
void ST7735::writeData(uint8_t data)
{
    uint8_t tx[1] = {data};
    uint8_t rx[1] = {0};
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 1,
        .speed_hz = this->_spi_speed,
        .delay_usecs = 0,
        .bits_per_word = this->_spi_bits_per_word,
    };
    // set DC pin high for data :
    gpiod_line_set_value(this->_dc_line, 1);
    // write data :
    if (ioctl(this->_spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1)
    {
        throw std::runtime_error("Error writing SPI message");
    }
    //
}

void ST7735::init()
{
    writeCommand(ST7735_SWRESET); // Software reset
    usleep(150 * 1000);           // Delay 150 ms

    writeCommand(ST7735_SLPOUT); // Sleep out
    usleep(500 * 1000);          // Delay 500 ms

    writeCommand(ST7735_FRMCTR1); // Frame rate control - normal mode
    writeData(0x01);
    writeData(0x2C);
    writeData(0x2D);

    writeCommand(ST7735_FRMCTR2); // Frame rate control - idle mode
    writeData(0x01);
    writeData(0x2C);
    writeData(0x2D);

    writeCommand(ST7735_FRMCTR3); // Frame rate control - partial mode
    writeData(0x01);
    writeData(0x2C);
    writeData(0x2D);
    writeData(0x01);
    writeData(0x2C);
    writeData(0x2D);

    writeCommand(ST7735_INVCTR); // Display inversion control
    writeData(0x07);             // No inversion

    writeCommand(ST7735_PWCTR1); // Power control
    writeData(0xA2);
    writeData(0x02); // -4.6V
    writeData(0x84); // Auto mode

    writeCommand(ST7735_PWCTR2); // Power control
    writeData(0x0A);             // Opamp current small
    writeData(0x00);             // Boost frequency

    writeCommand(ST7735_PWCTR4); // Power control
    writeData(0x8A);             // BCLK/2, Opamp current small & Medium low
    writeData(0x2A);

    writeCommand(ST7735_PWCTR5); // Power control
    writeData(0x8A);
    writeData(0xEE);

    writeCommand(ST7735_VMCTR1); // Power control
    writeData(0x0E);

    writeCommand(ST7735_INVON); // Don't invert display

    writeCommand(ST7735_MADCTL); // Memory access control (directions)
    writeData(0xC8);             // Row address/col address, bottom to top refresh (BGR MODE) ! check if that is the right mode

    writeCommand(ST7735_COLMOD); // Set color mode
    writeData(0x05);             // 16-bit color

    writeCommand(ST7735_CASET); // Column address set
    writeData(0x00);
    writeData(this->_offset_x);
    writeData(0x00);
    writeData(this->_width + this->_offset_x - 1);

    writeCommand(ST7735_RASET); // Row address set
    writeData(0x00);
    writeData(this->_offset_y);
    writeData(0x00);
    writeData(this->_height + this->_offset_y - 1);

    writeCommand(ST7735_GMCTRP1); // Set Gamma
    writeData(0x02);
    writeData(0x1c);
    writeData(0x07);
    writeData(0x12);
    writeData(0x37);
    writeData(0x32);
    writeData(0x29);
    writeData(0x2d);
    writeData(0x29);
    writeData(0x25);
    writeData(0x2B);
    writeData(0x39);
    writeData(0x00);
    writeData(0x01);
    writeData(0x03);
    writeData(0x10);

    writeCommand(ST7735_GMCTRN1); // Set Gamma
    writeData(0x03);
    writeData(0x1d);
    writeData(0x07);
    writeData(0x06);
    writeData(0x2E);
    writeData(0x2C);
    writeData(0x29);
    writeData(0x2D);
    writeData(0x2E);
    writeData(0x2E);
    writeData(0x37);
    writeData(0x3F);
    writeData(0x00);
    writeData(0x00);
    writeData(0x02);
    writeData(0x10);

    writeCommand(ST7735_NORON); // Normal display on
    usleep(10 * 1000);          // Delay 10 ms

    writeCommand(ST7735_DISPON); // Display on
    usleep(100 * 1000);          // Delay 100 ms
}

void ST7735::setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint16_t x_start = x0 + this->_offset_x;
    uint16_t x_end = x1 + this->_offset_x;
    uint16_t y_start = y0 + this->_offset_y;
    uint16_t y_end = y1 + this->_offset_y;
    writeCommand(ST7735_CASET); // Column addr set
    writeData(x_start >> 8);
    writeData(x_start & 0xFF); // XSTART
    writeData(x_end >> 8);
    writeData(x_end & 0xFF); // XEND

    writeCommand(ST7735_RASET); // Row addr set
    writeData(y_start >> 8);
    writeData(y_start & 0xFF); // YSTART
    writeData(y_end >> 8);
    writeData(y_end & 0xFF);    // YEND
    writeCommand(ST7735_RAMWR); // write to RAM
}
void ST7735::fillScreen(u_int16_t color)
{
    setWindow(0, 0, this->_width - 1, this->_height - 1);
    for (size_t i = 0; i < this->_width * this->_height; i++)
    {
        writeData(color >> 8);
        writeData(color & 0xFF);
    }
}

void ST7735::drawFullScreen(uint16_t *buffer)
{
    setWindow(0, 0, this->_height - 1, this->_width - 1);
    for (size_t i = 0; i < this->_width * this->_height; i++)
    {
        writeData(buffer[i] >> 8);
        writeData(buffer[i] & 0xFF);
    }
}
uint16_t ST7735::color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
uint16_t ST7735::color565(uint8_t grayscale)
{
    return ((grayscale & 0xF8) << 8) | ((grayscale & 0xFC) << 3) | (grayscale >> 3);
}
void ST7735::drawPixel(uint8_t x, uint8_t y, uint16_t color)
{
    // rotate by 90°  and flip horizontally
    y = this->_width - y - 1;
    setWindow(y, x, y, x);
    writeData(color >> 8);
    writeData(color & 0xFF);
}
uint32_t ST7735::alpha_blending(uint32_t color, uint8_t alpha)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    r = (r * alpha) / 255;
    g = (g * alpha) / 255;
    b = (b * alpha) / 255;
    return (r << 16) | (g << 8) | b;
}
// TODO optimize this function
void ST7735::drawBufferMono(uint8_t *buffer, uint16_t color, uint16_t bg_color, size_t height, size_t width, int x_offset)
// the screen is rotated by 90° and flipped horizontally !
// for x offset that means that is in reality a y offset !
{
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            if (buffer[i * width + j] == 0)
            {
                drawPixel(j + x_offset, i, bg_color);
            }
            else
            {
                drawPixel(j + x_offset, i, color);
            }
        }
    }
}

size_t ST7735::getWidth() const
{
    return this->_width;
}
size_t ST7735::getHeight() const
{
    return this->_height;
}