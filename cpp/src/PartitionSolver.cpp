#include "PartitionSolver.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct DimsLess {
  bool operator()(const std::array<int, 3>& lhs, const std::array<int, 3>& rhs) const {
    if (lhs[0] != rhs[0]) {
      return lhs[0] < rhs[0];
    }
    if (lhs[1] != rhs[1]) {
      return lhs[1] < rhs[1];
    }
    return lhs[2] < rhs[2];
  }
};

struct PieceOption {
  int x = 0;
  int y = 0;
  int z = 0;
  int dx = 1;
  int dy = 1;
  int dz = 1;
  std::array<int, 3> normDims{1, 1, 1};
  std::vector<std::uint64_t> mask;
};

int CoordToId(const int x, const int y, const int z, const int m, const int n) {
  return x + m * (y + n * z);
}

std::array<int, 3> NormalizeDims(const int dx, const int dy, const int dz) {
  std::array<int, 3> dims{dx, dy, dz};
  std::sort(dims.begin(), dims.end());
  return dims;
}

std::string DimsToKey(const std::array<int, 3>& dims) {
  return std::to_string(dims[0]) + "x" + std::to_string(dims[1]) + "x" + std::to_string(dims[2]);
}

int CompareDims(const std::array<int, 3>& lhs, const std::array<int, 3>& rhs) {
  for (int i = 0; i < 3; ++i) {
    if (lhs[i] != rhs[i]) {
      return lhs[i] - rhs[i];
    }
  }
  return 0;
}

bool Overlaps(const std::vector<std::uint64_t>& occupancy, const std::vector<std::uint64_t>& mask) {
  for (std::size_t i = 0; i < occupancy.size(); ++i) {
    if ((occupancy[i] & mask[i]) != 0U) {
      return true;
    }
  }
  return false;
}

void ApplyOr(std::vector<std::uint64_t>& occupancy, const std::vector<std::uint64_t>& mask) {
  for (std::size_t i = 0; i < occupancy.size(); ++i) {
    occupancy[i] |= mask[i];
  }
}

void ApplyXor(std::vector<std::uint64_t>& occupancy, const std::vector<std::uint64_t>& mask) {
  for (std::size_t i = 0; i < occupancy.size(); ++i) {
    occupancy[i] ^= mask[i];
  }
}

int FirstEmpty(const std::vector<std::uint64_t>& occupancy, const int volume) {
  for (int id = 0; id < volume; ++id) {
    const std::uint64_t word = occupancy[static_cast<std::size_t>(id / 64)];
    const std::uint64_t bit = std::uint64_t{1} << (id % 64);
    if ((word & bit) == 0U) {
      return id;
    }
  }
  return -1;
}

bool PartitionLess(const PartitionResult& lhs, const PartitionResult& rhs) {
  if (lhs.pieceCount != rhs.pieceCount) {
    return lhs.pieceCount > rhs.pieceCount;
  }

  const std::size_t len = std::min(lhs.lexExpanded.size(), rhs.lexExpanded.size());
  for (std::size_t i = 0; i < len; ++i) {
    const int cmp = CompareDims(lhs.lexExpanded[i], rhs.lexExpanded[i]);
    if (cmp != 0) {
      return cmp < 0;
    }
  }

  return lhs.lexExpanded.size() < rhs.lexExpanded.size();
}

}  // namespace

