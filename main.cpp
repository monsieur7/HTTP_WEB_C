
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
// local include

#include "BME280.hpp"
#include "LTR559.hpp"
#include "ADS1015.hpp"
#include "MICS6814.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
int main()
{
    BME280 bme280;
    LTR559 ltr559;
    MICS6814 mics6814;

    float oxydising = mics6814.readOxydising();
    float nh3 = mics6814.readNH3();
    float reducing = mics6814.readReducing();

    float lux = ltr559.getLux();

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

    // freetype init
    FT_Library library;

    FT_Face face;
    FT_Error error;
    error = FT_Init_FreeType(&library);
    if (error)
    {
        std::cerr << "Error while initializing FreeType" << std::endl;
        return 1;
    }
    error = FT_New_Face(library, "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 0, &face);
    if (error == FT_Err_Unknown_File_Format)
    {
        std::cerr << "Error while loading font file" << std::endl;
        return 1;
    }
    else if (error)
    {
        std::cerr << "Error while loading font file" << std::endl;
        return 1;
    }
    FT_Set_Pixel_Sizes(face, 0, 12);
    // load caracter A
    if (FT_Load_Char(face, 'A', FT_LOAD_RENDER))
    {
        std::cerr << "Error while loading character" << std::endl;
        return 1;
    }
    // render bitmap on screen :

    FT_Done_Face(face);
}
