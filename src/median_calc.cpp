#include "median_calc.hpp"

const double EPSILON = 1e-10;

MedianCalc::MedianCalc(shared_ptr<data_queue> _tasks) {
    tasks = _tasks;
}

MedianCalc::~MedianCalc() {}

void MedianCalc::Calc() {
    unique_ptr<Data> task;
    double old_median = -1;
    double now_median;

    while (true) {
        if (tasks->size() > 0) {
            task = tasks->pop();
            acc(task->price);

            if (boost::accumulators::count(acc) > 5) {
                now_median = median(acc);
                if (fabs(now_median - old_median) > EPSILON) {
                    fileStreamer_mutex.lock();
                    // cout << "New median: " << fixed << setprecision(8) << median(acc) << " Time: " << task->receive_ts << endl;
                    (*fileStreamer) << task->receive_ts << ";" << now_median << "\n";
                    fileStreamer_mutex.unlock();
                    old_median = now_median;
                }

            }

        } else {
            this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void MedianCalc::set_streamer(shared_ptr<FileStreamer> _fileStreamer) {
    fileStreamer = _fileStreamer;
}