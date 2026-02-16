/**
 * \file tdigest.hpp
 * \brief Реализация T-Digest для оценки квантилей
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 * 
 * T-Digest - алгоритм для эффективной оценки квантилей
 * больших потоков данных с высокой точностью.
 */

#ifndef TDIGEST_HPP
#define TDIGEST_HPP

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <stdexcept>

namespace app::statistics {

/**
 * \brief T-Digest алгоритм для оценки квантилей
 * 
 * Позволяет эффективно оценивать квантили (включая медиану)
 * больших потоков данных с ограниченным использованием памяти.
 */
class tdigest {
public:
    /**
     * \brief Конструктор
     * \param compression_ параметр компрессии
     *        Обычно значения от 20 до 200
     */
    explicit tdigest(std::size_t compression_ = 100);
    
    /**
     * \brief Добавляет значение в распределение
     * \param value_ значение для добавления
     */
    void add(double value_) noexcept;
    
    /**
     * \brief Вычисляет квантиль распределения
     * \param q_ квантиль от 0 до 1
     * \return значение квантиля
     * \throws std::invalid_argument если q_ вне [0,1]
     */
    [[nodiscard]] double quantile(double q_) const noexcept(false);
    
    /**
     * \brief Вычисляет медиану распределения
     * \return медианное значение
     */
    [[nodiscard]] double median() const noexcept { return quantile(0.5); }

    /**
     * \brief Вычисляет mean
     * \return среднее значение
     */
    [[nodiscard]] double mean() const noexcept;
    
    /**
     * \brief Вычисляет extra_values (mean, p90, p95, p99)
     * \return extra_values (mean, p90, p95, p99)
     */
    [[nodiscard]] std::vector<std::pair<std::string, double>> extra_values(std::vector<std::string> const values_name_) const noexcept;
    
    /**
     * \brief Возвращает количество добавленных элементов
     */
    [[nodiscard]] std::size_t size() const noexcept { return _total_count; }
    
    /**
     * \brief Проверяет, пуст ли дигест
     */
    [[nodiscard]] bool empty() const noexcept { return _total_count == 0; }

private:
    /**
     * \brief Центроид - кластер близких значений
     */
    struct centroid {
        double _mean;       ///< Среднее значение
        std::size_t _count; ///< Количество точек в кластере
        
        centroid(double mean_ = 0.0, std::size_t count_ = 0) noexcept;
        
        /**
         * \brief Добавляет значение в центроид
         */
        void add(double value_) noexcept;
        
        /**
         * \brief Сливает два центроида
         */
        void merge(const centroid& other_) noexcept;
    };
    
    /**
     * \brief Вычисляет максимальный вес для центроида при данном квантиле
     */
    [[nodiscard]] double max_weight(double q_) const noexcept;
    
    /**
     * \brief Сжимает центроиды для поддержания ограничения памяти
     */
    void compress();
    
    /**
     * \brief Находит ближайший центроид к значению
     */
    [[nodiscard]] std::size_t find_nearest_centroid(double value_) const noexcept;

    /**
     * \brief Возвращает количество данных
     */
    [[nodiscard]] std::size_t total_count() const noexcept;

private:
    static constexpr double MAX_DOUBLE = std::numeric_limits<double>::max();
    static constexpr double WEIGHT_MULTIPLIER = 4.0;
    
    std::size_t _compression;           ///< Параметр компрессии
    std::vector<centroid> _centroids;   ///< Вектор центроидов
    std::size_t _total_count{0};        ///< Общее количество точек
    double _min_value{MAX_DOUBLE};      ///< Минимальное значение
    double _max_value{-MAX_DOUBLE};     ///< Максимальное значение
};

}  // namespace app::statistics

#endif  // TDIGEST_HPP