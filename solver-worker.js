let cancelRequested = false;

self.onmessage = (event) => {
  const payload = event.data || {};
  if (payload.type === "cancel") {
    cancelRequested = true;
    return;
  }

  if (payload.type === "solve") {
    cancelRequested = false;
    try {
      solve(payload);
    } catch (error) {
      postMessage({
        type: "error",
        message: error instanceof Error ? error.message : String(error),
      });
    }
  }
};

function solve(payload) {
  const m = Number(payload.m);
  const n = Number(payload.n);
  const k = Number(payload.k);
  const maxSeconds = Number(payload.maxSeconds ?? 20);
  const countOnly = Boolean(payload.countOnly);

  if (![m, n, k, maxSeconds].every((value) => Number.isFinite(value) && value > 0)) {
    throw new Error("Parametry muszą być dodatnimi liczbami.");
  }

  const volume = m * n * k;
  const startMs = Date.now();
  const deadlineMs = startMs + Math.floor(maxSeconds * 1000);

  const coords = new Array(volume);
  for (let z = 0; z < k; z += 1) {
    for (let y = 0; y < n; y += 1) {
      for (let x = 0; x < m; x += 1) {
        const id = coordToId(x, y, z, m, n);
        coords[id] = { x, y, z };
      }
    }
  }

  const optionsByAnchor = Array.from({ length: volume }, () => []);

  for (let z = 0; z < k; z += 1) {
    for (let y = 0; y < n; y += 1) {
      for (let x = 0; x < m; x += 1) {
        const anchor = coordToId(x, y, z, m, n);

        for (let dx = 1; dx <= m - x; dx += 1) {
          for (let dy = 1; dy <= n - y; dy += 1) {
            for (let dz = 1; dz <= k - z; dz += 1) {
              let mask = 0n;
              for (let zz = z; zz < z + dz; zz += 1) {
                for (let yy = y; yy < y + dy; yy += 1) {
                  for (let xx = x; xx < x + dx; xx += 1) {
                    const id = coordToId(xx, yy, zz, m, n);
                    mask |= 1n << BigInt(id);
                  }
                }
              }

              optionsByAnchor[anchor].push({
                x,
                y,
                z,
                dx,
                dy,
                dz,
                mask,
                normKey: toNormKey(dx, dy, dz),
              });
            }
          }
        }
      }
    }
  }

  let visitedNodes = 0;
  let visitedTilings = 0;
  let timedOut = false;
  let nextProgressMs = Date.now() + 250;

  const uniquePartitions = new Map();
  const path = [];

  function shouldStop() {
    if (cancelRequested) {
      return true;
    }
    if (Date.now() > deadlineMs) {
      timedOut = true;
      cancelRequested = true;
      return true;
    }
    return false;
  }

  function postProgress(force) {
    const now = Date.now();
    if (!force && now < nextProgressMs) {
      return;
    }

    nextProgressMs = now + 250;
    postMessage({
      type: "progress",
      elapsedMs: now - startMs,
      visitedNodes,
      visitedTilings,
      uniqueCount: uniquePartitions.size,
    });
  }

  function addSolution() {
    visitedTilings += 1;

    const counts = new Map();
    for (const piece of path) {
      counts.set(piece.normKey, (counts.get(piece.normKey) ?? 0) + 1);
    }

    const sortedEntries = Array.from(counts.entries()).sort((a, b) =>
      compareNormKeys(a[0], b[0]),
    );

    const signatureKey = sortedEntries
      .map(([key, count]) => `${key}^${count}`)
      .join("|");

    if (uniquePartitions.has(signatureKey)) {
      return;
    }

    const countsList = sortedEntries.map(([key, count]) => ({
      dims: key.split("x").map(Number),
      count,
    }));

    const lexExpanded = [];
    for (const entry of countsList) {
      for (let i = 0; i < entry.count; i += 1) {
        lexExpanded.push(entry.dims.slice());
      }
    }
    lexExpanded.sort(compareDims);

    uniquePartitions.set(signatureKey, {
      signatureKey,
      counts: countsList,
      pieceCount: path.length,
      arrangement: countOnly
        ? []
        : path.map((piece) => ({
            x: piece.x,
            y: piece.y,
            z: piece.z,
            dx: piece.dx,
            dy: piece.dy,
            dz: piece.dz,
            normKey: piece.normKey,
          })),
      lexExpanded,
    });
  }

  function dfs(occupancy) {
    if (shouldStop()) {
      return;
    }

    visitedNodes += 1;
    if (visitedNodes % 2048 === 0) {
      postProgress(false);
    }

    const anchor = firstEmpty(occupancy, volume);
    if (anchor === -1) {
      addSolution();
      postProgress(false);
      return;
    }

    const options = optionsByAnchor[anchor];
    for (const option of options) {
      if ((occupancy & option.mask) !== 0n) {
        continue;
      }

      path.push(option);
      dfs(occupancy | option.mask);
      path.pop();

      if (cancelRequested) {
        return;
      }
    }
  }

  dfs(0n);

  postProgress(true);

  const partitions = Array.from(uniquePartitions.values());
  partitions.sort(comparePartitions);

  const outputPartitions = partitions.map((partition) => ({
    signatureKey: partition.signatureKey,
    counts: partition.counts,
    pieceCount: partition.pieceCount,
    arrangement: partition.arrangement,
  }));

  const completed = !cancelRequested || timedOut;

  postMessage({
    type: "done",
    m,
    n,
    k,
    countOnly,
    volume,
    elapsedMs: Date.now() - startMs,
    visitedNodes,
    visitedTilings,
    uniqueCount: outputPartitions.length,
    partitions: outputPartitions,
    timedOut,
    cancelledByUser: !timedOut && cancelRequested,
    completed,
  });
}

function coordToId(x, y, z, m, n) {
  return x + m * (y + n * z);
}

function firstEmpty(occupancy, volume) {
  for (let id = 0; id < volume; id += 1) {
    if (((occupancy >> BigInt(id)) & 1n) === 0n) {
      return id;
    }
  }
  return -1;
}

function toNormKey(dx, dy, dz) {
  const arr = [dx, dy, dz].sort((a, b) => a - b);
  return `${arr[0]}x${arr[1]}x${arr[2]}`;
}

function compareNormKeys(keyA, keyB) {
  const a = keyA.split("x").map(Number);
  const b = keyB.split("x").map(Number);
  return compareDims(a, b);
}

function compareDims(a, b) {
  for (let i = 0; i < 3; i += 1) {
    if (a[i] !== b[i]) {
      return a[i] - b[i];
    }
  }
  return 0;
}

function comparePartitions(a, b) {
  if (a.pieceCount !== b.pieceCount) {
    return b.pieceCount - a.pieceCount;
  }

  const len = Math.min(a.lexExpanded.length, b.lexExpanded.length);
  for (let i = 0; i < len; i += 1) {
    const cmp = compareDims(a.lexExpanded[i], b.lexExpanded[i]);
    if (cmp !== 0) {
      return cmp;
    }
  }

  return a.lexExpanded.length - b.lexExpanded.length;
}
