#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <sstream>
#include <regex>
#include <argp.h>
#include <thread>

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

#define file_timeout 30 * 60 // 30 minutes
// JSON :
#include <nlohmann/json.hpp>
std::atomic<bool> running(true);
struct audio_pass_data
{
    int duration;
    std::filesystem::path outputFile;
    redisContext *c;
};

void continous_polling(redisQueue &q)
{
    while (running)
    {
        q.startFirstJob();
    }
    std::cout << "Exiting continous polling" << std::endl;
}

int recordAudio(int duration, const std::filesystem::path &outputFile, redisContext *c)
{
    // Construct the command to record audio
    std::string command = "arecord -d " + std::to_string(duration) + " -f cd " + outputFile.c_str();

    // Execute the command
    int result = std::system(command.c_str());

    // Check if the command was successful
    if (result != 0)
    {
        std::cerr << "Error: Failed to record audio." << std::endl;
        return result;
    }
    else
    {
        std::cout << "Recording saved to " << outputFile << std::endl;
        // Add the file to the redis queue
        std::string key = "audio_file";
        nlohmann::json j;
        j["file"] = outputFile.c_str();
        j["duration"] = duration;
        j["timeCreated"] = std::time(nullptr);
        std::string json = j.dump();
        redisReply *reply = (redisReply *)redisCommand(c, "RPUSH %s %s", key.c_str(), json.c_str());
        if (reply == NULL)
        {
            std::cerr << "Error: Could not add audio file to redis queue" << std::endl;
            throw std::runtime_error("Error: Could not add audio file to redis queue");
        }
        freeReplyObject(reply);
        return 0;
    }
}

void cleanupTempFile(redisContext *c)
{
    // get the audio file from the redis queue
    redisReply *reply = (redisReply *)redisCommand(c, "LRANGE audio_file 0 -1");
    if (reply == NULL)
    {
        std::cerr << "Error: Could not get audio file from redis queue" << std::endl;
        throw std::runtime_error("Error: Could not get audio file from redis queue");
    }
    if (reply->type != REDIS_REPLY_ARRAY && reply->type != REDIS_REPLY_NIL)
    {
        std::cerr << "Error: Invalid reply type" << std::endl;
        std::cerr << "Error: " << reply->type << std::endl;
        freeReplyObject(reply);
        throw std::runtime_error("Error: Invalid reply type");
    }
    if (reply->elements == 0 || reply->type == REDIS_REPLY_NIL)
    {
        std::cerr << "Error: No audio files in the queue" << std::endl;
        freeReplyObject(reply);
        return;
    }
    for (size_t i = 0; i < reply->elements; i++)
    {
        std::string json = reply->element[i]->str;
        nlohmann::json j = nlohmann::json::parse(json);
        std::filesystem::path filePath = j["file"];
        time_t timeCreated = j["timeCreated"];
        double diff = std::difftime(std::time(nullptr), timeCreated);
        if (diff > file_timeout)
        {
            // delete the file
            std::filesystem::remove(filePath);
            // remove the file from the queue
            redisReply *replyRemove = (redisReply *)redisCommand(c, "LREM audio_file 0 %s", json.c_str());
            if (replyRemove == NULL)
            {
                std::cerr << "Error: Could not remove audio file from redis queue" << std::endl;
                throw std::runtime_error("Error: Could not remove audio file from redis queue");
            }
            freeReplyObject(replyRemove);
        }
    }
    freeReplyObject(reply);
}

