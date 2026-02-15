/**
 * \file config_parser.cpp
 * \brief Реализация парсинга конфигурации
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "config_parser.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <boost/regex.hpp>
#include <toml++/toml.hpp>

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
    const boost::program_options::variables_map& vm_)
{
    parsing_result config;
    
    if (!vm_.contains("config")) {
        throw std::runtime_error{"Missing required argument: --config"};
    }
    
    const auto config_path = vm_["config"].as<string>();
    if (!boost::filesystem::exists(config_path)) {
        throw std::runtime_error{
            "Configuration file not found: " + config_path
        };
    }
    
    config._config_file = config_path;
    
    try {
        const auto toml_file = toml::parse_file(config._config_file.string());
        const auto& main_table = toml_file["main"];
        
        config._input_dir = string(main_table["input"].value_or(""));
        config._output_dir = string(main_table["output"].value_or(""));
        config._csv_filename_mask = extract_filename_masks(toml_file);
        
        if (!config._input_dir.empty()) {
            config._csv_files = find_csv_files(
                config._input_dir,
                config._csv_filename_mask
            );
        }
        
    } catch (const toml::parse_error& err_) {
        throw std::runtime_error{
            "Failed to parse TOML config: " + std::string{err_.description()}
        };
    }
    
    return config;
}

}  // namespace app::config
