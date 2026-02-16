/**
 * \file main.cpp
 * \brief Главный файл программы для вычисления медианы CSV файлов
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#define NOMINMAX

#include <windows.h>

#include <chrono>
#include <codecvt>
#include <filesystem>
#include <iostream>
#include <locale>
#include <thread>


#include "argument_parser.hpp"
#include "config_parser.hpp"
#include "data_queue.hpp"
#include "file_streamer.hpp"
#include "logger.hpp"
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
    #ifdef _WIN32
        system("chcp 65001 > nul");
        SetConsoleOutputCP(CP_UTF8);
    #endif

    app::processing::logger::init();

    spdlog::info("Запуск приложения " ANSI_GREEN "csv_median_calculator v1.0.0" ANSI_RESET);
    
    try {
        const auto cli_args = app::cli::parse_arguments(argc, argv);
        
        if (cli_args._show_help) {
            std::cout << app::cli::create_options_description() << std::endl;
            return 0;
        }
        
        const auto config = app::config::parse_configuration(cli_args._variables);
        
        if (!config.is_valid()) {
            return 1;
        }
        
        auto tasks = std::make_shared<app::processing::data_queue>();
        
        if (!std::filesystem::exists(config._output_dir / "median.csv")) {
            spdlog::info("Создание " ANSI_YELLOW "{}" ANSI_RESET, config._output_dir.string() + "/median.csv");
            fs::create_directories(config._output_dir);
        }
        
        const auto output_path = config._output_dir / "median.csv";
        
        auto file_streamer = std::make_shared<app::io::file_streamer>(
            output_path.string()
        );
        auto readers_mgr = std::make_unique<app::io::readers_manager>(tasks, cli_args._streaming_mode);
        auto median_calc = std::make_shared<app::processing::median_calculator>(tasks, config._extra_values_name);
        
        median_calc->set_output_stream(file_streamer);
        
        for (const auto& file : config._csv_files) {
            readers_mgr->add_csv_file(file);
        }
        
        auto calc_thread = median_calc->run_async();
        
        if (cli_args._streaming_mode) {
            std::cout << "Нажмите Enter для остановки..." << std::endl;
            std::cin.get();
            spdlog::info("Остановка...");
            readers_mgr->stop_all();
            readers_mgr->join_all_readers();
        } else {
            readers_mgr->join_all_readers();
            readers_mgr->stop_all();
        }
        
        file_streamer->flush();
        
        std::cout << "======================================================" << std::endl;
        spdlog::info("Обработано строк: " ANSI_GREEN "{}" ANSI_RESET, readers_mgr->total_tasks().load());
        spdlog::info("Записано изменений медианы: " ANSI_GREEN "{}" ANSI_RESET, file_streamer->total_records());
        spdlog::info("Результат сохранен в: " ANSI_YELLOW "{}" ANSI_RESET, output_path.string());
        spdlog::info(ANSI_GREEN "Завершение работы" ANSI_RESET);
        
    } catch (const std::exception& e) {
        spdlog::critical("Ошибка: {}", e.what());
        return 1;
    }
    
    return 0;
}
