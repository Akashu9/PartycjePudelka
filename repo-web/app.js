const form = document.getElementById("solve-form");
const inputM = document.getElementById("input-m");
const inputN = document.getElementById("input-n");
const inputK = document.getElementById("input-k");
const inputTime = document.getElementById("input-time");
const countOnlyInput = document.getElementById("count-only");

const solveBtn = document.getElementById("solve-btn");
const cancelBtn = document.getElementById("cancel-btn");
const statusBox = document.getElementById("status");
const summaryBox = document.getElementById("summary");

const listEmpty = document.getElementById("partitions-empty");
const partitionsList = document.getElementById("partitions-list");

const vizTitle = document.getElementById("viz-title");
const canvas = document.getElementById("viz-canvas");
const alphaRange = document.getElementById("alpha-range");
const zoomRange = document.getElementById("zoom-range");
const sliceModeSelect = document.getElementById("slice-mode");
const sliceRange = document.getElementById("slice-range");

const ctx = canvas.getContext("2d");
const solverWorker = new Worker("./solver-worker.js");

const state = {
  isRunning: false,
  countOnly: false,
  partitions: [],
  selectedIndex: -1,
  dims: { m: 2, n: 2, k: 2 },
  renderer: {
    rotX: -0.52,
    rotY: 0.82,
    zoom: Number(zoomRange.value),
    alpha: Number(alphaRange.value),
    sliceMode: sliceModeSelect.value,
    sliceT: Number(sliceRange.value),
    dragging: false,
    dragLastX: 0,
    dragLastY: 0,
  },
};

solverWorker.onmessage = (event) => {
  const payload = event.data || {};

  if (payload.type === "progress") {
    setStatus(
      `Obliczanie... unikalnych: ${payload.uniqueCount}, odwiedzonych ułożeń: ${payload.visitedTilings}, węzłów: ${payload.visitedNodes}`,
      "running",
    );
    return;
  }

  if (payload.type === "error") {
    finishRun();
    setStatus(`Błąd: ${payload.message}`, "error");
    return;
  }

  if (payload.type === "done") {
    finishRun();

    state.partitions = payload.partitions || [];
    state.selectedIndex = -1;
    state.countOnly = Boolean(payload.countOnly);
    state.dims = { m: payload.m, n: payload.n, k: payload.k };
    updateSliceControl();

    const elapsed = formatMs(payload.elapsedMs);

    if (payload.timedOut) {
      setStatus(
        `Przerwano po limicie czasu (${elapsed}). Zebrano ${payload.uniqueCount} unikalnych partycji.`,
        "warn",
      );
    } else if (payload.cancelledByUser) {
      setStatus(
        `Przerwano ręcznie. Zebrano ${payload.uniqueCount} unikalnych partycji.`,
        "warn",
      );
    } else {
      setStatus(
        `Zakończono: ${payload.uniqueCount} unikalnych partycji w ${elapsed}.`,
        "ok",
      );
    }

    summaryBox.textContent = [
      `Wymiary: ${payload.m}×${payload.n}×${payload.k}`,
      `Objętość: ${payload.volume}`,
      `Unikalne partycje: ${payload.uniqueCount}`,
      `Odwiedzone ułożenia: ${payload.visitedTilings}`,
      `Węzły przeszukiwania: ${payload.visitedNodes}`,
      state.countOnly ? "Tryb: tylko zliczanie" : "Tryb: pełny (z ułożeniami)",
    ].join(" | ");

    renderPartitionList();

    if (state.partitions.length > 0) {
      selectPartition(0);
    } else {
      vizTitle.textContent = "Brak partycji do wizualizacji";
      drawScene();
    }
  }
};

