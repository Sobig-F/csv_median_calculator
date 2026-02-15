/**
 * \file config_parser.hpp
 * \brief Конфигурация приложения для обработки CSV файлов
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <filesystem>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace app::config {

using path = std::filesystem::path;
using string = std::string;
using string_vector = std::vector<std::string>;

struct parsing_result {
    path _input_dir;
    path _output_dir;
    path _config_file;
    string_vector _csv_files;
    string_vector _csv_filename_mask;
    
    /**
     * \brief Проверяет, валидна ли конфигурация
     * \return true если все обязательные поля заполнены
     */
    [[nodiscard]] bool is_valid() const noexcept;
};

/**
 * \brief Парсит аргументы командной строки в конфигурацию
 * \param vm_ переменные после парсинга program_options
 * \return структура с конфигурацией
 * \throws std::runtime_error при ошибках парсинга
 */
[[nodiscard]] parsing_result parse_configuration(
    const boost::program_options::variables_map& vm_);

}  // namespace app::config

#endif  // CONFIG_PARSER_HPP