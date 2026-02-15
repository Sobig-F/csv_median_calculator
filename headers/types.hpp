/**
 * \file types.hpp
 * \brief Основные типы данных для обработки CSV
 * \author github: Sobig-F
 * \date 2026-02-15
 * \version 1.0
 */

#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

/**
 * \brief Структура для хранения данных из CSV
 * 
 * Содержит временную метку и цену из CSV файла.
 */
class data {
public:
    std::int_fast64_t receive_ts;   ///< Временная метка получения
    double price;                   ///< Цена
    
    /**
     * \brief Конструктор
     * \param time_ временная метка
     * \param price_ цена
     */
    data(std::int_fast64_t time_, double price_) noexcept:
        receive_ts{time_},
        price{price_}
    {}
};

#endif  // TYPES_HPP
