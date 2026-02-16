/**
 * \file logger.hpp
 * \brief Логгер
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */ 

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "spdlog/spdlog.h"

namespace app::processing {

#define ANSI_RESET   "\033[0m"
#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

/**
 * \brief logger для логирования событий
 */
class logger {
private:
    static std::once_flag init_flag;
    static std::shared_ptr<spdlog::logger> logger_instance;
public:
    /**
     * \brief Инициализация логгера
     */
    [[nodiscard]] static void init() noexcept;

    /**
     * \brief Завершение и закрытие логгера
     */
    [[nodiscard]] static void shutdown() noexcept;
private:
    logger() = delete;                          //< запрет на создание
    ~logger() = delete;                         //< запрет на уничтожение
    logger(const logger&) = delete;             //< запрет на перемещение
    logger& operator=(const logger&) = delete;  //< запрет на копирование
};

}  // namespace app::processing

#endif  // LOGGER_HPP