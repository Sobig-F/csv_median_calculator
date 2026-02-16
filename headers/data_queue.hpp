/**
 * \file data_queue.hpp
 * \brief Потокобезопасная очередь для передачи данных между потоками
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef data_QUEUE_HPP
#define data_QUEUE_HPP

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>

#include "types.hpp"

namespace app::processing {

/**
 * \brief Потокобезопасная очередь для объектов data
 * 
 * Позволяет безопасно передавать уникальные указатели на data
 * между потоками производителями и потребителями.
 */
class data_queue {
public:
    /**
     * \brief Конструктор
     */
    data_queue() = default;
    
    /**
     * \brief Деструктор
     */
    ~data_queue() = default;
    
    // Запрет копирования
    data_queue(const data_queue&) = delete;
    data_queue& operator=(const data_queue&) = delete;
    
    // Разрешение перемещения
    data_queue(data_queue&& other_) noexcept;
    data_queue& operator=(data_queue&& other_) noexcept;
    
    /**
     * \brief Добавляет задачу в очередь
     * \param task_ уникальный указатель на data
     */
    void push(std::unique_ptr<data> task_) noexcept(false);
    
    /**
     * \brief Извлекает задачу из очереди (блокирующая версия)
     * \return уникальный указатель на data (ждёт пока появится элемент)
     */
    [[nodiscard]] std::unique_ptr<data> wait_and_pop() noexcept(false);
    
    /**
     * \brief Проверяет, пуста ли очередь
     * \return true если очередь пуста
     */
    [[nodiscard]] bool empty() const noexcept;
    
    /**
     * \brief Останавливает ожидание в wait_and_pop
     */
    void stop() noexcept;

    /**
     * \brief Остановлена ли очередь
     */
    std::atomic<bool> is_stopped() noexcept;

    [[nodiscard]] std::atomic<std::size_t> total_count() const noexcept;

private:
    std::queue<std::unique_ptr<data>> _tasks;        ///< Очередь задач
    mutable std::mutex _mutex;                       ///< Мьютекс для синхронизации
    std::condition_variable _condition;              ///< Condition variable для ожидания
    std::atomic<bool> _stopped{false};               ///< Флаг остановки
    std::atomic<std::size_t> _total_count{0};        ///< Обработанные задачи
};

}  // namespace app::processing

#endif  // data_QUEUE_HPP