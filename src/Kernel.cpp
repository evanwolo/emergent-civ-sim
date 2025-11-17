#include "Kernel.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_set>

Kernel::Kernel(const KernelConfig& cfg) : cfg_(cfg), rng_(cfg.seed) {
    reset(cfg);
}

void Kernel::reset(const KernelConfig& cfg) {
    cfg_ = cfg;
    generation_ = 0;
    rng_.seed(cfg.seed);
    initAgents();
    buildSmallWorld();
}

void Kernel::initAgents() {
    agents_.clear();
    agents_.reserve(cfg_.population);
    regionIndex_.assign(cfg_.regions, {});
    
    std::normal_distribution<double> xDist(0.0, 0.75);
    std::normal_distribution<double> traitDist(0.5, 0.15);
    std::uniform_real_distribution<double> uniDist(0.0, 1.0);
    std::uniform_int_distribution<std::uint32_t> regionDist(0, cfg_.regions - 1);
    std::uniform_int_distribution<std::uint8_t> langDist(0, 3);  // 4 base languages
    
    for (std::uint32_t i = 0; i < cfg_.population; ++i) {
        Agent a;
        a.id = i;
        a.region = regionDist(rng_);
        a.primaryLang = langDist(rng_);
        a.fluency = std::clamp(0.7 + 0.3 * (uniDist(rng_) - 0.5), 0.3, 1.0);
        
        // Personality traits
        a.openness = std::clamp(traitDist(rng_), 0.0, 1.0);
        a.conformity = std::clamp(traitDist(rng_), 0.0, 1.0);
        a.assertiveness = std::clamp(traitDist(rng_), 0.0, 1.0);
        a.sociality = std::clamp(traitDist(rng_), 0.0, 1.0);
        
        // Initial beliefs
        for (int k = 0; k < 4; ++k) {
            a.x[k] = xDist(rng_);
            a.B[k] = fastTanh(a.x[k]);
        }
        
        // Module multipliers (initialized; modules will update)
        a.m_comm = 1.0;
        a.m_susceptibility = 0.7 + 0.6 * (a.openness - 0.5);
        a.m_susceptibility = std::clamp(a.m_susceptibility, 0.4, 1.2);
        a.m_mobility = 0.8 + 0.4 * a.sociality;
        
        regionIndex_[a.region].push_back(i);
        agents_.push_back(std::move(a));
    }
}

void Kernel::buildSmallWorld() {
    const std::uint32_t N = cfg_.population;
    std::uint32_t K = cfg_.avgConnections;
    if (K % 2) ++K;  // ensure even
    const std::uint32_t halfK = K / 2;
    
    std::uniform_real_distribution<double> uniDist(0.0, 1.0);
    std::uniform_int_distribution<std::uint32_t> nodeDist(0, N - 1);
    
    // Ring lattice
    for (std::uint32_t i = 0; i < N; ++i) {
        for (std::uint32_t d = 1; d <= halfK; ++d) {
            std::uint32_t j = (i + d) % N;
            agents_[i].neighbors.push_back(j);
            agents_[j].neighbors.push_back(i);
        }
    }
    
    // Rewiring
    for (std::uint32_t i = 0; i < N; ++i) {
        std::unordered_set<std::uint32_t> current(agents_[i].neighbors.begin(), 
                                                    agents_[i].neighbors.end());
        for (std::uint32_t d = 1; d <= halfK; ++d) {
            if (uniDist(rng_) < cfg_.rewireProb) {
                std::uint32_t oldJ = (i + d) % N;
                
                // Remove old edge
                auto& niNbrs = agents_[i].neighbors;
                auto& njNbrs = agents_[oldJ].neighbors;
                niNbrs.erase(std::remove(niNbrs.begin(), niNbrs.end(), oldJ), niNbrs.end());
                njNbrs.erase(std::remove(njNbrs.begin(), njNbrs.end(), i), njNbrs.end());
                current.erase(oldJ);
                
                // Find new target
                std::uint32_t newJ;
                do {
                    newJ = nodeDist(rng_);
                } while (newJ == i || current.count(newJ));
                
                // Add new edge
                agents_[i].neighbors.push_back(newJ);
                agents_[newJ].neighbors.push_back(i);
                current.insert(newJ);
            }
        }
    }
    
    // Deduplicate and remove self-loops
    for (auto& agent : agents_) {
        std::unordered_set<std::uint32_t> unique;
        std::vector<std::uint32_t> cleaned;
        for (auto nid : agent.neighbors) {
            if (nid != agent.id && unique.insert(nid).second) {
                cleaned.push_back(nid);
            }
        }
        agent.neighbors = std::move(cleaned);
    }
}

