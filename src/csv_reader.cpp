/**
 * \file csv_reader.cpp
 * \brief Реализация чтения CSV файлов
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "csv_reader.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "data_queue.hpp"
#include "types.hpp"
#include "logger.hpp"

namespace app::io {

namespace {
    // Константы для парсинга
    constexpr char CSV_DELIMITER = ';';
    constexpr std::size_t TIMESTAMP_INDEX = 0;
    constexpr std::size_t PRICE_INDEX = 2;
    
    // Мьютекс для синхронизации вывода (можно вынести в отдельный логгер)
    std::mutex g_cout_mutex;
    
    /**
     * \brief Безопасно парсит строку в int64_t
     */
    [[nodiscard]] std::optional<std::int_fast64_t> safe_parse_int(
        const std::string& str_) noexcept
    {
        try {
            return boost::lexical_cast<std::int_fast64_t>(str_);
        } catch (const boost::bad_lexical_cast&) {
            return std::nullopt;
        }
    }
    
    /**
     * \brief Безопасно парсит строку в double
     */
    [[nodiscard]] std::optional<double> safe_parse_double(
        const std::string& str_) noexcept
    {
        try {
            return boost::lexical_cast<double>(str_);
        } catch (const boost::bad_lexical_cast&) {
            return std::nullopt;
        }
    }
    
} // unnamed namespace

// ==================== csv_reader implementation ====================

csv_reader::csv_reader(path_string filename_, data_queue_ptr tasks_, bool streamin_mode_)
    : _mapping{filename_.c_str(), boost::interprocess::read_only}
    , _region{_mapping, boost::interprocess::read_only}
    , _data{static_cast<const char*>(_region.get_address())}
    , _size{_region.get_size()}
    , _position{0}
    , _filename{std::move(filename_)}
    , _tasks{std::move(tasks_)}
    , _streaming_mode{streamin_mode_}
{
    _local_queue = std::make_shared<app::processing::data_queue>();
}

csv_reader::csv_reader(csv_reader&& other_) noexcept
    : _mapping{std::move(other_._mapping)}
    , _region{std::move(other_._region)}
    , _data{other_._data}
    , _size{other_._size}
    , _position{other_._position}
    , _filename{std::move(other_._filename)}
    , _tasks{std::move(other_._tasks)}
{
    other_._data = nullptr;
    other_._size = 0;
    other_._position = 0;
}

csv_reader& csv_reader::operator=(csv_reader&& other_) noexcept
{
    if (this != &other_) {
        _mapping = std::move(other_._mapping);
        _region = std::move(other_._region);
        _data = other_._data;
        _size = other_._size;
        _position = other_._position;
        _filename = std::move(other_._filename);
        _tasks = std::move(other_._tasks);
        
        other_._data = nullptr;
        other_._size = 0;
        other_._position = 0;
    }
    return *this;
}

const path_string& csv_reader::filename() const noexcept
{
    return _filename;
}

std::unique_ptr<data> csv_reader::parse_line(
    const std::string& line_) noexcept
{
    try {
        std::vector<std::string> split_line;
        boost::split(split_line, line_, boost::is_any_of(std::string{CSV_DELIMITER}));
        
        if (split_line.size() <= std::max(TIMESTAMP_INDEX, PRICE_INDEX)) {
            return nullptr;  // Недостаточно полей
        }
        
        auto timestamp = safe_parse_int(split_line[TIMESTAMP_INDEX]);
        auto price = safe_parse_double(split_line[PRICE_INDEX]);
        
        if (!timestamp || !price) {
            return nullptr;  // Ошибка парсинга чисел
        }
        
        return std::make_unique<data>(*timestamp, *price);
        
    } catch (const std::exception& e_) {
        // Логируем ошибку, но не прерываем выполнение
        std::lock_guard<std::mutex> lock{g_cout_mutex};
        spdlog::error("Parse error in line '{}': ", e_.what());
        return nullptr;
    }
}

void csv_reader::refresh(std::size_t position_) noexcept(false)
{
    _mapping = file_mapping{_filename.c_str(), boost::interprocess::read_only};
    _region = mapped_region{_mapping, boost::interprocess::read_only};
    _data = static_cast<const char*>(_region.get_address());
    _size = _region.get_size();
    _position = position_;
}

void csv_reader::read_file(std::stop_token stoken_) noexcept(false)
{
    using namespace std::chrono_literals;
    
    std::string current_line;
    
    // Пропускаем заголовок (первую строку)
    while (_position < _size && _data[_position] != '\n') {
        ++_position;
    }
    ++_position;  // Пропускаем сам '\n'
    
    // Основной цикл чтения
    while (!stoken_.stop_requested()) {
        try {
            // Читаем строку до символа новой строки
            while (_position < _size && _data[_position] != '\n') {
                current_line += _data[_position++];
            }
            
            // Обрабатываем прочитанную строку
            if (!current_line.empty()) {
                if (auto data = parse_line(current_line)) {
                    _local_queue->push(std::move(data));
                }
                current_line.clear();
            }
            ++_position;  // Пропускаем '\n'
            
            // Проверяем, не вышли ли за границы файла
            if (_position >= _size) {
                if (!_streaming_mode) {
                    spdlog::info(ANSI_GREEN "SUCCESS:" ANSI_RESET " {}", _filename);
                    break;
                } else if (_existing_data_has_been_processed) {
                    _existing_data_has_been_processed = false;
                }

                const auto previous_size = _size;
                refresh(_position);
                
                // Если файл не вырос - ждём
                if (_size <= previous_size) {
                    std::this_thread::sleep_for(100ms);
                } else {
                    _existing_data_has_been_processed = true;
                }

                continue;
            }
            
            
        } catch (const std::exception& e_) {
            std::lock_guard<std::mutex> lock{g_cout_mutex};
            std::cerr << "Error reading file " << _filename << ": " 
                      << e_.what() << std::endl;
            
            // Пытаемся восстановиться
            current_line.clear();
            std::this_thread::sleep_for(1s);
        }
    }
}

}  // namespace app::io
