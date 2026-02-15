/**
 * \file data_queue.cpp
 * \brief Реализация потокобезопасной очереди
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "data_queue.hpp"

#include <utility>

namespace app::processing {

// ==================== move operations ====================

data_queue::data_queue(data_queue&& other_) noexcept
{
    std::lock_guard<std::mutex> lock{other_._mutex};
    _tasks = std::move(other_._tasks);
    _stopped.store(other_._stopped.load());
}

data_queue& data_queue::operator=(data_queue&& other_) noexcept
{
    if (this != &other_) {
        std::scoped_lock lock{_mutex, other_._mutex};
        _tasks = std::move(other_._tasks);
        _stopped.store(other_._stopped.load());
    }
    return *this;
}

// ==================== public interface ====================

void data_queue::push(std::unique_ptr<data> task_)
{
    {
        std::lock_guard<std::mutex> lock{_mutex};
        _tasks.push(std::move(task_));
    }
    
    // Уведомляем один ожидающий поток
    _condition.notify_one();
}

std::optional<std::unique_ptr<data>> data_queue::pop() noexcept
{
    std::lock_guard<std::mutex> lock{_mutex};
    
    if (_tasks.empty()) {
        return std::nullopt;
    }
    
    auto result = std::move(_tasks.front());
    _tasks.pop();
    return result;
}

std::unique_ptr<data> data_queue::wait_and_pop()
{
    std::unique_lock<std::mutex> lock{_mutex};
    
    // Ждём пока появятся данные или не будет остановки
    _condition.wait(lock, [this] {
        return !_tasks.empty() || _stopped.load();
    });
    
    // Если остановлено и очередь пуста, возвращаем nullptr
    if (_stopped.load() && _tasks.empty()) {
        return nullptr;
    }
    
    auto result = std::move(_tasks.front());
    _tasks.pop();
    return result;
}

std::size_t data_queue::size() const noexcept
{
    std::lock_guard<std::mutex> lock{_mutex};
    return _tasks.size();
}

bool data_queue::empty() const noexcept
{
    std::lock_guard<std::mutex> lock{_mutex};
    return _tasks.empty();
}

void data_queue::stop() noexcept
{
    _stopped.store(true);
    _condition.notify_all();  // Будим все ожидающие потоки
}

std::atomic<bool> data_queue::is_stopped() noexcept
{
    return _stopped.load();
}

}  // namespace app::processing