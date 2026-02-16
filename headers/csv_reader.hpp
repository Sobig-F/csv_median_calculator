/**
 * \file csv_reader.hpp
 * \brief Чтение CSV файлов с использованием memory-mapped files
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <memory>
#include <string>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "data_queue.hpp"

namespace app::io {

using file_mapping = boost::interprocess::file_mapping;
using mapped_region = boost::interprocess::mapped_region;
using data_queue_ptr = std::shared_ptr<app::processing::data_queue>;
using path_string = std::string;

/**
 * \brief Класс для чтения CSV файлов с поддержкой динамического обновления
 * 
 * Использует memory-mapped files для эффективного чтения больших файлов
 * и отслеживает изменения файла в реальном времени.
 */
class csv_reader {
public:
    /**
     * \brief Конструктор
     * \param filename_ путь к CSV файлу
     * \param tasks_ очередь для передачи прочитанных данных
     * \throws boost::interprocess::interprocess_exception если файл не может быть открыт
     */
    csv_reader(path_string filename_, data_queue_ptr tasks_, bool streamin_mode_);
    
    /**
     * \brief Деструктор
     */
    ~csv_reader() = default;
    
    // Запрет копирования
    csv_reader(const csv_reader&) = delete;
    csv_reader& operator=(const csv_reader&) = delete;
    
    // Разрешение перемещения
    csv_reader(csv_reader&& other_) noexcept;
    csv_reader& operator=(csv_reader&& other_) noexcept;
    
    /**
     * \brief Читает файл и помещает данные в очередь
     * 
     * Метод работает в цикле, ожидая появления новых данных в файле.
     * Для остановки чтения используйте механизм отмены через data_queue.
     */
    void read_file() noexcept(false);
    
    /**
     * \brief Возвращает имя файла
     * \return путь к файлу
     */
    [[nodiscard]] const path_string& filename() const noexcept;

private:
    /**
     * \brief Обновляет memory-mapped region после изменения файла
     * \param position_ текущая позиция для продолжения чтения
     */
    void refresh(std::size_t position_) noexcept(false);
    
    /**
     * \brief Парсит одну строку CSV в структуру Data
     * \param line_ строка для парсинга
     * \return unique_ptr на Data или nullptr при ошибке
     */
    [[nodiscard]] static std::unique_ptr<class data> parse_line(
        const std::string& line_) noexcept;

private:
    file_mapping _mapping;      ///< Memory-mapped file
    mapped_region _region;      ///< Mapped region
    const char* _data;          ///< Указатель на данные
    std::size_t _size;          ///< Размер файла
    std::size_t _position;      ///< Текущая позиция чтения
    path_string _filename;      ///< Имя файла
    data_queue_ptr _tasks;      ///< Очередь для результатов
    bool _existing_data_has_been_processed{true}; ///< Обработаны ли существующие данные
    bool _streaming_mode{false};///< Состояние streaming-mode (нужно ли ожидать новых данных)
};

}  // namespace app::io

#endif  // CSV_READER_HPP