#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <string>
#include <fstream>
#include <cstdint>
#include <vector>
#include <array>

// Forward declarations
struct Agent;
struct KernelConfig;
class Kernel;

namespace serialization {

// Magic number and version for checkpoint files
constexpr std::uint32_t CHECKPOINT_MAGIC = 0x45435356;  // "VCSE" in hex
constexpr std::uint32_t CHECKPOINT_VERSION = 1;

// Checkpoint header
struct CheckpointHeader {
    std::uint32_t magic = CHECKPOINT_MAGIC;
    std::uint32_t version = CHECKPOINT_VERSION;
    std::uint64_t generation = 0;
    std::uint32_t num_agents = 0;
    std::uint32_t num_regions = 0;
    std::uint64_t seed = 0;
    std::uint64_t timestamp = 0;  // Unix timestamp when saved
};

// Save simulation state to file
bool saveCheckpoint(const Kernel& kernel, const std::string& filepath);

// Load simulation state from file
bool loadCheckpoint(Kernel& kernel, const std::string& filepath);

// Helper functions for binary I/O
template<typename T>
void writeBinary(std::ofstream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
void readBinary(std::ifstream& in, T& value) {
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
}

// Write array
template<typename T, std::size_t N>
void writeBinaryArray(std::ofstream& out, const std::array<T, N>& arr) {
    out.write(reinterpret_cast<const char*>(arr.data()), sizeof(T) * N);
}

template<typename T, std::size_t N>
void readBinaryArray(std::ifstream& in, std::array<T, N>& arr) {
    in.read(reinterpret_cast<char*>(arr.data()), sizeof(T) * N);
}

// Write vector
template<typename T>
void writeBinaryVector(std::ofstream& out, const std::vector<T>& vec) {
    std::uint32_t size = static_cast<std::uint32_t>(vec.size());
    writeBinary(out, size);
    if (size > 0) {
        out.write(reinterpret_cast<const char*>(vec.data()), sizeof(T) * size);
    }
}

template<typename T>
void readBinaryVector(std::ifstream& in, std::vector<T>& vec) {
    std::uint32_t size;
    readBinary(in, size);
    vec.resize(size);
    if (size > 0) {
        in.read(reinterpret_cast<char*>(vec.data()), sizeof(T) * size);
    }
}

// Write string
inline void writeBinaryString(std::ofstream& out, const std::string& str) {
    std::uint32_t size = static_cast<std::uint32_t>(str.size());
    writeBinary(out, size);
    if (size > 0) {
        out.write(str.data(), size);
    }
}

inline void readBinaryString(std::ifstream& in, std::string& str) {
    std::uint32_t size;
    readBinary(in, size);
    str.resize(size);
    if (size > 0) {
        in.read(&str[0], size);
    }
}

}  // namespace serialization

#endif // SERIALIZATION_H
