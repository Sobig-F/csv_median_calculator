#ifndef READERS_MANAGER_HPP
#define READERS_MANAGER_HPP

#include <string>
#include <vector>
#include <thread>

#include "csv_reader.hpp"
#include "data_queue.hpp"

using namespace std;

class ReadersManager {
    vector<shared_ptr<CSVReader>> readers;
    vector<jthread> threads;
    size_t readers_count;
    shared_ptr<data_queue> tasks;

public:
    ReadersManager(shared_ptr<data_queue> _tasks);
    ~ReadersManager();
    void append_file(string filename);
};

#endif