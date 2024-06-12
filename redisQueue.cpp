#include "redisQueue.hpp"
redisQueue::redisQueue(redisContext *r)
{
    redis = r;
    connect();
    // clear queue :
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "DEL jobs");
    freeReplyObject(reply);
}
redisQueue::~redisQueue()
{
    disconnect();
}
void redisQueue::connect()
{
    if (redis == NULL || redis->err)
    {
        std::cerr << "Error: " << redis->errstr << std::endl;
        exit(1);
    }
}
void redisQueue::disconnect()
{
    redisFree(redis);
}
void redisQueue::addJob(job j)
{
    enqueue(j);
    curr_id++;
}

void redisQueue::printJobs()
{
    for (auto &j : jobs)
    {
        std::cout << "Job ID: " << j.first << std::endl;
    }
}
void redisQueue::getJobs()
{
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "LRANGE jobs 0 -1");
    for (size_t i = 0; i < reply->elements; i++)
    {
        int id = strtol(reply->element[i]->str, NULL, 10);
        job j = jobs[id];
        std::cout << "Job ID: " << j.get_id() << std::endl;
    }
    freeReplyObject(reply);
}
void redisQueue::getJob(int id)
{
    job j = jobs[id];
    std::cout << "Job ID: " << j.get_id() << std::endl;
}
void redisQueue::enqueue(job j)
{
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "LPUSH jobs %d", j.get_id());
    freeReplyObject(reply);
    jobs[j.get_id()] = j;
}
void redisQueue::dequeue()
{
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "RPOP jobs");
    freeReplyObject(reply);
}
void redisQueue::removeJob(int id)
{
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "LREM jobs 1 %d", id);
    freeReplyObject(reply);
    jobs.erase(id);
}
void redisQueue::updateJob(int id, job j)
{
    removeJob(id);
    enqueue(j);
}
void redisQueue::startJob(int id)
{
    job j = jobs[id];
    j.run();
}

int redisQueue::startFirstJob(std::mutex &m)
{
    redisReply *reply;
    // using LMOVE :
    reply = (redisReply *)redisCommand(redis, "LMOVE jobs jobs_temp RIGHT LEFT");
    if (reply->type == REDIS_REPLY_NIL)
    {
        std::cerr << "No jobs in queue" << std::endl;
        freeReplyObject(reply);
        m.unlock();
        return -1;
    }
    if (reply->type != REDIS_REPLY_STRING)
    {
        std::cerr << "Error moving job" << std::endl;
        freeReplyObject(reply);
        m.unlock();
        return -1;
    }

    int id = strtol(reply->str, NULL, 10);
    freeReplyObject(reply);
    std::cerr << "Starting job with ID: " << id << std::endl;
    m.unlock();
    startJob(id);
    return id;
}
void redisQueue::finishJob(int id)
{

    // remove job from jobs_temp :
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "LREM jobs_temp 1 %d", id);
    if (reply->integer == 0)
    {
        std::cerr << "Job not found in jobs_temp" << std::endl;
        freeReplyObject(reply);
        return;
    }
    freeReplyObject(reply);
    return;
}

job_status redisQueue::getJobStatus(int id)
{
    if (id > this->curr_id || id < 0)
    {
        return ERROR;
    }
    // test if job is is in jobs :
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "LRANGE jobs 0 -1");
    for (size_t i = 0; i < reply->elements; i++)
    {
        int job_id = strtol(reply->element[i]->str, NULL, 10);
        if (job_id == id)
        {
            freeReplyObject(reply);
            return PENDING;
        }
    }
    freeReplyObject(reply);
    // test if job is in jobs_temp :
    reply = (redisReply *)redisCommand(redis, "LRANGE jobs_temp 0 -1");
    for (size_t i = 0; i < reply->elements; i++)
    {
        int job_id = strtol(reply->element[i]->str, NULL, 10);
        if (job_id == id)
        {
            freeReplyObject(reply);
            return RUNNING;
        }
    }
    freeReplyObject(reply);
    return FINISHED;
}