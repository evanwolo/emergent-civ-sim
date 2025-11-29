#ifndef VALIDATION_H
#define VALIDATION_H

#include <cmath>
#include <stdexcept>
#include <string>
#include <cstdint>

// Compile-time validation toggle
#ifndef NDEBUG
#define VALIDATE_ENABLED 1
#else
#define VALIDATE_ENABLED 0
#endif

namespace validation {

// Check if value is finite (not NaN or Inf)
inline bool isFinite(double value) {
    return std::isfinite(value);
}

// Validate a double value is finite
inline void checkFinite(double value, const char* name) {
#if VALIDATE_ENABLED
    if (!std::isfinite(value)) {
        throw std::runtime_error(std::string("Non-finite value detected in ") + name + 
                                ": " + std::to_string(value));
    }
#endif
}

// Validate a value is in range [min, max]
inline void checkRange(double value, double min, double max, const char* name) {
#if VALIDATE_ENABLED
    if (!std::isfinite(value) || value < min || value > max) {
        throw std::runtime_error(std::string(name) + " out of range [" + 
                                std::to_string(min) + ", " + std::to_string(max) + 
                                "]: " + std::to_string(value));
    }
#endif
}

// Validate an index is in bounds
inline void checkIndex(std::uint32_t index, std::size_t size, const char* name) {
#if VALIDATE_ENABLED
    if (index >= size) {
        throw std::out_of_range(std::string(name) + " index out of bounds: " + 
                               std::to_string(index) + " >= " + std::to_string(size));
    }
#endif
}

// Validate a belief array for NaN/Inf
inline void checkBeliefs(const double* beliefs, std::size_t count, const char* context) {
#if VALIDATE_ENABLED
    for (std::size_t i = 0; i < count; ++i) {
        if (!std::isfinite(beliefs[i])) {
            throw std::runtime_error(std::string("Non-finite belief in ") + context + 
                                    " at index " + std::to_string(i) + 
                                    ": " + std::to_string(beliefs[i]));
        }
        if (beliefs[i] < -1.0 || beliefs[i] > 1.0) {
            throw std::runtime_error(std::string("Belief out of [-1,1] range in ") + context + 
                                    " at index " + std::to_string(i) + 
                                    ": " + std::to_string(beliefs[i]));
        }
    }
#endif
}

// Validate non-negative value
inline void checkNonNegative(double value, const char* name) {
#if VALIDATE_ENABLED
    if (!std::isfinite(value) || value < 0.0) {
        throw std::runtime_error(std::string(name) + " must be non-negative: " + 
                                std::to_string(value));
    }
#endif
}

// Validate positive value
inline void checkPositive(double value, const char* name) {
#if VALIDATE_ENABLED
    if (!std::isfinite(value) || value <= 0.0) {
        throw std::runtime_error(std::string(name) + " must be positive: " + 
                                std::to_string(value));
    }
#endif
}

// Trade flow conservation check (debugging)
inline void checkTradeConservation(double total_exports, double total_imports, double tolerance = 0.01) {
#if VALIDATE_ENABLED
    double imbalance = std::abs(total_exports - total_imports);
    if (imbalance > tolerance * std::max(total_exports, total_imports)) {
        throw std::runtime_error("Trade conservation violated: exports=" + 
                                std::to_string(total_exports) + 
                                ", imports=" + std::to_string(total_imports));
    }
#endif
}

}  // namespace validation

#endif // VALIDATION_H
