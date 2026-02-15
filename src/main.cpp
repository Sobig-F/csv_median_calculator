/**
 * \file main.cpp
 * \brief Главный файл программы для вычисления медианы CSV файлов
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include <iostream>
#include <filesystem>
#include <thread>

#include "argument_parser.hpp"
#include "config_parser.hpp"
#include "data_queue.hpp"
#include "file_streamer.hpp"
#include "median_calculator.hpp"
#include "readers_manager.hpp"

namespace fs = std::filesystem;

/**
 * \brief Точка входа в программу
 * \param argc количество аргументов командной строки
 * \param argv массив аргументов командной строки
 * \return 0 при успешном выполнении, иначе код ошибки
 */
int main(int argc, char* argv[]) {
    try {
        const auto cli_args = app::cli::parse_arguments(argc, argv);
        
        if (cli_args._show_help) {
            std::cout << app::cli::create_options_description() << std::endl;
            return 0;
        }
        
        const auto config = app::config::parse_configuration(cli_args._variables);
        
        if (!config.is_valid()) {
            throw std::runtime_error{
                "Invalid configuration: input/output directories not set or invalid"
            };
        }
        
        auto tasks = std::make_shared<app::processing::data_queue>();
        
        fs::create_directories(config._output_dir);
        
        const auto output_path = config._output_dir / "median.csv";
        
        auto file_streamer = std::make_shared<app::io::file_streamer>(
            output_path.string()
        );
        auto readers_mgr = std::make_unique<app::io::readers_manager>(tasks);
        auto median_calc = std::make_shared<app::processing::median_calculator>(tasks);
        
        median_calc->set_output_stream(file_streamer);
        
        for (const auto& file : config._csv_files) {
            std::cout << "Adding file: " << file << std::endl;
            readers_mgr->add_csv_file(file);
        }
        
        std::cout << "Started " << readers_mgr->reader_count() 
                  << " readers. Processing..." << std::endl;
        
        auto calc_thread = median_calc->run_async();
        
        std::cout << "Press Enter to stop..." << std::endl;
        std::cin.get();
        
        std::cout << "Shutting down..." << std::endl;
        median_calc->stop();
        readers_mgr->stop_all();
        tasks->stop();
        
        readers_mgr->join_all();
        if (calc_thread.joinable()) {
            calc_thread.join();
        }
        
        std::cout << "Done. Results saved to: " << output_path << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
