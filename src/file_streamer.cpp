/**
 * \file file_streamer.cpp
 * \brief Реализация потокового вывода в CSV
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "file_streamer.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace app::io {

namespace fs = std::filesystem;

std::size_t file_streamer::_total_records = 0;

// ==================== конструктор/деструктор ====================

file_streamer::file_streamer(std::string filename_)
    : _filename{std::move(filename_)}
{
    // Открываем файл в режиме append
    _file_stream.open(_filename, std::ios::app);
    
    if (!_file_stream.is_open()) {
        throw std::runtime_error{
            "Failed to open file for writing: " + _filename
        };
    }
}

file_streamer::file_streamer(file_streamer&& other_) noexcept
    : _file_stream{std::move(other_._file_stream)}
    , _filename{std::move(other_._filename)}
    , _header_written{other_._header_written}
{}

file_streamer& file_streamer::operator=(file_streamer&& other_) noexcept
{
    if (this != &other_) {
        _file_stream = std::move(other_._file_stream);
        _filename = std::move(other_._filename);
        _header_written = other_._header_written;
    }
    return *this;
}

// ==================== private методы ====================

void file_streamer::write_header_if_needed(std::vector<std::pair<std::string, double>> const extra_values_name_) noexcept
{
    // Если файл пустой - пишем заголовок
    if (fs::file_size(_filename) == 0) {
        _file_stream << "receive_ts;median";
        for (auto& _extra_value_name : extra_values_name_) {
            _file_stream << ";" << _extra_value_name.first;
        }
        _file_stream << std::endl;
    }
    _header_written = true;
}

// ==================== public методы ====================

file_streamer& file_streamer::write_median(
    std::int_fast64_t timestamp_,
    double median_,
    std::vector<std::pair<std::string, double>> const extra_values_) noexcept(false)
{
    if (!_file_stream.is_open()) {
        throw std::runtime_error{"File stream is not open"};
    }

    // Проверяем, нужно ли писать заголовок
    if (!_header_written) {
        write_header_if_needed(extra_values_);
    }
    
    // Форматируем вывод
    _file_stream << std::fixed << std::setprecision(8) << timestamp_ << ';' << median_;
    
    for (auto& value : extra_values_) {
        _file_stream << ";" << std::fixed << std::setprecision(8) << value.second;
    }
    _file_stream << std::endl;

    ++_total_records;

    return *this;
}

std::size_t file_streamer::total_records() const noexcept
{
    return _total_records;
}

void file_streamer::flush() noexcept
{
    if (_file_stream.is_open()) {
        _file_stream.flush();
    }
}

}  // namespace app::io