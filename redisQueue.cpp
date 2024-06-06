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

void redisQueue::startFirstJob()
{
    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "LPOP jobs");
    if (reply->type == REDIS_REPLY_NIL)
    {
        freeReplyObject(reply);
        return;
    }
    if (!(reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_NIL || reply->type == REDIS_REPLY_INTEGER))
    {
        std::cerr << "Error: Invalid reply type in Redis Queue" << std::endl;
        std::cerr << "Error: " << reply->type << std::endl;
        freeReplyObject(reply);
        return;
    }
    if (reply->str == NULL || reply->type == REDIS_REPLY_NIL)
    {
        freeReplyObject(reply);
        return;
    }
    int id = strtol(reply->str, NULL, 10);
    freeReplyObject(reply);
    std::cerr << "Starting job with ID: " << id << std::endl;
    startJob(id);
}