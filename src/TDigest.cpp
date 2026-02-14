#include "TDigest.hpp"

double TDigest::maxWeight(double q) const {
        if (q <= 0 || q >= 1) { return 1e100; };
        return 4 * compression * q * (1.0 - q);
    }

void TDigest::add(double x) {
    if (centroids.empty()) {
        centroids.emplace_back(Centroid(x, 1));
    
        total_count = 1;
        return;
    }

    if (x < min_val) min_val = x;
    if (x > max_val) max_val = x;

    auto it = lower_bound(centroids.begin(), centroids.end(), x,
                [](const Centroid& c, double val){ return c.mean < val; });

    size_t pos = it - centroids.begin();

    size_t best_idx = pos;
    if (pos == centroids.size()) {
        best_idx = pos - 1;
    } else if (pos == 0) {
        best_idx = 0;
    } else {
        double left = abs(centroids[pos - 1].mean - x);
        double right = abs(centroids[pos].mean - x);

        best_idx = (left < right) ? pos - 1 : pos;
    }

    double cumulative = 0;
    for (int i = 0; i < best_idx; ++i) { cumulative += centroids[i].count; }
    double q = (cumulative + centroids[best_idx].count / 2.0) / (total_count + 1);

    if (centroids[best_idx].count + 1 <= maxWeight(q)) {
        centroids[best_idx].add(x);
    } else {
        centroids.push_back(Centroid(x, 1));
        sort(centroids.begin(), centroids.end(),
            [](const Centroid& a, const Centroid& b){ return a.mean < b.mean; });
    }

    ++total_count;

    if (centroids.size() > compression * 2) {
        compress();
    }
}

void TDigest::compress() {
    if (centroids.size() <= 1) { return; };

    sort(centroids.begin(), centroids.end(),
        [](const Centroid& a, const Centroid& b) { return a.mean < b.mean; });

    vector<Centroid> compressed;
    compressed.reserve(compression);

    double cumulative = 0;
    double q;

    for (const Centroid& c : centroids) {
        if (compressed.empty()) {
            compressed.push_back(c);
            cumulative += c.count;
            continue;
        }

        Centroid& last = compressed.back();

        q = cumulative / (double)total_count;
        
        if (last.count + c.count <= maxWeight(q)) { last.merge(c); }
        else { compressed.push_back(c); }

        cumulative += c.count;
    }

    centroids = move(compressed);
}

double TDigest::quantile(double q) const {
    if (q <= 0) { return min_val; };
    if (q >= 1) { return max_val; };

    double target = q * total_count;
    double cumulative = 0;
    double next, delta, fraction;

    for (size_t i = 0; i < centroids.size(); ++i) {
        const Centroid& c = centroids[i];

        next = cumulative + c.count;

        if (target < next) {
            if (c.count == 1) { return c.mean; }

            delta = target - cumulative;
            fraction = delta / c.count;

            double left_bound = (i > 0) ? centroids[i - 1].mean : min_val;
            double right_bound = (i < centroids.size() - 1) ? centroids[i + 1].mean : max_val;

            // return left_bound + (right_bound - left_bound) * (cumulative + delta) / total_count;

            double left_quantile = cumulative / total_count;
            double right_quantile = next / total_count;
            double t = (q - left_quantile) / (right_quantile - left_quantile);
            return left_bound + (right_bound - left_bound) * t;
        }
        cumulative = next;
    }

    return centroids.back().mean;
}

double TDigest::median() const {
    return quantile(0.5);
}