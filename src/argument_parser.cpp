/**
 * \file argument_parser.cpp
 * \brief Реализация парсинга аргументов командной строки
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "argument_parser.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace app::cli {

namespace {
    constexpr std::string_view CONFIG_OPTION = "config";
    constexpr std::string_view HELP_OPTION = "help";
    constexpr std::string_view DEFAULT_CONFIG = "config.toml";
    constexpr std::string_view STREAMING_MODE = "streaming-mode";
    
    /**
     * \brief Кастомный парсер для флага -cfg
     */
    [[nodiscard]] std::pair<std::string, std::string> parse_custom_flag(
        const std::string& s_) 
    {
        if (s_ == "-cfg") {
            return {std::string{CONFIG_OPTION}, {}};
        }
        
        if (s_.find("-cfg=") == 0) {
            return {std::string{CONFIG_OPTION}, s_.substr(5)};
        }

        if (s_ == "-config") {
            return {std::string{CONFIG_OPTION}, {}};
        }
        
        return {};
    }
    
    /**
     * \brief Создаёт парсер командной строки
     */
    [[nodiscard]] auto create_command_line_parser(
        int argc_,
        char* argv_[],
        const boost::program_options::options_description& desc_)
    {
        return boost::program_options::command_line_parser{argc_, argv_}
            .options(desc_)
            .extra_parser(parse_custom_flag);
    }
    
} // unnamed namespace

bool parsing_result::is_valid() const noexcept {
    return !_show_help;  // если показан help, то результат не для использования
}

boost::program_options::options_description create_options_description() {
    boost::program_options::options_description desc{"Allowed options"};
    
    desc.add_options()
        (std::string{HELP_OPTION}.c_str(), "Show this help message")
        (std::string{CONFIG_OPTION}.c_str(), 
         boost::program_options::value<std::string>()->default_value(
             std::string{DEFAULT_CONFIG}),
         "Path to configuration file (can use -config, -cfg or -cfg=FILE)")
        (std::string{STREAMING_MODE}.c_str(), "Enable streaming mode (flag, no arguments needed)");
    
    return desc;
}

parsing_result parse_arguments(int argc_, char* argv_[]) {
    const auto desc = create_options_description();
    parsing_result result;
    
    try {
        auto parser = create_command_line_parser(argc_, argv_, desc);
        boost::program_options::store(parser.run(), result._variables);
        boost::program_options::notify(result._variables);
        
        // Проверяем, нужно ли показать help
        if (result._variables.count(std::string{HELP_OPTION})) {
            result._show_help = true;
            return result;
        }
        
        // Извлекаем путь к конфигу
        if (result._variables.count(std::string{CONFIG_OPTION})) {
            result._config_file = result._variables[std::string{CONFIG_OPTION}]
                                      .as<std::string>();
        }

        if (result._variables.count(std::string{STREAMING_MODE})) {
            result._streaming_mode = true;
        }
        
    } catch (const boost::program_options::error& e_) {
        throw std::invalid_argument{
            "Failed to parse command line arguments: " + std::string{e_.what()}
        };
    }
    
    return result;
}

}  // namespace app::cli