double Kernel::languageQuality(const Agent& i, const Agent& j) const {
    if (i.primaryLang == j.primaryLang) {
        return std::min(i.fluency, j.fluency);
    } else {
        // Cross-lingual influence attenuates
        return 0.25 * std::min(i.fluency, j.fluency);
    }
}

double Kernel::similarityGate(const Agent& i, const Agent& j) const {
    // Cosine similarity in belief space
    double dot = 0.0, ni = 0.0, nj = 0.0;
    for (int k = 0; k < 4; ++k) {
        dot += i.B[k] * j.B[k];
        ni += i.B[k] * i.B[k];
        nj += j.B[k] * j.B[k];
    }
    
    double sim = 0.0;
    if (ni > 0 && nj > 0) {
        sim = dot / (std::sqrt(ni) * std::sqrt(nj));  // -1..1
    }
    sim = 0.5 * (sim + 1.0);  // normalize to 0..1
    return std::max(sim, cfg_.simFloor);
}

void Kernel::updateBeliefs() {
    // Compute deltas in parallel-friendly way
    std::vector<std::array<double, 4>> dx(agents_.size());
    
    for (std::size_t i = 0; i < agents_.size(); ++i) {
        const auto& ai = agents_[i];
        std::array<double, 4> acc{0, 0, 0, 0};
        
        for (auto jid : ai.neighbors) {
            const auto& aj = agents_[jid];
            
            double s = similarityGate(ai, aj);
            double lq = languageQuality(ai, aj);
            double comm = 0.5 * (ai.m_comm + aj.m_comm);
            double weight = cfg_.stepSize * s * lq * comm * ai.m_susceptibility;
            
            for (int k = 0; k < 4; ++k) {
                double diff = aj.B[k] - ai.B[k];
                acc[k] += weight * fastTanh(diff);
            }
        }
        
        dx[i] = acc;
    }
    
    // Apply updates
    for (std::size_t i = 0; i < agents_.size(); ++i) {
        for (int k = 0; k < 4; ++k) {
            agents_[i].x[k] += dx[i][k];
            agents_[i].B[k] = fastTanh(agents_[i].x[k]);
        }
    }
}

void Kernel::step() {
    updateBeliefs();
    ++generation_;
}

void Kernel::stepN(int n) {
    for (int i = 0; i < n; ++i) {
        step();
    }
}

Kernel::Metrics Kernel::computeMetrics() const {
    Metrics m;
    
    // Compute region centroids
    std::vector<std::array<double, 4>> centroids(cfg_.regions);
    std::vector<int> counts(cfg_.regions, 0);
    
    for (std::uint32_t r = 0; r < cfg_.regions; ++r) {
        std::array<double, 4> c{0, 0, 0, 0};
        for (auto id : regionIndex_[r]) {
            for (int k = 0; k < 4; ++k) {
                c[k] += agents_[id].B[k];
            }
        }
        int n = static_cast<int>(regionIndex_[r].size());
        if (n > 0) {
            for (int k = 0; k < 4; ++k) {
                c[k] /= n;
            }
        }
        centroids[r] = c;
        counts[r] = n;
    }
    
    // Pairwise distances between centroids
    std::vector<double> dists;
    for (std::uint32_t i = 0; i < cfg_.regions; ++i) {
        if (counts[i] == 0) continue;
        for (std::uint32_t j = i + 1; j < cfg_.regions; ++j) {
            if (counts[j] == 0) continue;
            double d = 0.0;
            for (int k = 0; k < 4; ++k) {
                double dd = centroids[i][k] - centroids[j][k];
                d += dd * dd;
            }
            dists.push_back(std::sqrt(d));
        }
    }
    
    if (!dists.empty()) {
        m.polarizationMean = std::accumulate(dists.begin(), dists.end(), 0.0) / dists.size();
        double sq = 0.0;
        for (double v : dists) {
            sq += (v - m.polarizationMean) * (v - m.polarizationMean);
        }
        m.polarizationStd = std::sqrt(sq / dists.size());
    }
    
    // Average traits
    m.avgOpenness = 0.0;
    m.avgConformity = 0.0;
    for (const auto& a : agents_) {
        m.avgOpenness += a.openness;
        m.avgConformity += a.conformity;
    }
    m.avgOpenness /= agents_.size();
    m.avgConformity /= agents_.size();
    
    return m;
}
