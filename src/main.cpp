#include <iostream>

#include "config.hpp"
#include "get_arg.hpp"
#include "data_queue.hpp"
#include "ReadersManager.hpp"
#include "median_calc.hpp"

int main(int argc, char* argv[]) {
    try {

        Config config = parsing(get_args(argc, argv));

        shared_ptr<data_queue> tasks = make_shared<data_queue>();
        unique_ptr<ReadersManager> readers_manager = make_unique<ReadersManager>(tasks);
        
        for (int i = 0; i < config.csv_files.size(); ++i) {
            readers_manager->append_file(config.csv_files[i]);
        }

        shared_ptr<MedianCalc> median_calc = make_shared<MedianCalc>(tasks);
        jthread calc(&MedianCalc::Calc, median_calc.get());

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}