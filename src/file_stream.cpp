#include "file_stream.hpp"

FileStreamer::FileStreamer(string filename) {
    file_desc.open(filename, ios::app);
    isEmpty = filesystem::file_size(filename) == 0;
}

FileStreamer::~FileStreamer() {
    if (file_desc.is_open()) {
        file_desc.close();
    }
}