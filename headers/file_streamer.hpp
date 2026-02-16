/**
 * \file file_streamer.hpp
 * \brief Потоковый вывод результатов в CSV файл
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef FILE_STREAMER_HPP
#define FILE_STREAMER_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>

#include "types.hpp"

namespace app::io {

/**
 * \brief Класс для записи результатов в CSV файл
 * 
 * Автоматически добавляет заголовок если файл пустой.
 * Потокобезопасность должна обеспечиваться вызывающим кодом.
 */
class file_streamer {
public:
    /**
     * \brief Конструктор
     * \param filename_ путь к выходному файлу
     * \throws std::runtime_error если файл не может быть открыт
     */
    explicit file_streamer(std::string filename_);
    
    /**
     * \brief Деструктор - автоматически закрывает файл
     */
    ~file_streamer() = default;
    
    // Запрет копирования
    file_streamer(const file_streamer&) = delete;
    file_streamer& operator=(const file_streamer&) = delete;
    
    // Разрешение перемещения
    file_streamer(file_streamer&& other_) noexcept;
    file_streamer& operator=(file_streamer&& other_) noexcept;
    
    /**
     * \brief Записывает медианное значение
     * \param timestamp_ временная метка
     * \param median_ медианное значение
     * \return ссылка на себя для chaining
     */
    file_streamer& write_median(std::int_fast64_t timestamp_, double median_);
    
    /**
     * \brief Проверяет, открыт ли файл
     * \return true если файл открыт
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * \brief Возвращает количество добавленных записей
     */
    std::size_t total_records() const noexcept;
    
    /**
     * \brief Принудительно сбрасывает буфер на диск
     */
    void flush();

private:
    /**
     * \brief Записывает заголовок если файл пустой
     */
    void write_header_if_needed();
    
private:
    std::ofstream _file_stream;           ///< Файловый поток
    std::string _filename;                ///< Имя файла для перемещения
    bool _header_written{false};          ///< Флаг записи заголовка
    static std::size_t _total_records;  ///< Общее количество записей
};

// Перегрузки операторов для удобства (но лучше использовать write_median)
template<typename T>
typename std::enable_if_t<std::is_arithmetic_v<T>, file_streamer&>
operator<<(file_streamer& stream_, const T& value_) {
    // Преобразуем в строку и записываем
    stream_.write_median(0, static_cast<double>(value_));
    return stream_;
}

}  // namespace app::io

#endif  // FILE_STREAMER_HPP