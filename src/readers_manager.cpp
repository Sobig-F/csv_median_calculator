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

readers_manager::readers_manager(bool streaming_mode_)
    :_streaming_mode{streaming_mode_}
{
    _tasks = std::make_shared<app::processing::data_queue>();
}

readers_manager::~readers_manager()
{
}

readers_manager::readers_manager(readers_manager&& other_) noexcept
    : _readers{std::move(other_._readers)}
    , _tasks{std::move(other_._tasks)}
{}

readers_manager& readers_manager::operator=(readers_manager&& other_) noexcept
{
    if (this != &other_) {
        stop();
        
        std::lock_guard<std::mutex> lock{_mutex};
        _readers = std::move(other_._readers);
        _tasks = std::move(other_._tasks);
    }
    return *this;
}

// ==================== public методы ====================

void readers_manager::add_csv_file(std::string filename_) noexcept(false)
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
        std::jthread thread = std::jthread{&csv_reader::read_file, reader.get(), _readers_stoken.get_token()};

        std::lock_guard<std::mutex> lock{_mutex};

        _readers.push_back({
            std::move(reader),
            std::move(thread),
        });
        
    } catch (const std::exception& e_) {
        throw std::runtime_error{
            "Failed to create reader for " + filename_ + ": " + e_.what()
        };
    }
}

void readers_manager::run() noexcept
{
    _redirecting_tasks = std::jthread{[this] {
        redirecting_tasks(_stoken_redirecting.get_token());
    }};
}

void readers_manager::stop() noexcept
{
    if (_streaming_mode) {
        //остановить ридеры
        _readers_stoken.request_stop();
    }
    for (auto& reader : _readers) {
        if (reader._thread.joinable()) {
            reader._thread.join();
        }
    }
    if (_redirecting_tasks.joinable()) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        _stoken_redirecting.request_stop();
        _redirecting_tasks.join();
    }
    //остановить главную очередь
    _tasks->stop();
}

std::atomic<std::size_t> readers_manager::total_tasks() const noexcept
{
    return _tasks->total_count().load();
}

void readers_manager::redirecting_tasks(std::stop_token stoken) noexcept
{
    // using namespace std::chrono_literals;

    int_fast64_t min_recieve_ts = 0;
    while (
    [&] -> bool {
        // std::lock_guard<std::mutex> lock{_mutex};
        for (auto& tasks : _readers) {
            if (!tasks._reader_local_queue->empty()) {
                return true;
            }
        }
        // std::cout << "FALSE" << std::endl;
        if (!stoken.stop_requested()) { return true; }
        return false;
    }()) {
        // std::lock_guard<std::mutex> lock{_mutex};
        std::shared_ptr<app::processing::data_queue> queue_with_min_ts = nullptr;
        for (auto& tasks : _readers) {
            if (tasks._reader_local_queue->empty()) {
                continue;
            }

            // //провевяемая очередь начинается с элемента < минимального времени
            // while (!tasks._reader_local_queue->empty()) {
            //     // spdlog::info("{}", tasks._reader_local_queue->front()->receive_ts);
            //     // Получаем указатель на первый элемент
            //     const data* front_data = tasks._reader_local_queue->front();
            //     if (!front_data) break;  // защита на всякий случай
                
            //     if (front_data->receive_ts >= min_recieve_ts) break;
            //     // Удаляем устаревший элемент
            //     tasks._reader_local_queue->pop();
            // }

            // захвачена первая непустая очередь
            if (!tasks._reader_local_queue->empty() && queue_with_min_ts == nullptr) {
                queue_with_min_ts = tasks._reader_local_queue;
            }
            if (!tasks._reader_local_queue->empty() && tasks._reader_local_queue->front()->receive_ts <= queue_with_min_ts->front()->receive_ts) {
                queue_with_min_ts = tasks._reader_local_queue;
            }

        }
        if (queue_with_min_ts) {
            min_recieve_ts = queue_with_min_ts->front()->receive_ts;
            _tasks->push(queue_with_min_ts->pop());
        }
    }
}

}  // namespace app::io