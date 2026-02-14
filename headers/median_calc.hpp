#ifndef MEDIAN_CALC_HPP
#define MEDIAN_CALC_HPP

#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <mutex>

#include "data_queue.hpp"
#include "file_stream.hpp"
#include "TDigest.hpp"

using namespace std;

class MedianCalc {
private:
    unique_ptr<TDigest> tdigest;
    shared_ptr<data_queue> tasks;
    shared_ptr<FileStreamer> fileStreamer;
    mutex out_mutex;
public:
    MedianCalc(shared_ptr<data_queue> _tasks);
    ~MedianCalc();

    void set_streamer(shared_ptr<FileStreamer> _fileStreamer);
    void Calc();
};

#endif