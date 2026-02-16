/**
 * \file logger.cpp
 * \brief Логгер
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "logger.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace app::processing {

std::once_flag logger::init_flag;
std::shared_ptr<spdlog::logger> logger::logger_instance;

// ==================== logger initialization ====================

void logger::init() noexcept
{
    std::call_once(init_flag, []() {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        
        logger_instance = std::make_shared<spdlog::logger>("console_logger", console_sink);
        logger_instance->set_level(spdlog::level::info);
                
        // Регистрируем как основной логгер
        spdlog::set_default_logger(logger_instance);
    });
}

// ==================== logger implementation ====================

void logger::shutdown() noexcept
{
    if (logger_instance) {
        logger_instance->flush();
        spdlog::drop_all();
        logger_instance.reset();
    }
}

}  // namespace app::processing