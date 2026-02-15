/**
 * \file readers_manager.cpp
 * \brief Реализация управления читателями
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "readers_manager.hpp"
#include "data_queue.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace app::io {

namespace fs = std::filesystem;

// ==================== конструкторы/деструктор ====================

readers_manager::readers_manager(std::shared_ptr<app::processing::data_queue> tasks_)
    : _tasks{std::move(tasks_)}
{}

readers_manager::~readers_manager()
{
    stop_all();
    join_all();
}

readers_manager::readers_manager(readers_manager&& other_) noexcept
    : _readers{std::move(other_._readers)}
    , _tasks{std::move(other_._tasks)}
{}

readers_manager& readers_manager::operator=(readers_manager&& other_) noexcept
{
    if (this != &other_) {
        stop_all();
        join_all();
        
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
        auto reader = std::make_shared<app::io::csv_reader>(filename_, _tasks);
        
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
        _tasks->stop();
    }
}

void readers_manager::join_all()
{
    std::lock_guard<std::mutex> lock{_mutex};
    
    for (auto& pair : _readers) {
        if (pair._thread.joinable()) {
            std::cout << "Try join" << std::endl;
            pair._thread.join();
            std::cout << "Success join" << std::endl;
        }
    }
    
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

}  // namespace app::io