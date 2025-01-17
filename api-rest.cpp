#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <sstream>
#include <regex>
#include <argp.h>
#include <thread>
#include <mutex>
#include <codecvt>
#include <locale>

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
#define SENSOR_SUPPORT
#define EASY_DEBUG
//  JOBS :
#include "job.hpp"
// MUTEX :
std::mutex mtx;

// Argument parsing definitions
#define PORT 8050 // port to listen on
const char *argp_program_version = "API-REST SERVER, VERSION 0.1";
const char *argp_program_bug_address = "<nolane.de@gmail.com>";
static char doc[] = "This is a C++ implementation of a REST API server";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"port", 'p', "PORT", 0, "Set the port to listen on"},
    {"cert", 'c', "CERT", 0, "Set the certificate path"},
    {"key", 'k', "KEY", 0, "Set the key path"},
    {"recording", 'r', "RECORDING", 0, "Recording Directory"},
    {0, 0, 0, 0, nullptr}};

struct arguments
{
    int port;
    std::filesystem::path certPath;
    std::filesystem::path keyPath;
    std::filesystem::path recordingPath;
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
    case 'r':
        arguments->recordingPath = arg ? std::filesystem::absolute(std::filesystem::path(arg)) : "";
        break;
    case ARGP_KEY_END:
        // check if there are any non opion arguments
        if (arguments->keyPath.empty() || arguments->certPath.empty() || arguments->recordingPath.empty())
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
struct display_pass_data
{
    std::wstring text;
    textLCD *textDraw;
    ST7735 *lcd;
    int speed;
    std::array<int, 3> color;
};
void continous_polling(redisQueue &q)
{
    while (running)
    {
        // mutex lock
        mtx.lock();
        int id = q.startFirstJob(mtx);
        // mutex unlock done in startFirstJob
        if (id >= 0)
        {
            mtx.lock();
            q.finishJob(id);
            mtx.unlock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
            return -1;
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
        return;
    }
    if (reply->type != REDIS_REPLY_ARRAY && reply->type != REDIS_REPLY_NIL)
    {
        std::cerr << "Error: Invalid reply type" << std::endl;
        std::cerr << "Error: " << reply->type << std::endl;
        freeReplyObject(reply);
        return;
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
                return;
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
int displayText(void *arg)
{
    std::cerr << "Displaying text" << std::endl;
    display_pass_data *data = (display_pass_data *)arg;

    std::wstring text = data->text;
    textLCD *textDraw = data->textDraw;
    ST7735 *lcd = data->lcd;
    int speed = data->speed;
    std::array<int, 3> color = data->color;
    // 24bit to 16bit color :
    uint16_t color16 = lcd->color565(color[0], color[1], color[2]);

    std::wcerr << "Text : " << data->text << std::endl;
    lcd->fillScreen(ST7735_BLACK);
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;

    textDraw->textSize(text, &width, &height);
    if (width <= 0)
    {
        std::cerr << "Error: Text width is <= 0" << std::endl;
        return -1;
    }
    for (size_t i = 0; i <= width + lcd->getWidth(); i += speed)
    {
        lcd->fillScreen(ST7735_BLACK);
        textDraw->drawText(text, x + i, y, color16);
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

std::filesystem::path createTempFile(std::filesystem::path directory)
{
    // create a temp file based on the current time
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << directory << "/" << t << ".wav";
    std::filesystem::path tempFile = ss.str();
    return tempFile;
}
int record_audio(void *arg)
{
    std::cerr << "Recording audio" << std::endl;
    audio_pass_data *data = (audio_pass_data *)arg;
    return recordAudio(data->duration, data->outputFile, data->c);
}

int main(int argc, char **argv)
{
#ifdef EASY_DEBUG // DEBUGGING
    if (argc == 1)
    {
        // cheating : adding argument for debug : (easy debugging)
        argc = 7;
        // fixme : find a way to remove those warnings
        char *new_argv[] = {argv[0], "--key", "../raspberrypi-fr.local.key", "--cert", "../raspberrypi-fr.local.crt", "-r", "../recordings"};
        argv = new_argv;
    }
#endif
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
    BME280 bme280 = BME280();
    MICS6814 mics6814 = MICS6814();
    LTR559 ltr559 = LTR559();

    // SETUP LCD SCREEN :
    ST7735 lcd = ST7735("/dev/spidev0.1", "gpiochip0", 8, 10000000, 9, -1, 12, 80, 160); // 80x160 (because its rotated !)
    lcd.init();
    lcd.fillScreen(ST7735_BLACK);

    textLCD textWriter = textLCD("../arial.ttf", 24, &lcd);

    if (bme280.begin() != 0)
    {
        std::cerr << "Error while initializing BME280" << std::endl;
        return -1;
    }
#endif
    // connect to redis :
    redisContext *c = redisConnect("127.0.0.1", 6379);
    redisContext *cleanup_context = redisConnect("127.0.0.1", 6379);  // 2nd connection for cleanup
    redisContext *add_file_cleanup = redisConnect("127.0.0.1", 6379); // 3rd connection for adding files to the cleanup queue
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
    if (add_file_cleanup == NULL || add_file_cleanup->err)
    {
        if (add_file_cleanup)
        {
            std::cerr << "Redis Error: " << add_file_cleanup->errstr << std::endl;
            redisFree(add_file_cleanup);
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
    std::filesystem::path recordingPath = std::filesystem::current_path();

    if (!arguments.keyPath.empty())
    {
        keyPath = arguments.keyPath;
    }

    if (!arguments.certPath.empty())
    {
        certPath = arguments.certPath;
    }
    if (!arguments.recordingPath.empty())
    {
        recordingPath = arguments.recordingPath;
    }

    SocketSecure server(arguments.port, certPath, keyPath);
    server.createSocket();
    server.bindSocket();
    ParseString p("");
    int id = 0;
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
            ss << "Access-Control-Allow-Origin: *\r\n"; // TODO : change this to the actual origin
            ss << "Connection: close\r\n";
            ss << "\r\n";
            // reegex declaration :
            std::regex record_regex(R"(^/recordings/([a-zA-Z0-9]+.wav))");
            std::smatch match;
            std::regex job_regex(R"(^/job/([0-9]+))");
            if (headers["Path"] == "/temperature" && headers["Method"] == "GET")
            {
                // return temperature as json
                nlohmann::json j;
#ifdef SENSOR_SUPPORT
                j["value"] = bme280.readTemp();
#else
                j["value"] = 0;
#endif
                j["unit"] = "°C";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/humidity" && headers["Method"] == "GET")
            {
                // return humidity as json
                nlohmann::json j;
#ifdef SENSOR_SUPPORT
                j["value"] = bme280.readHumidity();
#else
                j["value"] = 0;
#endif
                j["unit"] = "%";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/pressure" && headers["Method"] == "GET")
            {
                // return pressure as json
                nlohmann::json j;
#ifdef SENSOR_SUPPORT
                j["value"] = bme280.readPressure();
#else
                j["value"] = 0;
#endif
                j["unit"] = "hPa";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/light" && headers["Method"] == "GET")
            {
                // return light as json
                nlohmann::json j;
#ifdef SENSOR_SUPPORT
                j["value"] = ltr559.getLux();
#else
                j["value"] = 0;
#endif

                j["unit"] = "lux";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/proximity" && headers["Method"] == "GET")
            {
                // return proximity as json
                nlohmann::json j;
#ifdef SENSOR_SUPPORT
                j["value"] = ltr559.getProximity();
#else
                j["value"] = 0;
#endif
                j["unit"] = "proximity";
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                ss << j.dump();
            }
            else if (headers["Path"] == "/gas" && headers["Method"] == "GET")
            {
                // return gas as json :
                nlohmann::json j;
#ifdef SENSOR_SUPPORT
                j["oxidising"] = mics6814.readOxydising();
                j["reducing"] = mics6814.readReducing();
                j["nh3"] = mics6814.readNH3();
#else
                j["oxidising"] = 0;
                j["reducing"] = 0;
                j["nh3"] = 0;
#endif
                j["timestamp"] = std::time(nullptr);
                j["frequency"] = "1"; // TODO : complete this with the real frequency
                j["unit"] = "Ω";
                ss << j.dump();
            }
            else if (headers["Path"] == "/display" && headers["Method"] == "POST")
            {
                std::string body = headers["Body"];
                std::string text;
                std::array<int, 3> color = {255, 255, 255};
                int speed = 1;
                // convert it to json
                try
                {
                    nlohmann::json j = nlohmann::json::parse(body);
                    text = j["message"];
                    speed = j["speed"];

                    color[0] = j["color"][0];
                    color[1] = j["color"][1];
                    color[2] = j["color"][2];
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                    ss = std::stringstream(); // reset the stream !
                    ss << "HTTP/1.1 400 Bad Request\r\n";
                    ss << "Content-Type: application/json\r\n";
                    ss << "Connection: close\r\n";
                    ss << "\r\n";
                    nlohmann::json j;
                    j["error"] = "Invalid JSON";
                    ss << j.dump();
                    server.sendSocket(ss.str(), clients[i]);
                    server.closeSocket(clients[i]); // close the connection !
                    continue;
                }
                if (speed <= 0) // input validation
                {
                    speed = 1;
                }
                if (color[0] < 0 || color[0] > 255 || color[1] < 0 || color[1] > 255 || color[2] < 0 || color[2] > 255)
                {
                    color = {255, 255, 255};
                }
#ifdef SENSOR_SUPPORT
                // convert text to utf32 std::wstring
                std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
                std::wstring wstr = convert.from_bytes(text);

                job display(displayText, (void *)new display_pass_data{wstr, &textWriter, &lcd, speed, color}, id);
                id++;
                std::lock_guard<std::mutex> lock(mtx);
                queue.addJob(display);
                std::cerr << "Launching Text Display !" << std::endl;
#else
                std::cerr << "Display not supported" << std::endl;
#endif
                // body is in headers["Body"]
                // respond with json :
                // TODO !
            }
            else if (headers["Path"] == "/structure" && headers["Method"] == "GET")
            {
                // TODO : return json structure file !
                try
                {
                    // send the headers
                    server.sendSocket(ss.str(), clients[i]);
                    // send the file
                    std::filesystem::directory_entry file("../structure.json");
                    server.sendFile(file, clients[i]);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                    ss = std::stringstream(); // reset the stream !
                    ss << "HTTP/1.1 404 Not Found\r\n";
                    ss << "Content-Type: application/json\r\n";
                    ss << "Connection: close\r\n";
                    ss << "\r\n";
                    nlohmann::json j;
                    j["error"] = "File not found";
                    ss << j.dump();
                }
            }
            else if (std::regex_match(headers["Path"], match, record_regex) && headers["Method"] == "GET")
            {
                // extract the filename from the path
                std::string filename = match[1].str();
                std::filesystem::path filePath = recordingPath / filename;
                // check if the file exists
                if (std::filesystem::exists(filePath))
                {
                    // file exists
                    // TODO: handle the GET request for recordings/<filename>
                    ss = std::stringstream(); // reset the stream !
                    ss << "HTTP/1.1 200 OK\r\n";
                    ss << "Content-Type: audio/wav\r\n";
                    ss << "Transfer-Encoding: chunked\r\n";
                    ss << "Connection: close\r\n";
                    ss << "Access-Control-Allow-Origin: *\r\n"; // TODO : change this to the actual origin
                    ss << "\r\n";

                    // send the headers
                    server.sendSocket(ss.str(), clients[i]);
                    // send the file
                    std::filesystem::directory_entry file(filePath);

                    server.sendFile(file, clients[i]);
                    ss = std::stringstream(); // reset the stream !
                }
                else
                {
                    // file does not exist
                    ss = std::stringstream(); // reset the stream !
                    ss << "HTTP/1.1 404 Not Found\r\n";
                    ss << "Content-Type: application/json\r\n";
                    ss << "Connection: close\r\n";
                    ss << "\r\n";
                    nlohmann::json j;
                    j["error"] = "File not found";
                    ss << j.dump();
                }
                // TODO: handle the GET request for recordings/<filename>
            }
            // JOB ID:
            // PATH : /job/<job_id>
            else if (headers["Method"] == "GET" && std::regex_match(headers["Path"], match, job_regex))
            {
                // extract the job id from the path
                int job_id = std::stoi(match[1].str());
                // get the job status
                // job_status status = queue.getJobStatus(job_id);
                // create a json response
                nlohmann::json j;
                j["job_id"] = job_id;
                j["status"] = queue.getJobStatus(job_id);
                ss << j.dump();
            }
            else if (headers["Method"] == "GET" && headers["Path"] == "/mic/record")
            {
                // TODO
                std::cerr << "Recording audio for 5 seconds" << std::endl;
                std::string filename = createTempFile(recordingPath);
                job record(record_audio, (void *)new audio_pass_data{5, filename, add_file_cleanup}, id);
                std::cerr << "Job ID: " << record.get_id() << std::endl;
                id += 1; // increment the id
                // lock the mutex
                std::lock_guard<std::mutex> lock(mtx);
                queue.addJob(record);
                std::cerr << "Job added to queue" << std::endl;
                nlohmann::json j;
                std::string filename_without_path = std::filesystem::path(filename).filename();
                j["job_id"] = id;
                j["filename"] = filename_without_path;
                j["validity"] = 1800;
                j["unit"] = "s";
                std::cerr << "Sending JSON" << std::endl;
                ss << j.dump();
                // mutex will now be unlocked
            }

            // TODO : /display
            // TODO : /structure
            // TODO : /job/<job_id>
            else if (headers["Method"] == "OPTIONS")
            {
                // OPTION request - CORS REQUEST
                // See https://developer.mozilla.org/fr/docs/Web/HTTP/Methods/OPTIONS
                if (headers["Path"] == "")
                {
                    ss = std::stringstream(); // reset the stream !
                    ss << "HTTP/1.1 200 OK\r\n";
                    ss << "Allow: GET, POST, OPTIONS\r\n";
                }
                else
                {
                    ss = std::stringstream(); // reset the stream !
                    ss << "HTTP/1.1 200 OK\r\n";
                    ss << "Access-Control-Allow-Origin: *\r\n"; // TODO : change this to the actual origin
                    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
                    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
                    ss << "Connection: close\r\n";
                }
                ss << "Content-Length: 0\r\n";
                ss << "\r\n";
            }
            else
            {
                ss = std::stringstream(); // reset the stream !
                ss << "HTTP/1.1 404 Not Found\r\n";
                ss << "Content-Type: application/json\r\n";
                ss << "Connection: close\r\n";
                ss << "\r\n";
                nlohmann::json j;
                j["error"] = "Route not found";
                ss << j.dump();
            }
            if (ss.str().empty())
            {
                std::cerr << "Error: Empty response" << std::endl;
                continue;
            }
            else
            {
                server.sendSocket(ss.str(), clients[i]);
            }
            server.closeSocket(clients[i]); // close the connection !
        }
    }
    // join the threads
    std::cerr << "Joining threads" << std::endl;
    running = false;
    cleanupThread.join();
    continousPollingThread.join();
    // free the redis context
    redisFree(c);
    redisFree(cleanup_context);
    redisFree(add_file_cleanup);

    return 0;
}
