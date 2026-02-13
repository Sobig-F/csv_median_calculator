#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <fstream>
#include <iostream>


using namespace std;


namespace bip = boost::interprocess;

struct PriceData {
    int64_t receive_ts;
    int64_t exchange_ts;
    double price;
    double quantity;
};

class CSVReader {
private:
    bip::file_mapping mapping;
    bip::mapped_region region;
    const char* data;
    size_t size;
    size_t position;
    size_t last_size;

    void Refresh(size_t position);

public:
    string filename;

    CSVReader(string filename);

    ~CSVReader() {};

    CSVReader(const CSVReader&) = delete;
    CSVReader& operator=(const CSVReader&) = delete;
    
    CSVReader(CSVReader&& other) noexcept;
    CSVReader& operator=(CSVReader&& other) noexcept;

    void ReadFile();

};

#endif