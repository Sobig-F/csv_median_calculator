#ifndef DATA_QUEUE_HPP
#define DATA_QUEUE_HPP

#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

#include "types.hpp"

using namespace std;

class data_queue {
private:
    queue<unique_ptr<Data>> tasks;
    static atomic<size_t> tasks_count;

public:
    data_queue();
    ~data_queue();

    data_queue(const data_queue&) = delete;
    data_queue& operator=(const data_queue&) = delete;

    void push(unique_ptr<Data> task);
    unique_ptr<Data> pop();
    size_t size();
};

#endif