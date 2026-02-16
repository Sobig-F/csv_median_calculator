/**
 * \file config_parser.cpp
 * \brief Реализация парсинга конфигурации
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "config_parser.hpp"

#include <windows.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <boost/regex.hpp>
#include <toml++/toml.hpp>

#include "logger.hpp"

namespace app::config {

namespace {
    /**
     * \brief Извлекает маски файлов из TOML массива
     */
    [[nodiscard]] string_vector extract_filename_masks(const toml::table& tbl_) {
        string_vector result;
        
        if (auto arr = tbl_["main"]["filename_mask"].as_array()) {
            for (auto& elem : *arr) {
                if (auto str = elem.value<string>()) {
                    result.push_back(*str);
                }
            }
        }
        
        if (result.empty()) {
            result.push_back("");
        }
        return result;
    }
    
    /**
     * \brief Находит CSV файлы по маскам в директории
     */
    [[nodiscard]] string_vector find_csv_files(
        const path& dir_,
        const string_vector& masks_)
    {
        string_vector result;
        
        if (!boost::filesystem::exists(dir_.string())) {
            return result;
        }
        
        for (const auto& conf_mask : masks_) {
            const boost::regex mask{".*" + conf_mask + ".*\\.csv$", 
                                   boost::regex::icase};
            
            for (const auto& entry : boost::filesystem::directory_iterator{dir_.string()}) {
                if (boost::filesystem::is_regular_file(entry.path())) {
                    const auto filename = entry.path().filename().string();
                    
                    if (boost::regex_match(filename, mask)) {
                        result.push_back(entry.path().string());
                        spdlog::info("    --" ANSI_MAGENTA "{}" ANSI_RESET, filename);
                    }
                }
            }
        }
        
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        
        return result;
    }
}  // unnamed namespace

bool parsing_result::is_valid() const noexcept {
    return !_input_dir.empty() && 
           !_output_dir.empty() && 
           boost::filesystem::exists(_input_dir.string());
}

parsing_result parse_configuration(
    const boost::program_options::variables_map& vm_) noexcept(false)
{
    parsing_result config;
    string config_path;
    if (!vm_.contains("config")) {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        config_path = path(buffer).parent_path().string() + "/config.toml";
    } else {
        config_path = vm_["config"].as<string>();
    }
    
    if (!boost::filesystem::exists(config_path)) {
        spdlog::critical("Файл конфигурации не найден");
        return config;
    }
    spdlog::info("Чтение файла конфигурации: " ANSI_YELLOW "{}" ANSI_RESET, config_path);
    
    config._config_file = config_path;
    
    try {
        const auto toml_file = toml::parse_file(config._config_file.string());
        
        // Валидация секции [main]
        if (!toml_file.as_table()->contains("main")) {
            spdlog::critical("Отсутствует секция [main]");
            return config;
        }
        const auto& main_table = toml_file["main"];
        
        // Валидация параметра input
        if (!main_table.as_table()->contains("input")) {
            spdlog::critical("Отсутствует обязательный ключ input");
            return config;
        }
        config._input_dir = std::filesystem::absolute(
            string(main_table["input"].value_or(""))
        ).string();
         
        // Определение output директории
        if (!main_table.as_table()->contains("output")) {
            spdlog::warn("Не обнаружен ключ " ANSI_BLUE "output" ANSI_RESET ", будет создано в текущей директории");
            char buffer[MAX_PATH];
            GetModuleFileNameA(NULL, buffer, MAX_PATH);
            config._output_dir = path(buffer).parent_path().string() + "/output";
        } else {
            config._output_dir = std::filesystem::absolute(
                string(main_table["output"].value_or(""))
            ).string();
        }
        
        spdlog::info("Входная директория: " ANSI_YELLOW "{}" ANSI_RESET, config._input_dir.string());
        spdlog::info("Выходная директория: " ANSI_YELLOW "{}" ANSI_RESET, config._output_dir.string());
        
        config._csv_filename_mask = extract_filename_masks(toml_file);
        
        if (!config._input_dir.empty()) {
            spdlog::info("Поиск " ANSI_MAGENTA "*.csv" ANSI_RESET " файлов...");
            config._csv_files = find_csv_files(
                config._input_dir,
                config._csv_filename_mask
            );
            spdlog::info("Найдено файлов: " ANSI_GREEN "{}" ANSI_RESET, config._csv_files.size());
        } else {
            spdlog::critical("Директория " ANSI_YELLOW "{}" ANSI_RESET " пуста", config._input_dir.string());
            return config;
        }
        
    } catch (const toml::parse_error& err_) {
        throw std::runtime_error{
            "Failed to parse TOML config: " + std::string{err_.description()}
        };
    }
    
    return config;
}

}  // namespace app::config
