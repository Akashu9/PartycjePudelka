#include "RenderWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPolygonF>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct Face3D {
  std::array<Vec3, 4> vertices;
  double shade = 1.0;
};

struct ProjectedPoint {
  double sx = 0.0;
  double sy = 0.0;
  double depth = 0.0;
};

struct TransformedFace {
  QPolygonF polygon;
  double avgDepth = 0.0;
  QColor color;
};

struct HslColor {
  double h = 0.0;
  double s = 0.0;
  double l = 0.0;
};

Vec3 RotateY(const Vec3& point, const double angle) {
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  return {
      point.x * c + point.z * s,
      point.y,
      -point.x * s + point.z * c,
  };
}

Vec3 RotateX(const Vec3& point, const double angle) {
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  return {
      point.x,
      point.y * c - point.z * s,
      point.y * s + point.z * c,
  };
}

ProjectedPoint ProjectPoint(const Vec3& point,
                            const int m,
                            const int n,
                            const int k,
                            const double rotX,
                            const double rotY,
                            const double zoom,
                            const int width,
                            const int height) {
  const int maxDim = std::max({m, n, k});

  const Vec3 centered{
      point.x - static_cast<double>(m) / 2.0,
      point.z - static_cast<double>(k) / 2.0,
      point.y - static_cast<double>(n) / 2.0,
  };

  const Vec3 yRotated = RotateY(centered, rotY);
  const Vec3 xRotated = RotateX(yRotated, rotX);

  const double cam = static_cast<double>(maxDim) * 4.8;
  const double depth = cam - xRotated.z;
  const double persp = cam / std::max(0.2, depth);

  const double scale = (std::min(width, height) * 0.36 * zoom) / std::max(1, maxDim);

  return {
      static_cast<double>(width) / 2.0 + xRotated.x * scale * persp,
      static_cast<double>(height) / 2.0 - xRotated.y * scale * persp,
      xRotated.z,
  };
}

HslColor PieceColor(const int index) {
  const double h = 194.0 + ((index * 37) % 70);
  const double s = 54.0 + ((index * 11) % 14);
  const double l = 48.0 + ((index * 7) % 12);
  return {h, s, l};
}

double Clamp(const double value, const double minValue, const double maxValue) {
  return std::min(maxValue, std::max(minValue, value));
}

QColor FaceColor(const HslColor& baseColor, const double shade, const double alpha) {
  const double lightness = Clamp(baseColor.l * shade, 18.0, 78.0);
  const QColor color = QColor::fromHslF(baseColor.h / 360.0, baseColor.s / 100.0, lightness / 100.0, alpha);
  return color;
}

std::vector<Face3D> BuildCuboidFaces(const PiecePlacement& piece) {
  const double x0 = piece.x;
  const double y0 = piece.y;
  const double z0 = piece.z;
  const double x1 = piece.x + piece.dx;
  const double y1 = piece.y + piece.dy;
  const double z1 = piece.z + piece.dz;

  const std::array<Vec3, 8> vertices{{
      {x0, y0, z0},
      {x1, y0, z0},
      {x1, y1, z0},
      {x0, y1, z0},
      {x0, y0, z1},
      {x1, y0, z1},
      {x1, y1, z1},
      {x0, y1, z1},
  }};

  return {
      {{vertices[0], vertices[1], vertices[2], vertices[3]}, 0.74},
      {{vertices[4], vertices[5], vertices[6], vertices[7]}, 1.0},
      {{vertices[0], vertices[1], vertices[5], vertices[4]}, 0.86},
      {{vertices[1], vertices[2], vertices[6], vertices[5]}, 0.92},
      {{vertices[2], vertices[3], vertices[7], vertices[6]}, 0.82},
      {{vertices[3], vertices[0], vertices[4], vertices[7]}, 0.9},
  };
}

std::vector<PiecePlacement> ApplySliceToArrangement(const std::vector<PiecePlacement>& arrangement,
                                                    const RenderWidget::SliceMode mode,
                                                    const double t) {
  if (mode == RenderWidget::SliceMode::None) {
    return arrangement;
  }

  std::vector<PiecePlacement> result;
  result.reserve(arrangement.size());

  for (const PiecePlacement& piece : arrangement) {
    double x0 = piece.x;
    double y0 = piece.y;
    double z0 = piece.z;

    double x1 = piece.x + piece.dx;
    double y1 = piece.y + piece.dy;
    double z1 = piece.z + piece.dz;

    if (mode == RenderWidget::SliceMode::X) {
      if (t <= x0) {
        continue;
      }
      x1 = std::min(x1, t);
    } else if (mode == RenderWidget::SliceMode::Y) {
      if (t <= y0) {
        continue;
      }
      y1 = std::min(y1, t);
    } else if (mode == RenderWidget::SliceMode::Z) {
      if (t <= z0) {
        continue;
      }
      z1 = std::min(z1, t);
    }

    if (x1 <= x0 || y1 <= y0 || z1 <= z0) {
      continue;
    }

    PiecePlacement clipped = piece;
    clipped.x = x0;
    clipped.y = y0;
    clipped.z = z0;
    clipped.dx = x1 - x0;
    clipped.dy = y1 - y0;
    clipped.dz = z1 - z0;
    result.push_back(clipped);
  }

  return result;
}

}  // namespace

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
}

