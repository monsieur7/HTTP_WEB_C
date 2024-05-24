// INTERNAL LIBS :
#include "ADS1015.hpp"
#include "BME280.hpp"
#include "MICS6814.hpp"
#include "LTR559.hpp"

// LCD SCREEN :
#include "ST7735.hpp"
#include "textLCD.hpp"

// HTTPS SERVER
#include "socketSecure.hpp"
#include "generateHeaders.hpp"
#include "parseString.hpp"

// EXTERNAL LIBS :
#include <nlohmann/json.hpp>
#include "redisQueue.hpp"
#include "FileTypeDetector.hpp"

// JOBS :
#include "job.hpp"
// STD LIBS :
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <sstream>

#include <argp.h>
#define PORT 8080 // port to listen on
const char *argp_program_version = "API-REST SERVER, VERSION 0.1";
const char *argp_program_bug_address = "<nolane.de@gmail.com>";
static char doc[] = "This is a c++ implementation of a REST API server";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"port", 'p', "PORT", OPTION_ARG_OPTIONAL, "Set the port to listen on"},
    {"cert", 'c', "CERT", OPTION_ARG_OPTIONAL, "Set the certificate path"},
    {"key", 'k', "KEY", 0, "Set the key path"},
    {0}};

struct arguments
{
    int port;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = static_cast<struct arguments *>(state->input);
    switch (key)
    {
    case 'p':
        arguments->port = atoi(arg);
        break;
    case 'c':
        std::cout << "cert path : " << arg << std::endl;
        break;
    case 'k':
        std::cout << "key path : " << arg << std::endl;
        break;
    case ARGP_KEY_ARG:
        return 0;
    case ARGP_KEY_END:
        break;
    case ARGP_KEY_NO_ARGS:
        argp_usage(state);
        break;
    case ARGP_KEY_ERROR:
        argp_state_help(state, stderr, ARGP_HELP_STD_ERR);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, nullptr, nullptr, nullptr};

int main(int argc, char **argv)
{
    // argument parsing
    struct arguments arguments;
    arguments.port = PORT;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /*

    // SETUP SENSORS :
    BME280 bme280();
    MICS6814 mics6814();
    LTR559 ltr559();

    // SETUP LCD SCREEN :
    ST7735 lcd = ST7735("/dev/spidev0.1", "gpiochip0", 8, 10000000, 9, -1, 12, 80, 160); // 80x160 (because its rotated !)
    lcd.init();
    lcd.fillScreen(ST7735_BLACK);

    textLCD textLCD = textLCD("../arial.ttf", 24, &lcd);

    if (bme280.begin() != 0)
    {
        std::cerr << "Error while initializing BME280" << std::endl;
        return -1;
    }
    */
    // HTTPS server setup :
    //  key :
    std::filesystem::path keyPath = std::filesystem::current_path();
    keyPath /= "..";
    keyPath /= "raspberrypi-fr.local.key";

    // crt :
    std::filesystem::path certPath = std::filesystem::current_path();
    certPath /= "..";
    certPath /= "raspberrypi-fr.local.crt";

    SocketSecure server(arguments.port, keyPath, certPath);
}