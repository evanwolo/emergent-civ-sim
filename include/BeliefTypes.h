#ifndef BELIEF_TYPES_H
#define BELIEF_TYPES_H

#include <array>
#include <string>

constexpr int kDims = 4; // Authority, Tradition, Hierarchy, Faith

using BeliefVec = std::array<double, kDims>;

struct Personality {
  double openness;    // 0..1
  double charisma;    // 0..1
  double conformity;  // 0..1
};

struct AxesInfo {
  std::array<std::string, kDims> neg{{"Authority","Tradition","Hierarchy","Faith"}};
  std::array<std::string, kDims> pos{{"Liberty","Progress","Equality","Rationalism"}};
};

#endif