form.addEventListener("submit", (event) => {
  event.preventDefault();

  if (state.isRunning) {
    return;
  }

  const m = Number(inputM.value);
  const n = Number(inputN.value);
  const k = Number(inputK.value);
  const maxSeconds = Number(inputTime.value);

  if (![m, n, k, maxSeconds].every((value) => Number.isFinite(value) && value > 0)) {
    setStatus("Podaj poprawne dodatnie liczby dla m, n, k i limitu czasu.", "error");
    return;
  }

  const volume = m * n * k;
  if (volume > 72) {
    setStatus(
      "Objętość jest bardzo duża; przeszukiwanie może być bardzo wolne. Rozważ mniejsze wymiary.",
      "warn",
    );
  }

  state.countOnly = Boolean(countOnlyInput.checked);
  state.partitions = [];
  state.selectedIndex = -1;
  state.dims = { m, n, k };

  listEmpty.textContent = "Obliczenia w toku...";
  partitionsList.innerHTML = "";

  startRun();

  solverWorker.postMessage({
    type: "solve",
    m,
    n,
    k,
    maxSeconds,
    countOnly: state.countOnly,
  });
});

cancelBtn.addEventListener("click", () => {
  if (!state.isRunning) {
    return;
  }
  solverWorker.postMessage({ type: "cancel" });
  setStatus("Wysyłam żądanie przerwania...", "warn");
});

alphaRange.addEventListener("input", () => {
  state.renderer.alpha = Number(alphaRange.value);
  drawScene();
});

zoomRange.addEventListener("input", () => {
  state.renderer.zoom = Number(zoomRange.value);
  drawScene();
});

sliceModeSelect.addEventListener("change", () => {
  state.renderer.sliceMode = sliceModeSelect.value;
  updateSliceControl();
  drawScene();
});

sliceRange.addEventListener("input", () => {
  state.renderer.sliceT = Number(sliceRange.value);
  drawScene();
});

canvas.addEventListener("pointerdown", (event) => {
  state.renderer.dragging = true;
  state.renderer.dragLastX = event.clientX;
  state.renderer.dragLastY = event.clientY;
  canvas.setPointerCapture(event.pointerId);
});

canvas.addEventListener("pointermove", (event) => {
  if (!state.renderer.dragging) {
    return;
  }

  const dx = event.clientX - state.renderer.dragLastX;
  const dy = event.clientY - state.renderer.dragLastY;
  state.renderer.dragLastX = event.clientX;
  state.renderer.dragLastY = event.clientY;

  state.renderer.rotY += dx * 0.01;
  state.renderer.rotX += dy * 0.01;
  state.renderer.rotX = clamp(state.renderer.rotX, -1.45, 1.45);

  drawScene();
});

canvas.addEventListener("pointerup", () => {
  state.renderer.dragging = false;
});

canvas.addEventListener("pointercancel", () => {
  state.renderer.dragging = false;
});

canvas.addEventListener(
  "wheel",
  (event) => {
    event.preventDefault();

    const direction = Math.sign(event.deltaY);
    state.renderer.zoom = clamp(state.renderer.zoom - direction * 0.08, 0.6, 2.2);
    zoomRange.value = state.renderer.zoom.toFixed(2);
    drawScene();
  },
  { passive: false },
);

window.addEventListener("resize", () => {
  drawScene();
});

function startRun() {
  state.isRunning = true;
  solveBtn.disabled = true;
  cancelBtn.disabled = false;
  setStatus("Start obliczeń...", "running");
  summaryBox.textContent = "";
}

function finishRun() {
  state.isRunning = false;
  solveBtn.disabled = false;
  cancelBtn.disabled = true;
}

function renderPartitionList() {
  partitionsList.innerHTML = "";

  if (state.partitions.length === 0) {
    listEmpty.style.display = "block";
    listEmpty.textContent = "Brak znalezionych partycji w podanym limicie.";
    return;
  }

  listEmpty.style.display = "none";

  state.partitions.forEach((partition, index) => {
    const li = document.createElement("li");
    const button = document.createElement("button");
    button.className = "partition-btn";

    const title = `Partycja ${index + 1}: ${formatCounts(partition.counts)}`;
    const subtitle = document.createElement("span");
    subtitle.className = "partition-sub";
    subtitle.textContent = `Liczba klocków: ${partition.pieceCount}`;

    button.textContent = title;
    button.appendChild(subtitle);

    button.addEventListener("click", () => {
      selectPartition(index);
    });

    li.appendChild(button);
    partitionsList.appendChild(li);
  });

  refreshActivePartitionButton();
}

