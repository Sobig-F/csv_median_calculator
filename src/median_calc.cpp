#include "median_calc.hpp"

const double EPSILON = 1e-10;

MedianCalc::MedianCalc(shared_ptr<data_queue> _tasks)
    :   tdigest(make_unique<TDigest>(25)) 
{
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
            tdigest->add(task->price);
            now_median = tdigest->median();
            if (fabs(now_median - old_median) > EPSILON) {
                if (fileStreamer) {
                    out_mutex.lock();
                    (*fileStreamer) << task->receive_ts << ";" << now_median << "\n";
                    out_mutex.unlock();
                } else {
                    out_mutex.lock();
                    cout << std::fixed << std::setprecision(8) << "receive_ts: " << task->receive_ts << " / median: " << now_median << endl;
                    out_mutex.unlock();
                }
                old_median = now_median;
            }

        } else {
            this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void MedianCalc::set_streamer(shared_ptr<FileStreamer> _fileStreamer) {
    fileStreamer = _fileStreamer;
}