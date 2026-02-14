#ifndef MEDIAN_CALC_HPP
#define MEDIAN_CALC_HPP

#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <mutex>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

#include "data_queue.hpp"
#include "file_stream.hpp"

using namespace boost::accumulators;
using namespace std;

class MedianCalc {
private:
    accumulator_set<double, features<tag::median(with_p_square_quantile)> > acc;
    shared_ptr<data_queue> tasks;
    shared_ptr<FileStreamer> fileStreamer;
    mutex fileStreamer_mutex;
public:
    MedianCalc(shared_ptr<data_queue> _tasks);
    ~MedianCalc();

    void set_streamer(shared_ptr<FileStreamer> _fileStreamer);
    void Calc();
};

#endif