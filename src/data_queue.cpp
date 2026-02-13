#include "data_queue.hpp"

mutex push_mutex;
mutex pop_mutex;

atomic<size_t> data_queue::tasks_count = 0;

data_queue::data_queue() {};

data_queue::~data_queue() {};

void data_queue::push(unique_ptr<Data> task) {
    push_mutex.lock();
    tasks.push(move(task));
    ++tasks_count;
    push_mutex.unlock();
};

unique_ptr<Data> data_queue::pop() {
    pop_mutex.lock();
    unique_ptr<Data> result = move(tasks.front());
    tasks.pop();
    --tasks_count;
    pop_mutex.unlock();
    return result;
}

size_t data_queue::size() {
    return tasks_count;
}