QSize RenderWidget::minimumSizeHint() const {
  return QSize(520, 380);
}

void RenderWidget::setDimensions(const int m, const int n, const int k) {
  m_dimX = std::max(1, m);
  m_dimY = std::max(1, n);
  m_dimZ = std::max(1, k);
  m_sliceT = Clamp(m_sliceT, 0.0, currentSliceMax());
  update();
}

void RenderWidget::setCountOnly(const bool countOnly) {
  m_countOnly = countOnly;
  update();
}

void RenderWidget::setArrangement(const std::vector<PiecePlacement>& arrangement, const bool hasSelection) {
  m_arrangement = arrangement;
  m_hasSelection = hasSelection;
  update();
}

void RenderWidget::setAlpha(const double alpha) {
  m_alpha = Clamp(alpha, 0.1, 0.95);
  update();
}

void RenderWidget::setZoom(const double zoom) {
  m_zoom = Clamp(zoom, 0.6, 2.2);
  update();
}

void RenderWidget::setSliceMode(const SliceMode mode) {
  m_sliceMode = mode;
  m_sliceT = Clamp(m_sliceT, 0.0, currentSliceMax());
  update();
}

void RenderWidget::setSliceT(const double t) {
  m_sliceT = Clamp(t, 0.0, currentSliceMax());
  update();
}

double RenderWidget::zoom() const {
  return m_zoom;
}

double RenderWidget::alpha() const {
  return m_alpha;
}

double RenderWidget::sliceT() const {
  return m_sliceT;
}

RenderWidget::SliceMode RenderWidget::sliceMode() const {
  return m_sliceMode;
}

double RenderWidget::Clamp(const double value, const double minValue, const double maxValue) {
  return std::min(maxValue, std::max(minValue, value));
}

double RenderWidget::currentSliceMax() const {
  if (m_sliceMode == SliceMode::X) {
    return m_dimX;
  }
  if (m_sliceMode == SliceMode::Y) {
    return m_dimY;
  }
  if (m_sliceMode == SliceMode::Z) {
    return m_dimZ;
  }
  return 1.0;
}

void RenderWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
  gradient.setColorAt(0.0, QColor("#f9fcff"));
  gradient.setColorAt(1.0, QColor("#dae4ee"));
  painter.fillRect(rect(), gradient);

  auto drawCenterText = [&](const QString& message) {
    painter.setPen(QColor("#415f78"));
    QFont font("Avenir Next", std::max(11, width() / 54));
    painter.setFont(font);
    painter.drawText(rect(), Qt::AlignCenter, message);
  };

  if (!m_hasSelection) {
    drawCenterText("Wybierz partycje, aby zobaczyc ulozenie.");
    return;
  }

  if (m_countOnly || m_arrangement.empty()) {
    drawCenterText("Tryb tylko zliczania nie zawiera geometrii do rysowania.");
    return;
  }

  const std::vector<PiecePlacement> pieces = ApplySliceToArrangement(m_arrangement, m_sliceMode, m_sliceT);
  if (pieces.empty()) {
    drawCenterText("Przekroj odcial cala bryle. Zwieksz t.");
    return;
  }

  std::vector<TransformedFace> transformedFaces;
  transformedFaces.reserve(pieces.size() * 6);

  for (std::size_t index = 0; index < pieces.size(); ++index) {
    const HslColor baseColor = PieceColor(static_cast<int>(index));
    const auto cuboidFaces = BuildCuboidFaces(pieces[index]);

    for (const Face3D& face : cuboidFaces) {
      QPolygonF polygon;
      double depthSum = 0.0;
      bool valid = true;

      for (const Vec3& vertex : face.vertices) {
        const ProjectedPoint projected = ProjectPoint(vertex,
                                                      m_dimX,
                                                      m_dimY,
                                                      m_dimZ,
                                                      m_rotX,
                                                      m_rotY,
                                                      m_zoom,
                                                      width(),
                                                      height());
        if (!std::isfinite(projected.sx) || !std::isfinite(projected.sy)) {
          valid = false;
          break;
        }

        polygon << QPointF(projected.sx, projected.sy);
        depthSum += projected.depth;
      }

      if (!valid) {
        continue;
      }

      transformedFaces.push_back({
          polygon,
          depthSum / static_cast<double>(face.vertices.size()),
          FaceColor(baseColor, face.shade, m_alpha),
      });
    }
  }

  std::sort(transformedFaces.begin(), transformedFaces.end(),
            [](const TransformedFace& lhs, const TransformedFace& rhs) {
              return lhs.avgDepth < rhs.avgDepth;
            });

  QPen facePen(QColor(24, 42, 58, 115));
  facePen.setWidthF(std::max(1.0, width() * 0.0014));

  for (const TransformedFace& face : transformedFaces) {
    painter.setPen(facePen);
    painter.setBrush(face.color);
    painter.drawPolygon(face.polygon);
  }

  const std::array<Vec3, 8> corners{{
      {0.0, 0.0, 0.0},
      {static_cast<double>(m_dimX), 0.0, 0.0},
      {static_cast<double>(m_dimX), static_cast<double>(m_dimY), 0.0},
      {0.0, static_cast<double>(m_dimY), 0.0},
      {0.0, 0.0, static_cast<double>(m_dimZ)},
      {static_cast<double>(m_dimX), 0.0, static_cast<double>(m_dimZ)},
      {static_cast<double>(m_dimX), static_cast<double>(m_dimY), static_cast<double>(m_dimZ)},
      {0.0, static_cast<double>(m_dimY), static_cast<double>(m_dimZ)},
  }};

  std::array<ProjectedPoint, 8> projectedCorners{};
  for (std::size_t i = 0; i < corners.size(); ++i) {
    projectedCorners[i] = ProjectPoint(corners[i],
                                       m_dimX,
                                       m_dimY,
                                       m_dimZ,
                                       m_rotX,
                                       m_rotY,
                                       m_zoom,
                                       width(),
                                       height());
  }

  const std::array<std::array<int, 2>, 12> edges{{
      {0, 1},
      {1, 2},
      {2, 3},
      {3, 0},
      {4, 5},
      {5, 6},
      {6, 7},
      {7, 4},
      {0, 4},
      {1, 5},
      {2, 6},
      {3, 7},
  }};

  QPen wirePen(QColor(19, 40, 58, 128));
  wirePen.setWidthF(std::max(1.0, width() * 0.0017));
  painter.setPen(wirePen);
  painter.setBrush(Qt::NoBrush);

  for (const auto& edge : edges) {
    painter.drawLine(QPointF(projectedCorners[edge[0]].sx, projectedCorners[edge[0]].sy),
                     QPointF(projectedCorners[edge[1]].sx, projectedCorners[edge[1]].sy));
  }

  if (m_sliceMode != SliceMode::None) {
    std::array<Vec3, 4> plane{};

    if (m_sliceMode == SliceMode::X) {
      plane = {{
          {m_sliceT, 0.0, 0.0},
          {m_sliceT, static_cast<double>(m_dimY), 0.0},
          {m_sliceT, static_cast<double>(m_dimY), static_cast<double>(m_dimZ)},
          {m_sliceT, 0.0, static_cast<double>(m_dimZ)},
      }};
    } else if (m_sliceMode == SliceMode::Y) {
      plane = {{
          {0.0, m_sliceT, 0.0},
          {static_cast<double>(m_dimX), m_sliceT, 0.0},
          {static_cast<double>(m_dimX), m_sliceT, static_cast<double>(m_dimZ)},
          {0.0, m_sliceT, static_cast<double>(m_dimZ)},
      }};
    } else if (m_sliceMode == SliceMode::Z) {
      plane = {{
          {0.0, 0.0, m_sliceT},
          {static_cast<double>(m_dimX), 0.0, m_sliceT},
          {static_cast<double>(m_dimX), static_cast<double>(m_dimY), m_sliceT},
          {0.0, static_cast<double>(m_dimY), m_sliceT},
      }};
    }

    QPolygonF projectedPlane;
    for (const Vec3& point : plane) {
      const ProjectedPoint projected = ProjectPoint(point,
                                                    m_dimX,
                                                    m_dimY,
                                                    m_dimZ,
                                                    m_rotX,
                                                    m_rotY,
                                                    m_zoom,
                                                    width(),
                                                    height());
      projectedPlane << QPointF(projected.sx, projected.sy);
    }

    painter.setPen(QPen(QColor(168, 92, 39, 115), std::max(1.0, width() * 0.0015)));
    painter.setBrush(QColor(231, 151, 95, 31));
    painter.drawPolygon(projectedPlane);
  }
}

void RenderWidget::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_dragging = true;
    m_lastDragPos = event->position();
    setCursor(Qt::ClosedHandCursor);
  }
  QWidget::mousePressEvent(event);
}

void RenderWidget::mouseMoveEvent(QMouseEvent* event) {
  if (m_dragging) {
    const QPointF current = event->position();
    const double dx = current.x() - m_lastDragPos.x();
    const double dy = current.y() - m_lastDragPos.y();
    m_lastDragPos = current;

    m_rotY += dx * 0.01;
    m_rotX += dy * 0.01;
    m_rotX = Clamp(m_rotX, -1.45, 1.45);

    update();
  }
  QWidget::mouseMoveEvent(event);
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_dragging = false;
    unsetCursor();
  }
  QWidget::mouseReleaseEvent(event);
}

void RenderWidget::wheelEvent(QWheelEvent* event) {
  if (event->angleDelta().y() == 0) {
    QWidget::wheelEvent(event);
    return;
  }

  const int direction = event->angleDelta().y() > 0 ? -1 : 1;
  const double nextZoom = Clamp(m_zoom - (direction * 0.08), 0.6, 2.2);

  if (std::abs(nextZoom - m_zoom) > 1e-9) {
    m_zoom = nextZoom;
    emit zoomAdjusted(m_zoom);
    update();
  }

  event->accept();
}
