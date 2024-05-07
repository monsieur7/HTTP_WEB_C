#include "socketSecure.hpp"
#include "parseString.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include "FileTypeDetector.hpp"
#include "BME280.hpp"
#include "LTR559.hpp"
#include "ADS1015.hpp"

// #include <format>
#define PORT 8080 // port to listen on
std::filesystem::directory_entry findFile(std::map<std::filesystem::directory_entry, std::string> &files, std::string file)
{
    if (file[0] != '/')
    {
        return std::filesystem::directory_entry();
    }
    for (const auto &entry : files)
    {
        if (entry.first.path().filename() == file.substr(1))
        {
            return entry.first;
        }
    }
    return std::filesystem::directory_entry();
}
int main()
{
    BME280 bme280;
    LTR559 ltr559;

    ADS1015 ads1015;
    // config :
    CONFIG_REGISTER config;
    config.reg = CONFIG_REGISTER_MODE_CONTINUOUS |
                 CONFIG_REGISTER_OS_ON | CONFIG_REGISTER_MUX_AIN0_GND |
                 CONFIG_REGISTER_PGA_2048V | CONFIG_REGISTER_DR_1600SPS |
                 CONFIG_REGISTER_COMP_QUE_DISABLE;
    ads1015.setConfig(config);

    std::cout << "config ADC in CHIP" << std::bitset<16>(ads1015.getConfig()) << std::endl;
    std::cout << "Written config ADC " << std::bitset<16>(config.reg) << std::endl;

    float lux = ltr559.getLux();
    float voltage = ads1015.readADC();

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
    // Display the data
    std::cout << "Temperature : " << temperature << " °C" << std::endl;
    std::cout << "Pressure : " << pressure / 100.0f << " hPa" << std::endl;
    std::cout << "Humitidy : " << humidity << " %" << std::endl;
    std::cout << "Altitude : " << altitude << " m" << std::endl;
    std::cout << "Lux : " << lux << std::endl;
    std::cout << "Voltage : " << voltage << " V" << std::endl;
    // OPENSSL INIT :

    FileTypeDetector ftd;
    ftd.addSingleFileType("html", "text/html");
    ftd.addSingleFileType("css", "text/css");
    ftd.addSingleFileType("js", "text/javascript");
    ftd.addSingleFileType("txt", "");

    std::string dir = "www";
    std::filesystem::path path = std::filesystem::current_path();
    path /= "../";
    path /= dir;

    // key :
    std::filesystem::path keyPath = std::filesystem::current_path();
    keyPath /= "..";
    keyPath /= "raspberrypi-fr.local.key";

    // crt :
    std::filesystem::path certPath = std::filesystem::current_path();
    certPath /= "..";
    certPath /= "raspberrypi-fr.local.crt";

    // list all files in this dir:
    std::vector<std::filesystem::directory_entry> files;
    std::map<std::filesystem::directory_entry, std::string> file_types;
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        files.push_back(entry);
    }
    for (const auto &file : files)
    {
        std::cout << file << std::endl;
        std::string extension = file.path().extension();
        std::string type = ftd.getFileType(extension);
        file_types[file] = type;
    }
    SocketSecure s(PORT, certPath, keyPath);
    auto file = std::ifstream(path / "index.html", std::ios::binary);
    std::string response404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\n";
    // TODO : keep alive !

    s.createSocket();
    s.bindSocket();
    ParseString p("");
    while (true)
    {
        s.listenSocket();
        std::vector<int> clients = s.pollClients(1000);
        for (auto i = 0; i < clients.size(); i++)
        {
            std::cout << s.printClientInfo(clients[i]) << std::endl;
            std::string received = s.receiveSocket(clients[i]);
            p.setData(received);
            std::map<std::string, std::string> headers = p.parseRequest();
            for (auto const &x : headers)
            {
                std::cout << x.first << " : " << x.second << std::endl;
            }
            if (headers["Path"] == "/")
            {
                headers["Path"] = "/index.html"; // default file
            }
            std::filesystem::directory_entry file = findFile(file_types, headers["Path"]);
            if (file.path().filename() != "")
            {
                int file_size = file.file_size();
                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: Keep-Alive\r\nContent-Length: " + std::to_string(file_size) + "\r\n\r\n";
                s.sendSocket(response.c_str(), response.size(), clients[i]);
                s.sendFile(file, clients[i]);
            }
            // search in files if there is a file with this name and send it
            // if not send 404
            else
            {
                s.sendSocket(response404.c_str(), response404.size(), clients[i]);
                s.closeSocket(clients[i]); // close the connection
            }
        }
    }
    file.close();
    return 0;
}
