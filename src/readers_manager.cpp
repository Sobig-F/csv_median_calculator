/**
 * \file readers_manager.cpp
 * \brief Реализация управления читателями
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "readers_manager.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

#include "data_queue.hpp"
#include "logger.hpp"

namespace app::io {

namespace fs = std::filesystem;

// ==================== конструкторы/деструктор ====================

readers_manager::readers_manager(std::shared_ptr<app::processing::data_queue> tasks_, bool streaming_mode_)
    : _tasks{std::move(tasks_)}
    , _streaming_mode{streaming_mode_}
{}

readers_manager::~readers_manager()
{
    stop_all();
    join_all_readers();
}

readers_manager::readers_manager(readers_manager&& other_) noexcept
    : _readers{std::move(other_._readers)}
    , _tasks{std::move(other_._tasks)}
{}

readers_manager& readers_manager::operator=(readers_manager&& other_) noexcept
{
    if (this != &other_) {
        stop_all();
        join_all_readers();
        
        std::lock_guard<std::mutex> lock{_mutex};
        _readers = std::move(other_._readers);
        _tasks = std::move(other_._tasks);
    }
    return *this;
}

// ==================== public методы ====================

void readers_manager::add_csv_file(std::string filename_)
{
    // Проверяем существование файла
    if (!fs::exists(filename_)) {
        throw std::invalid_argument{
            "CSV file does not exist: " + filename_
        };
    }
    
    if (!fs::is_regular_file(filename_)) {
        throw std::invalid_argument{
            "Path is not a regular file: " + filename_
        };
    }
    
    try {
        // Создаём читателя
        auto reader = std::make_shared<app::io::csv_reader>(filename_, _tasks, _streaming_mode);
        
        // Создаём поток с функцией чтения
        // Используем jthread для автоматического join при разрушении
        std::jthread thread{&csv_reader::read_file, reader.get()};
        
        // Сохраняем пару
        std::lock_guard<std::mutex> lock{_mutex};
        _readers.push_back({
            std::move(reader),
            std::move(thread)
        });
        
    } catch (const std::exception& e_) {
        throw std::runtime_error{
            "Failed to create reader for " + filename_ + ": " + e_.what()
        };
    }
}

void readers_manager::stop_all() noexcept
{
    std::lock_guard<std::mutex> lock{_mutex};
    
    // Останавливаем все очереди задач, чтобы читатели завершились
    if (_tasks) {
        // spdlog::info("Есть очередь");
        using namespace std::chrono_literals;

        while (!_streaming_mode && !_tasks->empty()) {
            // spdlog::info("Ожидание завершения задач");
            std::this_thread::sleep_for(500ms);
        }

        _tasks->stop();
    }
}

void readers_manager::join_all_readers()
{
    std::lock_guard<std::mutex> lock{_mutex};
    // spdlog::info("Ожидание завершения читателей");
    for (auto& pair : _readers) {
        if (pair._thread.joinable()) {
            pair._thread.join();
        }
    }
    // spdlog::info("Читатели остановлены");
    
    _readers.clear();
}

std::size_t readers_manager::reader_count() const noexcept
{
    std::lock_guard<std::mutex> lock{_mutex};
    return _readers.size();
}

bool readers_manager::has_active_readers() const noexcept
{
    std::lock_guard<std::mutex> lock{_mutex};
    
    return std::any_of(_readers.begin(), _readers.end(),
        [](const auto& pair) {
            return pair._thread.joinable();
        });
}

std::atomic<std::size_t> readers_manager::total_tasks() const noexcept
{
    return _tasks->total_count().load();
}

}  // namespace app::io