function selectPartition(index) {
  if (index < 0 || index >= state.partitions.length) {
    return;
  }

  state.selectedIndex = index;
  refreshActivePartitionButton();

  const partition = state.partitions[index];
  vizTitle.textContent = `Partycja ${index + 1} | klocki: ${partition.pieceCount}`;

  drawScene();
}

function refreshActivePartitionButton() {
  const buttons = partitionsList.querySelectorAll(".partition-btn");
  buttons.forEach((button, idx) => {
    button.classList.toggle("active", idx === state.selectedIndex);
  });
}

function setStatus(message, kind) {
  statusBox.textContent = message;
  statusBox.classList.remove("idle", "running", "ok", "warn", "error");
  statusBox.classList.add(kind);
}

function formatCounts(counts) {
  const pieces = [];
  for (const entry of counts) {
    const dimsText = `(${entry.dims.join(",")})`;
    if (entry.count === 1) {
      pieces.push(dimsText);
    } else {
      pieces.push(`${dimsText}×${entry.count}`);
    }
  }
  return `{${pieces.join(", ")}}`;
}

function formatMs(ms) {
  if (ms < 1000) {
    return `${ms} ms`;
  }
  return `${(ms / 1000).toFixed(2)} s`;
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function updateSliceControl() {
  const mode = state.renderer.sliceMode;
  const { m, n, k } = state.dims;

  if (mode === "none") {
    sliceRange.disabled = true;
    sliceRange.min = "0";
    sliceRange.max = "1";
    sliceRange.step = "0.1";
    sliceRange.value = "1";
    state.renderer.sliceT = 1;
    return;
  }

  let maxValue = 1;
  if (mode === "x") {
    maxValue = m;
  } else if (mode === "y") {
    maxValue = n;
  } else if (mode === "z") {
    maxValue = k;
  }

  sliceRange.disabled = false;
  sliceRange.min = "0";
  sliceRange.max = String(maxValue);
  sliceRange.step = "0.05";
  state.renderer.sliceT = clamp(state.renderer.sliceT, 0, maxValue);
  sliceRange.value = state.renderer.sliceT.toFixed(2);
}

function resizeCanvasToDisplaySize() {
  const rect = canvas.getBoundingClientRect();
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const targetWidth = Math.round(rect.width * dpr);
  const targetHeight = Math.round(rect.height * dpr);

  if (canvas.width !== targetWidth || canvas.height !== targetHeight) {
    canvas.width = targetWidth;
    canvas.height = targetHeight;
  }
}

function drawScene() {
  resizeCanvasToDisplaySize();

  const width = canvas.width;
  const height = canvas.height;

  ctx.clearRect(0, 0, width, height);

  drawBackdrop(width, height);

  if (state.selectedIndex < 0 || state.selectedIndex >= state.partitions.length) {
    drawCenterText("Wybierz partycję, aby zobaczyć ułożenie.");
    return;
  }

  const partition = state.partitions[state.selectedIndex];
  if (state.countOnly || !partition.arrangement || partition.arrangement.length === 0) {
    drawCenterText("Tryb tylko zliczania nie zawiera geometrii do rysowania.");
    return;
  }

  const { m, n, k } = state.dims;
  const pieces = applySliceToArrangement(partition.arrangement, state.renderer.sliceMode, state.renderer.sliceT);

  if (pieces.length === 0) {
    drawCenterText("Przekrój odciął całą bryłę. Zwiększ t.");
    return;
  }

  const transformedFaces = [];
  let faceCounter = 0;

  pieces.forEach((piece, index) => {
    const cuboidFaces = buildCuboidFaces(piece);
    const baseColor = pieceColor(index);

    cuboidFaces.forEach((face) => {
      const projected = face.vertices.map((vertex) =>
        projectPoint(vertex, m, n, k, state.renderer.rotX, state.renderer.rotY, state.renderer.zoom, width, height),
      );

      if (projected.some((pt) => !Number.isFinite(pt.sx) || !Number.isFinite(pt.sy))) {
        return;
      }

      const avgDepth = projected.reduce((acc, pt) => acc + pt.depth, 0) / projected.length;

      transformedFaces.push({
        id: faceCounter,
        projected,
        avgDepth,
        color: faceColor(baseColor, face.shade, state.renderer.alpha),
      });
      faceCounter += 1;
    });
  });

  transformedFaces.sort((a, b) => a.avgDepth - b.avgDepth);

  for (const face of transformedFaces) {
    ctx.beginPath();
    ctx.moveTo(face.projected[0].sx, face.projected[0].sy);
    for (let i = 1; i < face.projected.length; i += 1) {
      ctx.lineTo(face.projected[i].sx, face.projected[i].sy);
    }
    ctx.closePath();
    ctx.fillStyle = face.color;
    ctx.fill();
    ctx.strokeStyle = "rgba(24, 42, 58, 0.45)";
    ctx.lineWidth = Math.max(1, Math.round(width * 0.0014));
    ctx.stroke();
  }

  drawOuterWireframe(m, n, k, width, height);
  drawSlicePlane(m, n, k, width, height);
}

function drawBackdrop(width, height) {
  const grad = ctx.createLinearGradient(0, 0, 0, height);
  grad.addColorStop(0, "#f9fcff");
  grad.addColorStop(1, "#dae4ee");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, width, height);
}

