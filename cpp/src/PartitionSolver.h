#pragma once

#include <atomic>
#include <functional>

#include "Model.h"

class PartitionSolver {
 public:
  using ProgressCallback = std::function<void(const ProgressUpdate&)>;

  static SolveResult Solve(const SolveParams& params,
                           std::atomic_bool& cancelRequested,
                           const ProgressCallback& onProgress);
};
