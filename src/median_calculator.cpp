/**
 * \file median_calculator.cpp
 * \brief Реализация вычисления медианы
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "median_calculator.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>

#include "logger.hpp"

namespace app::processing {

// ==================== конструкторы/деструктор ====================

median_calculator::median_calculator(
    std::shared_ptr<data_queue> tasks_,
    std::vector<std::string> extra_values_,
    std::size_t digest_compression_)
    : _extra_values_name{move(extra_values_)}
    , _tdigest{std::make_unique<app::statistics::tdigest>(digest_compression_)}
    , _tasks{std::move(tasks_)}
{}

median_calculator::~median_calculator()
{
    // stop();
}

median_calculator::median_calculator(median_calculator&& other_) noexcept
    : _tdigest{std::move(other_._tdigest)}
    , _tasks{std::move(other_._tasks)}
    , _file_streamer{std::move(other_._file_streamer)}
    , _running{other_._running.load()}
    // , _stopped{other_._stopped.load()}
{}

median_calculator& median_calculator::operator=(median_calculator&& other_) noexcept
{
    if (this != &other_) {
        _running.store(false);  // Останавливаем текущую обработку
        
        _tdigest = std::move(other_._tdigest);
        _tasks = std::move(other_._tasks);
        _file_streamer = std::move(other_._file_streamer);
        _running.store(other_._running.load());
        // _stopped.store(other_._stopped.load());
    }
    return *this;
}

// ==================== public методы ====================

void median_calculator::set_output_stream(
    std::shared_ptr<app::io::file_streamer> streamer_) noexcept
{
    std::lock_guard<std::mutex> lock{_output_mutex};
    _file_streamer = std::move(streamer_);
}

std::thread median_calculator::run_async() noexcept
{
    _running.store(true);
    return std::thread{[this] { process_loop(); }};
}

// ==================== private методы ====================

void median_calculator::output_result(
    std::int_fast64_t timestamp_,
    double median_,
    std::vector<std::pair<std::string, double>> const extra_values_) noexcept(false)
{
    std::lock_guard<std::mutex> lock{_output_mutex};
    
    if (_file_streamer) {
        // Запись в файл
        _file_streamer->write_median(timestamp_, median_, extra_values_);
    } else {
        // Вывод в консоль
        std::cout << std::fixed << std::setprecision(8)
        << "receive_ts: " << timestamp_
        << " / median: " << median_;
        
        std::cout << std::endl;
    }
}

void median_calculator::process_loop() noexcept(false)
{
    double old_median = -1.0;

    while (_running.load()) {
        // Блокируемся до появления данных или остановки
        auto task = _tasks->wait_and_pop();

        if (!task || !_running.load()) {
            break;
        }
        // Обновляем T-Digest
        _tdigest->add(task->price);
        const double now_median = _tdigest->median();
        std::vector<std::pair<std::string, double>> _extra_values = _tdigest->extra_values(_extra_values_name);
            
        // Выводим если медиана значительно изменилась
        if (std::abs(now_median - old_median) > EPSILON) {
            output_result(task->receive_ts, now_median, _extra_values);
            old_median = now_median;
        }
    }
    
    _running.store(false);
}

}  // namespace app::processing