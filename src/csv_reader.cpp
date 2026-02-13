#include "csv_reader.hpp"

// #include <thread>
// #include <chrono>

#include "types.hpp"
#include "data_queue.hpp"

using namespace std::chrono;
using namespace std::this_thread;

std::mutex cout_mutex;

unique_ptr<Data> take_price_and_time(const string& line) {
    vector<string> split_line;
    unique_ptr<Data> result;
    
    boost::split(split_line, line, boost::is_any_of(";"));

    return make_unique<Data>(
            boost::lexical_cast<int_fast64_t>(split_line[0]),
            boost::lexical_cast<double>(split_line[2])
    );
}

CSVReader::CSVReader(string filename, shared_ptr<data_queue> _tasks) 
    :   mapping(filename.c_str(), bip::read_only),
        region(mapping, bip::read_only),
        data(static_cast<const char*>(region.get_address())),
        size(region.get_size()),
        position(0),
        filename(filename),
        tasks(_tasks) {}

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
    unique_ptr<Data> csv_data;

    while (position < size && data[position] != '\n') {
        ++position;
    }
    ++position;
    
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
                    csv_data = take_price_and_time(line);
                    // cout_mutex.lock();
                    // cout << filename << ": " << csv_data.get()->receive_ts << "ms / " << csv_data.get()->price << "$" << endl;
                    // cout_mutex.unlock();
                    tasks.get()->push(move(csv_data));
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