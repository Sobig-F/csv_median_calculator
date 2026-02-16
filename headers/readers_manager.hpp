/**
 * \file readers_manager.hpp
 * \brief Управление коллекцией читателей CSV файлов
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef READERS_MANAGER_HPP
#define READERS_MANAGER_HPP

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <iostream>

#include "csv_reader.hpp"
#include "data_queue.hpp"

namespace app::io {

/**
 * \brief Управляет множеством читателей CSV файлов
 * 
 * Создаёт и запускает по потоку на каждый CSV файл.
 * Обеспечивает корректное завершение всех потоков.
 */
class readers_manager {
public:
    /**
     * \brief Конструктор
     * \param tasks_ очередь для передачи прочитанных данных
     * \param streaming_mode_ режим потокового чтения данных
     */
    explicit readers_manager(std::shared_ptr<app::processing::data_queue> tasks_, bool streaming_mode_);
    
    /**
     * \brief Деструктор - останавливает все потоки
     */
    ~readers_manager();
    
    // Запрет копирования
    readers_manager(const readers_manager&) = delete;
    readers_manager& operator=(const readers_manager&) = delete;
    
    // Разрешение перемещения
    readers_manager(readers_manager&& other_) noexcept;
    readers_manager& operator=(readers_manager&& other_) noexcept;
    
    /**
     * \brief Добавляет новый CSV файл для чтения и запускает поток для чтения
     * \param filename_ путь к CSV файлу
     * \throws std::invalid_argument если файл не существует
     * \throws std::runtime_error если не удалось создать читатель
     */
    void add_csv_file(std::string filename_) noexcept(false);
    
    /**
     * \brief Останавливает все потоки чтения
     */
    void stop_all() noexcept;
    
    /**
     * \brief Ждёт завершения всех потоков
     */
    void join_all_readers() noexcept;

    /**
     * \brief Возвращает количество обработанных задач
     */
    [[nodiscard]] std::atomic<std::size_t> total_tasks() const noexcept;

private:
    /**
     * \brief Внутренняя структура для хранения пары читатель-поток
     */
    struct reader_thread_pair {
        std::shared_ptr<app::io::csv_reader> _reader;
        std::jthread _thread;
    };
    
    std::vector<reader_thread_pair> _readers;               ///< Читатели и их потоки
    std::shared_ptr<app::processing::data_queue> _tasks;    ///< Очередь для данных
    mutable std::mutex _mutex;                              ///< Мьютекс для синхронизации
    bool _streaming_mode{false};                            ///< Состояние streaming-mode (нужно ли ожидать новых данных)
};

}  // namespace app::io

#endif  // READERS_MANAGER_HPP