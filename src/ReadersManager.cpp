#include "ReadersManager.hpp"

ReadersManager::ReadersManager(shared_ptr<data_queue> _tasks) {
    readers_count = 0;
    tasks = _tasks;
}

ReadersManager::~ReadersManager() {};

void ReadersManager::append_file(string filename) {
    readers.push_back(make_shared<CSVReader>(filename, tasks));
    threads.emplace_back(&CSVReader::ReadFile, readers.back().get());
    ++readers_count;
}