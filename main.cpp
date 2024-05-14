
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
// local include

#include "BME280.hpp"
#include "LTR559.hpp"
#include "ST7735.hpp"

#include "MICS6814.hpp"

#include "textLCD.hpp"
int main()
{
    BME280 bme280;
    LTR559 ltr559;
    MICS6814 mics6814;

    float lux = ltr559.getLux();

    // LCD SCREEN :
    ST7735 lcd = ST7735("/dev/spidev0.1", "gpiochip0", 8, 10000000, 9, -1, 12, 80, 160); // 80x160 (because its rotated !)

    lcd.init();

    lcd.fillScreen(ST7735_RED);
    // pause
    std::cerr << "LCD INITIALIZED" << std::endl;

    float oxydising = mics6814.readOxydising();
    float nh3 = mics6814.readNH3();
    float reducing = mics6814.readReducing();

    // initializing BME280
    if (bme280.begin() != 0)
    {
        std::cerr << "Error while initializing BME280" << std::endl;
        return 1;
    }
    // reading data from BME280
    float temperature = bme280.readTemp();
    float pressure = bme280.readPressure();
    float humidity = bme280.readHumidity();

    float altitude = bme280.readAltitude(1020.0f);
    lux = ltr559.getLux();
    // Display the data
    std::cout << "Temperature : " << temperature << " °C" << std::endl;
    std::cout << "Pressure : " << pressure / 100.0f << " hPa" << std::endl;
    std::cout << "Humitidy : " << humidity << " %" << std::endl;
    std::cout << "Altitude : " << altitude << " m" << std::endl;
    std::cout << "Oxydising : " << oxydising << " Ohms" << std::endl;
    std::cout << "Reducing : " << reducing << " Ohms" << std::endl;
    std::cout << "NH3 : " << nh3 << " Ohms" << std::endl;
    std::cout << "Lux : " << lux << std::endl;
    std::cout << "Proximity : " << ltr559.getProximity() << std::endl;
    // text printing with freeType
    textLCD textlcd = textLCD("../arial.ttf", 2, &lcd);
    textlcd.drawText(L"Temperature : " + std::to_wstring(temperature) + L" °C", 0, 0, ST7735_WHITE);
}
