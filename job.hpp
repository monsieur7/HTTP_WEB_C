#pragma once
#include <cstddef>
class job
{
public:
private:
    int (*job_func)(void *arg);
    void *arg;
    int id;

public:
    job(int (*job)(void *arg), void *arg, int id);
    job();
    int run();
    void set_id(int id);
    int get_id();
};
