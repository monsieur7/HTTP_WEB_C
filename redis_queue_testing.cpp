#include "redisQueue.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include <linux/unistd.h> /* for _syscallX macros/related stuff */
#include <linux/kernel.h> /* for struct sysinfo */
#include <sys/sysinfo.h>  /* for sysinfo */
std::atomic<bool> running(true);
struct audio_pass_data
{
    int duration;
    std::filesystem::path outputFile;
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

int recordAudio(int duration, const std::filesystem::path &outputFile)
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
        return 0;
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
    // get file name :

    std::filesystem::path filePath = std::filesystem::read_symlink("/proc/self/fd/" + std::to_string(fd));
    // close the file descriptor
    close(fd);
    return filePath;
}
int record_audio(void *arg)
{
    audio_pass_data *data = (audio_pass_data *)arg;
    return recordAudio(data->duration, data->outputFile);
}
int main()
{
    redisContext *c = redisConnect("127.0.0.1", 6379);
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
    job audio(record_audio, new audio_pass_data{5, tempFile}, 2);
    q.addJob(audio);
    q.addJob(j);
    q.printJobs();
    // start continous polling as a background thread
    std::thread t(continous_polling, std::ref(q));
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
}