void cleanupTempFiles_process(redisContext *c)
{
    while (running)
    {
        cleanupTempFile(c);
        std::cerr << "Cleaning up temporary files" << std::endl;
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

std::filesystem::path createTempFile()
{
    // create a temporary file with mktemp
    char template_name[] = "/tmp/audioXXXXXX";
    int fd = mkstemp(template_name);
    if (fd == -1)
    {
        std::cerr << "Error: Could not create temporary file" << std::endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
        throw std::runtime_error("Error: Could not create temporary file");
    }

    std::filesystem::path filePath = std::filesystem::read_symlink("/proc/self/fd/" + std::to_string(fd));
    // close the file descriptor
    close(fd);
    return filePath;
}
int record_audio(void *arg)
{
    audio_pass_data *data = (audio_pass_data *)arg;
    return recordAudio(data->duration, data->outputFile, data->c);
}

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
    // connect to redis :
    redisContext *c = redisConnect("127.0.0.1", 6379);
    redisContext *cleanup_context = redisConnect("127.0.0.1", 6379); // 2nd connection for cleanup
    if (c == NULL || c->err)
    {
        if (c)
        {
            std::cerr << "Redis Error: " << c->errstr << std::endl;
            redisFree(c);
        }
        else
        {
            std::cerr << "Error: Could not allocate redis context" << std::endl;
        }
        exit(1);
    }
    if (cleanup_context == NULL || cleanup_context->err)
    {
        if (cleanup_context)
        {
            std::cerr << "Redis Error: " << cleanup_context->errstr << std::endl;
            redisFree(cleanup_context);
        }
        else
        {
            std::cerr << "Error: Could not allocate redis context" << std::endl;
        }
        exit(1);
    }
    redisQueue queue(c);
    std::cerr << "Starting cleanup thread" << std::endl;
    std::thread cleanupThread(cleanupTempFiles_process, cleanup_context);
    std::cerr << "Starting continous polling thread" << std::endl;
    std::thread continousPollingThread(continous_polling, std::ref(queue));

    // TODO : add api routes
    //  HTTPS server setup
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
            // add every route here :
            std::stringstream ss;
            ss << "HTTP/1.1 200 OK\r\n";
            ss << "Content-Type: application/json\r\n";
            ss << "Connection: close\r\n";
            ss << "\r\n";

            if (headers["Path"] == "/temperature" && headers["Method"] == "GET")
            {
                // return temperature as json
                nlohmann::json j;
                j["value"] = "22.5";
                j["unit"] = "°C";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/humidity" && headers["Method"] == "GET")
            {
                // return humidity as json
                nlohmann::json j;
                j["value"] = "50"; // dummy value
                j["unit"] = "%";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/pressure" && headers["Method"] == "GET")
            {
                // return pressure as json
                nlohmann::json j;
                j["value"] = "1013"; // dummy value
                j["unit"] = "hPa";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/light" && headers["Method"] == "GET")
            {
                // return light as json
                nlohmann::json j;
                j["value"] = "100"; // dummy value
                j["unit"] = "lux";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/proximity" && headers["Method"] == "GET")
            {
                // return proximity as json
                nlohmann::json j;
                j["value"] = "10"; // dummy value
                j["unit"] = "proximity";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/gas" && headers["Method"] == "GET")
            {
                // return gas as json :
                nlohmann::json j;
                j["oxidising"] = "0.00"; // dummy value
                j["reducing"] = "0.00";  // dummy value
                j["nh3"] = "0.00";       // dummy value
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                j["unit"] = "Ω";
                ss << j.dump();
            }
            else if (headers["Path"] == "/display" && headers["Method"] == "POST")
            {
                // body is in headers["Body"]
                // TODO : add the display task to the redis queue !
            }
            else if (headers["Path"] == "/structure" && headers["Method"] == "GET")
            {
                // TODO : return json structure file !
            }

            // TODO : /display
            // TODO : /structure
            // TODO : /job/<job_id>
            // TODO : /recordings/<filename>
            // TODO : /mic/record (GET)
            else
            {
                ss << "HTTP/1.1 404 Not Found\r\n";
                ss << "Content-Type: application/json\r\n";
                ss << "Connection: close\r\n";
                ss << "\r\n";
                nlohmann::json j;
                j["error"] = "Route not found";
                ss << j.dump();
            }

            server.sendSocket(ss.str(), clients[i]);
            server.closeSocket(clients[i]); // close the connection !
        }
    }
    // join the threads
    std::cerr << "Joining threads" << std::endl;
    running = false;
    cleanupThread.join();
    continousPollingThread.join();

    return 0;
}
