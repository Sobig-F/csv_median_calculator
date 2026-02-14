#ifndef FILE_STREAM_HPP
#define FILE_STREAM_HPP

#include <string>
#include <fstream>
#include <filesystem>

#include <boost/lexical_cast.hpp>

#include "types.hpp"

using namespace std;

class FileStreamer {
private:
    ofstream file_desc;
    unique_ptr<pair<int_fast64_t, double>> queue;
    bool isEmpty;
public:
    FileStreamer(string filename);
    ~FileStreamer();

    template<typename T>
    FileStreamer& operator<<(const T& value) {
        if (isEmpty) {
            file_desc << "receive_ts;median" << endl;
            isEmpty = false;
        }

        file_desc << boost::lexical_cast<string>(value);
        file_desc.flush();

        return *this;
    }
};

#endif