function drawCenterText(message) {
  ctx.fillStyle = "#415f78";
  ctx.font = `${Math.max(15, Math.round(canvas.width * 0.02))}px \"Avenir Next\", \"Trebuchet MS\", sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(message, canvas.width / 2, canvas.height / 2);
}

function pieceColor(index) {
  const hue = 194 + ((index * 37) % 70);
  const sat = 54 + ((index * 11) % 14);
  const light = 48 + ((index * 7) % 12);
  return { h: hue, s: sat, l: light };
}

function faceColor(baseColor, shade, alpha) {
  const l = clamp(baseColor.l * shade, 18, 78);
  return `hsla(${baseColor.h} ${baseColor.s}% ${l}% / ${alpha})`;
}

function buildCuboidFaces(piece) {
  const x0 = piece.x;
  const y0 = piece.y;
  const z0 = piece.z;
  const x1 = piece.x + piece.dx;
  const y1 = piece.y + piece.dy;
  const z1 = piece.z + piece.dz;

  const v = [
    { x: x0, y: y0, z: z0 },
    { x: x1, y: y0, z: z0 },
    { x: x1, y: y1, z: z0 },
    { x: x0, y: y1, z: z0 },
    { x: x0, y: y0, z: z1 },
    { x: x1, y: y0, z: z1 },
    { x: x1, y: y1, z: z1 },
    { x: x0, y: y1, z: z1 },
  ];

  return [
    { vertices: [v[0], v[1], v[2], v[3]], shade: 0.74 },
    { vertices: [v[4], v[5], v[6], v[7]], shade: 1.0 },
    { vertices: [v[0], v[1], v[5], v[4]], shade: 0.86 },
    { vertices: [v[1], v[2], v[6], v[5]], shade: 0.92 },
    { vertices: [v[2], v[3], v[7], v[6]], shade: 0.82 },
    { vertices: [v[3], v[0], v[4], v[7]], shade: 0.9 },
  ];
}

function applySliceToArrangement(arrangement, mode, t) {
  if (mode === "none") {
    return arrangement.map((piece) => ({ ...piece }));
  }

  const result = [];

  for (const piece of arrangement) {
    let x0 = piece.x;
    let y0 = piece.y;
    let z0 = piece.z;
    let x1 = piece.x + piece.dx;
    let y1 = piece.y + piece.dy;
    let z1 = piece.z + piece.dz;

    if (mode === "x") {
      if (t <= x0) {
        continue;
      }
      x1 = Math.min(x1, t);
    } else if (mode === "y") {
      if (t <= y0) {
        continue;
      }
      y1 = Math.min(y1, t);
    } else if (mode === "z") {
      if (t <= z0) {
        continue;
      }
      z1 = Math.min(z1, t);
    }

    if (x1 <= x0 || y1 <= y0 || z1 <= z0) {
      continue;
    }

    result.push({
      x: x0,
      y: y0,
      z: z0,
      dx: x1 - x0,
      dy: y1 - y0,
      dz: z1 - z0,
      normKey: piece.normKey,
    });
  }

  return result;
}

function projectPoint(point, m, n, k, rotX, rotY, zoom, width, height) {
  const maxDim = Math.max(m, n, k);

  const centered = {
    x: point.x - m / 2,
    y: point.z - k / 2,
    z: point.y - n / 2,
  };

  const yRot = rotateY(centered, rotY);
  const xRot = rotateX(yRot, rotX);

  const cam = maxDim * 4.8;
  const depth = cam - xRot.z;
  const persp = cam / Math.max(0.2, depth);

  const scale = (Math.min(width, height) * 0.36 * zoom) / Math.max(1, maxDim);

  return {
    sx: width / 2 + xRot.x * scale * persp,
    sy: height / 2 - xRot.y * scale * persp,
    depth: xRot.z,
  };
}

function rotateY(point, angle) {
  const c = Math.cos(angle);
  const s = Math.sin(angle);
  return {
    x: point.x * c + point.z * s,
    y: point.y,
    z: -point.x * s + point.z * c,
  };
}

function rotateX(point, angle) {
  const c = Math.cos(angle);
  const s = Math.sin(angle);
  return {
    x: point.x,
    y: point.y * c - point.z * s,
    z: point.y * s + point.z * c,
  };
}

function drawOuterWireframe(m, n, k, width, height) {
  const corners = [
    { x: 0, y: 0, z: 0 },
    { x: m, y: 0, z: 0 },
    { x: m, y: n, z: 0 },
    { x: 0, y: n, z: 0 },
    { x: 0, y: 0, z: k },
    { x: m, y: 0, z: k },
    { x: m, y: n, z: k },
    { x: 0, y: n, z: k },
  ].map((point) =>
    projectPoint(
      point,
      m,
      n,
      k,
      state.renderer.rotX,
      state.renderer.rotY,
      state.renderer.zoom,
      width,
      height,
    ),
  );

  const edges = [
    [0, 1],
    [1, 2],
    [2, 3],
    [3, 0],
    [4, 5],
    [5, 6],
    [6, 7],
    [7, 4],
    [0, 4],
    [1, 5],
    [2, 6],
    [3, 7],
  ];

  ctx.strokeStyle = "rgba(19, 40, 58, 0.5)";
  ctx.lineWidth = Math.max(1, Math.round(width * 0.0017));

  for (const [a, b] of edges) {
    ctx.beginPath();
    ctx.moveTo(corners[a].sx, corners[a].sy);
    ctx.lineTo(corners[b].sx, corners[b].sy);
    ctx.stroke();
  }
}

function drawSlicePlane(m, n, k, width, height) {
  const mode = state.renderer.sliceMode;
  if (mode === "none") {
    return;
  }

  const t = state.renderer.sliceT;
  let plane = null;

  if (mode === "x") {
    plane = [
      { x: t, y: 0, z: 0 },
      { x: t, y: n, z: 0 },
      { x: t, y: n, z: k },
      { x: t, y: 0, z: k },
    ];
  } else if (mode === "y") {
    plane = [
      { x: 0, y: t, z: 0 },
      { x: m, y: t, z: 0 },
      { x: m, y: t, z: k },
      { x: 0, y: t, z: k },
    ];
  } else if (mode === "z") {
    plane = [
      { x: 0, y: 0, z: t },
      { x: m, y: 0, z: t },
      { x: m, y: n, z: t },
      { x: 0, y: n, z: t },
    ];
  }

  if (!plane) {
    return;
  }

  const projected = plane.map((point) =>
    projectPoint(
      point,
      m,
      n,
      k,
      state.renderer.rotX,
      state.renderer.rotY,
      state.renderer.zoom,
      width,
      height,
    ),
  );

  ctx.beginPath();
  ctx.moveTo(projected[0].sx, projected[0].sy);
  for (let i = 1; i < projected.length; i += 1) {
    ctx.lineTo(projected[i].sx, projected[i].sy);
  }
  ctx.closePath();
  ctx.fillStyle = "rgba(231, 151, 95, 0.12)";
  ctx.fill();
  ctx.strokeStyle = "rgba(168, 92, 39, 0.45)";
  ctx.lineWidth = Math.max(1, Math.round(width * 0.0015));
  ctx.stroke();
}

updateSliceControl();
drawScene();
