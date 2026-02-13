#include "csv_reader.hpp"

#include <thread>
#include <chrono> 

using namespace std::chrono;
using namespace std::this_thread;

CSVReader::CSVReader(string filename) 
    :   mapping(filename.c_str(), bip::read_only),
        region(mapping, bip::read_only),
        data(static_cast<const char*>(region.get_address())),
        size(region.get_size()),
        position(0),
        filename(filename) {}

CSVReader::CSVReader(CSVReader&& other) noexcept
    :   mapping(std::move(other.mapping)),
        region(std::move(other.region)),
        data(other.data),
        size(other.size),
        position(other.position),
        filename(std::move(other.filename)) {
    
        other.data = nullptr;
        other.size = 0;
        other.position = 0;
}

CSVReader& CSVReader::operator=(CSVReader&& other) noexcept {
    if (this != &other) {
        mapping = std::move(other.mapping);
        region = std::move(other.region);
        data = other.data;
        size = other.size;
        position = other.position;
        filename = std::move(other.filename);
        
        other.data = nullptr;
        other.size = 0;
        other.position = 0;
    }
    return *this;
}

void CSVReader::ReadFile() {
    string line;
    while (true)
    {
        try
        {
            while (position < size && data[position] != '\n') {
                line += data[position++];
            }
            
            if (position > size) {
                last_size = size;
                Refresh(position);
                
                if (size > last_size) {
                    last_size = size;
                } else {
                    sleep_for(milliseconds(100));
                }
            } else {
                if (line.size() > 0) {
                    // cout << "--" << filename << "--" << line << endl;
                    line.clear();
                }
                ++position;
            }


        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            break;
        }
    }
}

void CSVReader::Refresh(size_t pos) {
    mapping = bip::file_mapping(filename.c_str(), bip::read_only);
    region = bip::mapped_region(mapping, bip::read_only);
    data = (static_cast<const char*>(region.get_address()));
    size = (region.get_size());
    position = pos;
}