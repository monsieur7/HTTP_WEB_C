#include "redisQueue.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include <linux/unistd.h>    /* for _syscallX macros/related stuff */
#include <linux/kernel.h>    /* for struct sysinfo */
#include <sys/sysinfo.h>     /* for sysinfo */
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
int test_func(void *arg)
{
    std::string *s = (std::string *)arg;
    std::cout << *s << std::endl;
    return 0;
}

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
int main()
{
    redisContext *c = redisConnect("127.0.0.1", 6379);
    redisContext *cleanup_context = redisConnect("127.0.0.1", 6379); // 2nd connection for cleanup
    if (c == NULL || c->err)
    {
        if (c)
        {
            std::cerr << "Error: " << c->errstr << std::endl;
            redisFree(c);
        }
        else
        {
            std::cerr << "Error: Could not allocate redis context" << std::endl;
        }
        exit(1);
    }
    redisQueue q(c);
    job j(test_func, new std::string("Hello, World!"), 1);
    std::filesystem::path tempFile = createTempFile();
    job audio(record_audio, new audio_pass_data{5, tempFile, c}, 2);
    q.addJob(audio);
    q.addJob(j);
    q.printJobs();
    // start continous polling as a background thread
    std::thread t(continous_polling, std::ref(q));
    std::thread cleanup(cleanupTempFiles_process, std::ref(cleanup_context));

    // https://stackoverflow.com/questions/1540627/what-api-do-i-call-to-get-the-system-uptime
    for (int i = 0; i < 10; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // print sysinfo
        struct sysinfo info;
        sysinfo(&info);
        std::cout << "Uptime: " << info.uptime << std::endl;
        std::cout << "Total RAM: " << std::setprecision(2) << ((float)info.totalram * info.mem_unit) / (1024 * 1024 * 1024) << std::endl;
        std::cout << "Free RAM: " << std::setprecision(2) << ((float)info.freeram * info.mem_unit) / (1024 * 1024 * 1024) << std::endl;
        std::cout << "Number of processes: " << info.procs << std::endl;
        std::cout << "Load average: " << std::setprecision(2) << info.loads[0] / 65536.0f << std::endl;
    }
    running = false;
    t.join();
    cleanup.join();
}