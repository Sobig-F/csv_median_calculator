#include <iostream>
#include <thread>

#include "config.hpp"
#include "get_arg.hpp"
#include "csv_reader.hpp"

int main(int argc, char* argv[]) {
    try {

        Config config = parsing(get_args(argc, argv));
        cout << "Config file: " << config.config_file << endl;
        cout << "Input dir: " << config.input_dir << endl;
        cout << "Output dir: " << config.output_dir << endl;
        
        cout << "CSV filename masks:" << endl;
        for (const auto& mask : config.csv_filename_mask) {
            cout << "  - " << mask << endl;
        }

        vector<unique_ptr<CSVReader>> readers;
        vector<jthread> threads;
        unique_ptr<CSVReader> reader;
        for (int i = 0; i < config.csv_files.size(); ++i) {
            reader = make_unique<CSVReader>(config.csv_files[i]);
            cout << reader->filename << endl;
            threads.emplace_back(&CSVReader::ReadFile, reader.get());
            readers.push_back(move(reader));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}