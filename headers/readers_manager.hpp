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
    explicit readers_manager(bool streaming_mode_);
    
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
     * \brief Возвращает количество обработанных задач
     */
    [[nodiscard]] std::atomic<std::size_t> total_tasks() const noexcept;

    /**
     * \brief Запускает сортировку и отправку задач из reader в очередь задач
     */
    void run() noexcept;

    /**
     * \brief Останавливает всех reader, выключает "воронку" и закрывает очередь
     */
    void stop() noexcept;

    /**
     * \brief Очередь задач
     */
    [[nodiscard]] std::shared_ptr<app::processing::data_queue> tasks() const noexcept { return _tasks; };

private:
    /**
     * \brief Перекидывает задачи из _readers в _tasks
     */
    void redirecting_tasks(std::stop_token stoken) noexcept;

    /**
     * \brief Внутренняя структура для хранения читателей
     */
    struct reader {
        std::shared_ptr<app::io::csv_reader> _reader;
        std::jthread _thread;
        std::shared_ptr<app::processing::data_queue> _reader_local_queue;

        reader(std::shared_ptr<app::io::csv_reader> reader_, std::jthread thread_)
        : _reader{std::move(reader_)}
        , _thread{std::move(thread_)}
        {
            _reader_local_queue = _reader->local_queue();
        }
    };
    
    std::vector<reader> _readers;                           ///< Читатели, их потоки и верхние data
    std::shared_ptr<app::processing::data_queue> _tasks;    ///< Очередь для данных
    mutable std::mutex _mutex;                              ///< Мьютекс для синхронизации
    bool _streaming_mode{false};                            ///< Состояние streaming-mode (нужно ли ожидать новых данных)
    std::jthread _redirecting_tasks;                        ///< Поток "воронки" задач
    std::stop_source _readers_stoken;                       ///< Источник токена остановки reader
    std::stop_source _stoken_redirecting;                   ///< Источник токена остановки "воронки"
};

}  // namespace app::io

#endif  // READERS_MANAGER_HPP