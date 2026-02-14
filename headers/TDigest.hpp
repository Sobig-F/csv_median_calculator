#ifndef TDIGEST_HPP
#define TDIGEST_HPP

#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

class TDigest {
private:
    struct Centroid
    {
        double mean;
        size_t count;

        Centroid(double m = 0, size_t c = 0)
        :   mean(m),
            count(c) {}

        void add(double x) {
            mean = (mean * count + x) / (count + 1);
            count++;
        }

        void merge(const Centroid& other) {
            mean = (mean * count + other.mean * other.count) / (count + other.count);
            count += other.count;
        }
    };
    
    size_t compression;
    vector<Centroid> centroids;
    size_t total_count;
    double min_val;
    double max_val;

    double maxWeight(double q) const;
    void compress();

public:
    TDigest(size_t comp = 100)
    :   compression(comp),
        total_count(0),
        min_val(1e100),
        max_val(-1e100) {}

    void add(double x);
    double quantile(double q) const;
    double median() const;
};

#endif