/**
 * \file argument_parser.hpp
 * \brief Парсинг аргументов командной строки
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <string>
#include <optional>

#include <boost/program_options.hpp>

namespace app::cli {

/**
 * \brief Результат парсинга аргументов командной строки
 */
struct parsing_result {
    bool _streaming_mode{false};
    bool _show_help{false};
    std::string _config_file{"config.toml"};
    boost::program_options::variables_map _variables;
};

/**
 * \brief Парсит аргументы командной строки
 * \param argc_ количество аргументов
 * \param argv_ массив аргументов
 * \return результат парсинга
 * \throws std::invalid_argument при ошибках парсинга
 */
[[nodiscard]] parsing_result parse_arguments(
    int argc_,
    char* argv_[]) noexcept(false);

/**
 * \brief Создаёт описание доступных опций
 * \return объект с описанием опций
 */
[[nodiscard]] boost::program_options::options_description create_options_description() noexcept;

}  // namespace app::cli

#endif  // ARGUMENT_PARSER_HPP