SolveResult PartitionSolver::Solve(const SolveParams& params,
                                   std::atomic_bool& cancelRequested,
                                   const ProgressCallback& onProgress) {
  if (params.m <= 0 || params.n <= 0 || params.k <= 0 || params.maxSeconds <= 0) {
    throw std::runtime_error("Parametry musza byc dodatnimi liczbami.");
  }

  const int m = params.m;
  const int n = params.n;
  const int k = params.k;
  const int volume = m * n * k;
  const bool countOnly = params.countOnly;

  using Clock = std::chrono::steady_clock;
  const auto startTime = Clock::now();
  const auto deadline = startTime + std::chrono::seconds(params.maxSeconds);

  const int words = (volume + 63) / 64;
  std::vector<std::vector<PieceOption>> optionsByAnchor(static_cast<std::size_t>(volume));

  for (int z = 0; z < k; ++z) {
    for (int y = 0; y < n; ++y) {
      for (int x = 0; x < m; ++x) {
        const int anchor = CoordToId(x, y, z, m, n);

        for (int dx = 1; dx <= m - x; ++dx) {
          for (int dy = 1; dy <= n - y; ++dy) {
            for (int dz = 1; dz <= k - z; ++dz) {
              PieceOption option;
              option.x = x;
              option.y = y;
              option.z = z;
              option.dx = dx;
              option.dy = dy;
              option.dz = dz;
              option.normDims = NormalizeDims(dx, dy, dz);
              option.mask.assign(static_cast<std::size_t>(words), 0U);

              for (int zz = z; zz < z + dz; ++zz) {
                for (int yy = y; yy < y + dy; ++yy) {
                  for (int xx = x; xx < x + dx; ++xx) {
                    const int id = CoordToId(xx, yy, zz, m, n);
                    option.mask[static_cast<std::size_t>(id / 64)] |= std::uint64_t{1} << (id % 64);
                  }
                }
              }

              optionsByAnchor[static_cast<std::size_t>(anchor)].push_back(std::move(option));
            }
          }
        }
      }
    }
  }

  std::int64_t visitedNodes = 0;
  std::int64_t visitedTilings = 0;
  bool timedOut = false;

  std::unordered_map<std::string, PartitionResult> uniquePartitions;
  std::vector<const PieceOption*> path;
  std::vector<std::uint64_t> occupancy(static_cast<std::size_t>(words), 0U);

  auto elapsedMs = [&]() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime).count();
  };

  auto shouldStop = [&]() {
    if (cancelRequested.load()) {
      return true;
    }
    if (Clock::now() > deadline) {
      timedOut = true;
      cancelRequested.store(true);
      return true;
    }
    return false;
  };

  auto nextProgress = Clock::now() + std::chrono::milliseconds(250);
  auto postProgress = [&](const bool force) {
    if (!onProgress) {
      return;
    }

    const auto now = Clock::now();
    if (!force && now < nextProgress) {
      return;
    }

    nextProgress = now + std::chrono::milliseconds(250);

    ProgressUpdate update;
    update.elapsedMs = elapsedMs();
    update.visitedNodes = visitedNodes;
    update.visitedTilings = visitedTilings;
    update.uniqueCount = static_cast<int>(uniquePartitions.size());
    onProgress(update);
  };

  auto addSolution = [&]() {
    ++visitedTilings;

    std::map<std::array<int, 3>, int, DimsLess> counts;
    for (const PieceOption* piece : path) {
      counts[piece->normDims] += 1;
    }

    std::string signature;
    bool first = true;
    for (const auto& entry : counts) {
      if (!first) {
        signature.push_back('|');
      }
      first = false;
      signature += DimsToKey(entry.first);
      signature.push_back('^');
      signature += std::to_string(entry.second);
    }

    if (uniquePartitions.find(signature) != uniquePartitions.end()) {
      return;
    }

    PartitionResult partition;
    partition.signatureKey = signature;
    partition.pieceCount = static_cast<int>(path.size());

    for (const auto& entry : counts) {
      PartitionCount countEntry;
      countEntry.dims = entry.first;
      countEntry.count = entry.second;
      partition.counts.push_back(countEntry);

      for (int i = 0; i < entry.second; ++i) {
        partition.lexExpanded.push_back(entry.first);
      }
    }

    std::sort(partition.lexExpanded.begin(), partition.lexExpanded.end(), [](const auto& lhs, const auto& rhs) {
      return CompareDims(lhs, rhs) < 0;
    });

    if (!countOnly) {
      partition.arrangement.reserve(path.size());
      for (const PieceOption* piece : path) {
        PiecePlacement placement;
        placement.x = piece->x;
        placement.y = piece->y;
        placement.z = piece->z;
        placement.dx = piece->dx;
        placement.dy = piece->dy;
        placement.dz = piece->dz;
        placement.normDims = piece->normDims;
        partition.arrangement.push_back(placement);
      }
    }

    uniquePartitions.emplace(signature, std::move(partition));
  };

  std::function<void()> dfs = [&]() {
    if (shouldStop()) {
      return;
    }

    ++visitedNodes;
    if ((visitedNodes % 2048) == 0) {
      postProgress(false);
    }

    const int anchor = FirstEmpty(occupancy, volume);
    if (anchor == -1) {
      addSolution();
      postProgress(false);
      return;
    }

    const auto& options = optionsByAnchor[static_cast<std::size_t>(anchor)];
    for (const PieceOption& option : options) {
      if (Overlaps(occupancy, option.mask)) {
        continue;
      }

      ApplyOr(occupancy, option.mask);
      path.push_back(&option);
      dfs();
      path.pop_back();
      ApplyXor(occupancy, option.mask);

      if (cancelRequested.load()) {
        return;
      }
    }
  };

  dfs();
  postProgress(true);

  SolveResult result;
  result.m = m;
  result.n = n;
  result.k = k;
  result.countOnly = countOnly;
  result.volume = volume;
  result.elapsedMs = elapsedMs();
  result.visitedNodes = visitedNodes;
  result.visitedTilings = visitedTilings;
  result.timedOut = timedOut;

  const bool cancelled = cancelRequested.load();
  result.cancelledByUser = !timedOut && cancelled;
  result.completed = !cancelled || timedOut;

  result.partitions.reserve(uniquePartitions.size());
  for (auto& entry : uniquePartitions) {
    result.partitions.push_back(std::move(entry.second));
  }

  std::sort(result.partitions.begin(), result.partitions.end(), PartitionLess);
  result.uniqueCount = static_cast<int>(result.partitions.size());

  return result;
}
