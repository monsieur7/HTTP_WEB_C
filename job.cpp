#include "job.hpp"

job::job(int (*job)(void *arg), void *arg, int id)
{
    this->job_func = job;
    this->arg = arg;
    this->id = id;
}
job::job()
{
    this->job_func = nullptr;
    this->arg = NULL;
    this->id = 0;
}

int job::run()
{
    return job_func(arg);
}

void job::set_id(int id)
{
    this->id = id;
}
int job::get_id()
{
    return id;
}