#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <sstream>
#include <regex>
#include <argp.h>

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
// #define SENSOR_SUPPORT
//  JOBS :
#include "job.hpp"

// Argument parsing definitions
#define PORT 8080 // port to listen on
const char *argp_program_version = "API-REST SERVER, VERSION 0.1";
const char *argp_program_bug_address = "<nolane.de@gmail.com>";
static char doc[] = "This is a C++ implementation of a REST API server";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"port", 'p', "PORT", 0, "Set the port to listen on"},
    {"cert", 'c', "CERT", 0, "Set the certificate path"},
    {"key", 'k', "KEY", 0, "Set the key path"},
    {0, 0, 0, 0, nullptr}};

struct arguments
{
    int port;
    std::filesystem::path certPath;
    std::filesystem::path keyPath;
};
// parser function
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments *)state->input;
    switch (key)

    {
    case 'p':
        arguments->port = arg ? std::stoi(arg) : PORT;
        break;
    case 'c':
        arguments->certPath = arg ? std::filesystem::absolute(std::filesystem::path(arg)) : "";
        break;
    case 'k':
        arguments->keyPath = arg ? std::filesystem::absolute(std::filesystem::path(arg)) : "";
        break;
    case ARGP_KEY_END:
        // check if there are any non opion arguments
        if (arguments->keyPath.empty() || arguments->certPath.empty())
        {
            argp_error(state, "You must provide a certificate and a key path");
            return ARGP_ERR_UNKNOWN;
        }
        break;
    case ARGP_KEY_ARG:
        // check if there are any non opion arguments
        if (state->arg_num > 0)
        {
            argp_error(state, "Too many arguments");
            return ARGP_ERR_UNKNOWN;
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char **argv)
{
    // argument parsing
    struct arguments arguments;
    arguments.port = PORT;
    arguments.certPath = "";
    arguments.keyPath = "";

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    std::cout << "Key path : " << arguments.keyPath << std::endl;
    std::cout << "Port : " << arguments.port << std::endl;
    std::cout << "Cert path : " << arguments.certPath << std::endl;

#ifdef SENSOR_SUPPORT
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
#endif

    // HTTPS server setup
    std::filesystem::path keyPath = std::filesystem::current_path();
    std::filesystem::path certPath = std::filesystem::current_path();

    if (!arguments.keyPath.empty())
    {
        keyPath = arguments.keyPath;
    }

    if (!arguments.certPath.empty())
    {
        certPath = arguments.certPath;
    }

    SocketSecure server(arguments.port, certPath, keyPath);
    server.createSocket();
    server.bindSocket();
    ParseString p("");
    while (true)
    {
        server.listenSocket();
        std::vector<int> clients;
        try
        {
            clients = server.pollClients(1000);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << '\n';
        }
        for (std::size_t i = 0; i < clients.size(); i++)
        {
            std::cout << server.printClientInfo(clients[i]) << std::endl;
            std::string received;
            try
            {
                received = server.receiveSocket(clients[i]);
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << e.what() << '\n';
                server.closeSocket(clients[i]);
                continue;
            }

            p.setData(received);
            std::map<std::string, std::string> headers = p.parseRequest();
            for (auto const &x : headers)
            {
                std::cout << x.first << " : " << x.second << std::endl;
            }
            // send a dummy response
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 5\r\n\r\nHello";
            server.sendSocket(response, clients[i]);
            server.closeSocket(clients[i]); // close the connection !
        }
    }

    return 0;
}
