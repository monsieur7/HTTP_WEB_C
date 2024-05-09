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
    this->_width = width;
    this->_height = height;
    this->_offset_x = (ST7735_COLS - width) / 2;
    this->_offset_y = (ST7735_ROWS - height) / 2;

    init();
}

ST7735::~ST7735()
{
    close(this->_spi_fd);
    gpiod_line_release(this->_dc_line);
    gpiod_line_release(this->_rst_line);
    gpiod_line_release(this->_bl_line);
    gpiod_chip_close(this->_chip);
}
void ST7735::writeCommand(uint8_t command)
{
    gpiod_line_set_value(this->_dc_line, 0);
    uint8_t tx[1] = {command};
    uint8_t rx[1] = {0};
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 1,
        .speed_hz = this->_spi_speed,
        .bits_per_word = this->_spi_bits_per_word,
        .delay_usecs = 0,
    };
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
    gpiod_line_set_value(this->_dc_line, 1);
    uint8_t tx[1] = {data};
    uint8_t rx[1] = {0};
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 1,
        .speed_hz = this->_spi_speed,
        .bits_per_word = this->_spi_bits_per_word,
        .delay_usecs = 0,
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
    sendCommand(ST7735_SWRESET); // Software reset
    usleep(150 * 1000);          // Delay 150 ms

    sendCommand(ST7735_SLPOUT); // Sleep out
    usleep(500 * 1000);         // Delay 500 ms

    sendCommand(ST7735_FRMCTR1); // Frame rate control - normal mode
    sendData(0x01);
    sendData(0x2C);
    sendData(0x2D);
    usleep(10 * 1000); // Delay 10 ms

    sendCommand(ST7735_FRMCTR2); // Frame rate control - idle mode
    sendData(0x01);
    sendData(0x2C);
    sendData(0x2D);
    usleep(10 * 1000); // Delay 10 ms

    sendCommand(ST7735_FRMCTR3); // Frame rate control - partial mode
    sendData(0x01);
    sendData(0x2C);
    sendData(0x2D);
    sendData(0x01);
    sendData(0x2C);
    sendData(0x2D);
    usleep(10 * 1000); // Delay 10 ms

    sendCommand(ST7735_INVCTR); // Display inversion control
    sendData(0x07);             // No inversion

    sendCommand(ST7735_PWCTR1); // Power control
    sendData(0xA2);
    sendData(0x02); // -4.6V
    sendData(0x84); // Auto mode

    sendCommand(ST7735_PWCTR2); // Power control
    sendData(0x0A);             // Opamp current small
    sendData(0x00);             // Boost frequency

    sendCommand(ST7735_PWCTR4); // Power control
    sendData(0x8A);             // BCLK/2, Opamp current small & Medium low
    sendData(0x2A);

    sendCommand(ST7735_PWCTR5); // Power control
    sendData(0x8A);
    sendData(0xEE);

    sendCommand(ST7735_VMCTR1); // Power control
    sendData(0x0E);

    sendCommand(ST7735_INVOFF); // Don't invert display

    sendCommand(ST7735_MADCTL); // Memory access control (directions)
    sendData(0xC8);             // Row address/col address, bottom to top refresh

    sendCommand(ST7735_COLMOD); // Set color mode
    sendData(0x05);             // 16-bit color

    sendCommand(ST7735_CASET);  // Column address set
    sendData(0x00);             // XSTART = 0
    sendData(this->_width - 1); // XEND = _width - 1

    sendCommand(ST7735_RASET);   // Row address set
    sendData(0x00);              // YSTART = 0
    sendData(this->_height - 1); // YEND = _height - 1

    sendCommand(ST7735_NORON); // Normal display on
    usleep(10 * 1000);         // Delay 10 ms

    sendCommand(ST7735_DISPON); // Display on
    usleep(100 * 1000);         // Delay 100 ms
}

void ST7735::setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    sendCommand(ST7735_CASET); // Column addr set
    sendData(0x00);
    sendData(x0 + this->_offset_x); // XSTART
    sendData(0x00);
    sendData(x1 + this->_offset_x); // XEND

    sendCommand(ST7735_RASET); // Row addr set
    sendData(0x00);
    sendData(y0 + this->_offset_y); // YSTART
    sendData(0x00);
    sendData(y1 + this->_offset_y); // YEND

    sendCommand(ST7735_RAMWR); // write to RAM
}
void ST7735::fillScreen(u_int16_t color)
{
    setWindow(0, 0, this->_width - 1, this->_height - 1);
    for (int i = 0; i < this->_width * this->_height; i++)
    {
        sendData(color >> 8);
        sendData(color & 0xFF);
    }
}

void ST7735::drawFullScreen(uint16_t *buffer)
{
    setWindow(0, 0, this->_width - 1, this->_height - 1);
    for (int i = 0; i < this->_width * this->_height; i++)
    {
        sendData(buffer[i] >> 8);
        sendData(buffer[i] & 0xFF);
    }
}