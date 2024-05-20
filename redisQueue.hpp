#include <iostream>
#include <string>
#include <map>
#include <hiredis/hiredis.h>
#include "job.hpp"
#pragma once
class redisQueue
{
private:
    redisContext *redis;
    int curr_id = 0;
    std::map<int, job> jobs;
    void connect();
    void disconnect();
    void enqueue(job j);
    void dequeue();

public:
    redisQueue(redisContext *r);
    ~redisQueue();
    void addJob(job j);
    void removeJob(int id);
    void updateJob(int id, job j);
    void printJobs();
    void getJobs();
    void getJob(int id);
    void startJob(int id);
    void startFirstJob();
};