/**
 * \file median_calculator.hpp
 * \brief Вычисление медианы потока данных с использованием T-Digest
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef MEDIAN_CALCULATOR_HPP
#define MEDIAN_CALCULATOR_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "data_queue.hpp"
#include "file_streamer.hpp"
#include "tdigest.hpp"

namespace app::processing {

/**
 * \brief Класс для вычисления медианы в реальном времени
 * 
 * Получает данные из очереди, обновляет T-Digest и выводит
 * медиану при её значительном изменении.
 */
class median_calculator {
public:
    /**
     * \brief Конструктор
     * \param tasks_ очередь с входными данными
     * \param digest_compression_ компрессия для T-Digest (по умолчанию 25)
     */
    explicit median_calculator(
        std::shared_ptr<data_queue> tasks_,
        std::vector<std::string> extra_values_ = {},
        std::shared_ptr<app::io::file_streamer> file_streamer_ = nullptr,
        std::size_t digest_compression_ = 25);
    
    /**
     * \brief Деструктор - останавливает обработку
     */
    ~median_calculator();
    
    // Запрет копирования
    median_calculator(const median_calculator&) = delete;
    median_calculator& operator=(const median_calculator&) = delete;
    
    // Разрешение перемещения
    median_calculator(median_calculator&& other_) noexcept;
    median_calculator& operator=(median_calculator&& other_) noexcept;

    void stop() noexcept;

private:
    /**
     * \brief Внутренний метод обработки данных
     */
    void calculating(std::stop_token stoken_) noexcept(false);
    
    /**
     * \brief Выводит результат
     */
    void output_result(
        std::int_fast64_t timestamp_,
        double median_,
        std::vector<std::pair<std::string, double>> const extra_values_) noexcept(false);

private:
    static constexpr double EPSILON = 1e-10;                ///< Порог изменения медианы
    
    std::unique_ptr<app::statistics::tdigest> _tdigest;     ///< T-Digest для оценки квантилей
    std::shared_ptr<data_queue> _tasks;                     ///< Входная очередь
    std::shared_ptr<app::io::file_streamer> _file_streamer; ///< Выходной поток
    std::mutex _output_mutex;                               ///< Мьютекс для вывода
    std::vector<std::string> _extra_values_name;            ///< Вычисляемые values
    std::jthread _calculating;                              ///< Поток калькулятора
    mutable std::mutex _mutex;                              ///< Мьютекс для записи в файл
    std::stop_source _stop_source;                          ///< Источник токена остановки потока калькулятора
};

}  // namespace app::processing

#endif  // MEDIAN_CALCULATOR_HPP