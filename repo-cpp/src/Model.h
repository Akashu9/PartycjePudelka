#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct PiecePlacement {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double dx = 1.0;
  double dy = 1.0;
  double dz = 1.0;
  std::array<int, 3> normDims{1, 1, 1};
};

struct PartitionCount {
  std::array<int, 3> dims{1, 1, 1};
  int count = 0;
};

struct PartitionResult {
  std::string signatureKey;
  std::vector<PartitionCount> counts;
  int pieceCount = 0;
  std::vector<PiecePlacement> arrangement;
  std::vector<std::array<int, 3>> lexExpanded;
};

struct SolveParams {
  int m = 2;
  int n = 2;
  int k = 2;
  int maxSeconds = 20;
  bool countOnly = false;
};

struct ProgressUpdate {
  std::int64_t elapsedMs = 0;
  std::int64_t visitedNodes = 0;
  std::int64_t visitedTilings = 0;
  int uniqueCount = 0;
};

struct SolveResult {
  int m = 0;
  int n = 0;
  int k = 0;
  bool countOnly = false;
  int volume = 0;
  std::int64_t elapsedMs = 0;
  std::int64_t visitedNodes = 0;
  std::int64_t visitedTilings = 0;
  int uniqueCount = 0;
  std::vector<PartitionResult> partitions;
  bool timedOut = false;
  bool cancelledByUser = false;
  bool completed = false;
};
