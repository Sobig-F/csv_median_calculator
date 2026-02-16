/**
 * \file tdigest.cpp
 * \brief Реализация T-Digest для оценки квантилей
 * \author github: Sobig-F
 * \date 2026-02-15
 */

#include "tdigest.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace app::statistics {

// ==================== centroid implementation ====================

tdigest::centroid::centroid(double mean_, std::size_t count_) noexcept
    : _mean(mean_)
    , _count(count_)
{}

void tdigest::centroid::add(double value_) noexcept
{
    _mean = (_mean * _count + value_) / (_count + 1);
    ++_count;
}

void tdigest::centroid::merge(const centroid& other_) noexcept
{
    const auto total = _count + other_._count;
    _mean = (_mean * _count + other_._mean * other_._count) / total;
    _count = total;
}

// ==================== tdigest implementation ====================

tdigest::tdigest(std::size_t compression_)
    : _compression(compression_)
{
    if (_compression == 0) {
        throw std::invalid_argument{"Compression parameter must be positive"};
    }
    _centroids.reserve(_compression * 2);
}

double tdigest::max_weight(double q_) const noexcept
{
    return WEIGHT_MULTIPLIER * _compression * q_ * (1.0 - q_);
}

std::size_t tdigest::find_nearest_centroid(double value_) const noexcept
{
    if (_centroids.empty()) {
        return 0;
    }
    
    auto it = std::lower_bound(_centroids.begin(), _centroids.end(), value_,
        [](const centroid& c_, double val_) {
            return c_._mean < val_;
        });
    
    const std::size_t pos = it - _centroids.begin();
    
    if (pos == 0) return 0;
    if (pos == _centroids.size()) return _centroids.size() - 1;
    
    const double left_dist = std::abs(_centroids[pos - 1]._mean - value_);
    const double right_dist = std::abs(_centroids[pos]._mean - value_);
    
    return (left_dist < right_dist) ? (pos - 1) : pos;
}

std::size_t tdigest::total_count() const noexcept
{
    return _total_count;
}

void tdigest::add(double value_) noexcept
{
    if (value_ < _min_value) _min_value = value_;
    if (value_ > _max_value) _max_value = value_;
    
    if (_centroids.empty()) {
        _centroids.emplace_back(value_, 1);
        _total_count = 1;
        return;
    }
    
    const std::size_t best_idx = find_nearest_centroid(value_);
    
    double cumulative = 0.0;
    for (std::size_t i = 0; i < best_idx; ++i) {
        cumulative += _centroids[i]._count;
    }
    
    const double q = (cumulative + _centroids[best_idx]._count / 2.0) / 
                     static_cast<double>(_total_count + 1);
    
    if (_centroids[best_idx]._count + 1 <= max_weight(q)) {
        _centroids[best_idx].add(value_);
    } else {
        _centroids.emplace_back(value_, 1);
        std::sort(_centroids.begin(), _centroids.end(),
                    [](const centroid& a_, const centroid& b_) {
                        return a_._mean < b_._mean;
                    });
    }
    
    ++_total_count;
    
    if (_centroids.size() > _compression * 2) {
        compress();
    }
}

void tdigest::compress()
{
    if (_centroids.size() <= 1) return;
    
    std::sort(_centroids.begin(), _centroids.end(),
                    [](const centroid& a_, const centroid& b_) {
                        return a_._mean < b_._mean;
                    });
    
    std::vector<centroid> compressed;
    compressed.reserve(_compression);
    
    double cumulative = 0.0;
    
    for (const auto& c : _centroids) {
        if (compressed.empty()) {
            compressed.push_back(c);
            cumulative += c._count;
            continue;
        }
        
        auto& last = compressed.back();
        const double q = cumulative / static_cast<double>(_total_count);
        
        if (last._count + c._count <= max_weight(q)) {
            last.merge(c);
        } else {
            compressed.push_back(c);
        }
        
        cumulative += c._count;
    }
    
    _centroids = std::move(compressed);
}

double tdigest::quantile(double q_) const noexcept(false)
{
    if (q_ < 0.0 || q_ > 1.0) {
        throw std::invalid_argument{
            "Quantile must be in range [0, 1], got: " + std::to_string(q_)
        };
    }
    
    if (_centroids.empty()) {
        throw std::runtime_error{"Cannot compute quantile from empty digest"};
    }
    
    if (q_ == 0.0) return _min_value;
    if (q_ == 1.0) return _max_value;
    
    const double target = q_ * static_cast<double>(_total_count);
    double cumulative = 0.0;
    
    for (std::size_t i = 0; i < _centroids.size(); ++i) {
        const auto& c = _centroids[i];
        const double next = cumulative + static_cast<double>(c._count);
        
        if (target < next) {
            if (c._count == 1) {
                return c._mean;
            }
            
            const double left_bound = (i > 0) ? _centroids[i - 1]._mean : _min_value;
            const double right_bound = (i < _centroids.size() - 1) ? 
                                        _centroids[i + 1]._mean : _max_value;
            
            const double left_quantile = cumulative / static_cast<double>(_total_count);
            const double right_quantile = next / static_cast<double>(_total_count);
            const double t = (q_ - left_quantile) / (right_quantile - left_quantile);
            
            return left_bound + (right_bound - left_bound) * t;
        }
        
        cumulative = next;
    }
    
    return _centroids.back()._mean;
}

}  // namespace app::statistics