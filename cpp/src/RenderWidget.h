#pragma once

#include <QPointF>
#include <QWidget>

#include <vector>

#include "Model.h"

class QPainter;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

class RenderWidget : public QWidget {
  Q_OBJECT

 public:
  enum class SliceMode {
    None,
    X,
    Y,
    Z,
  };

  explicit RenderWidget(QWidget* parent = nullptr);

  QSize minimumSizeHint() const override;

  void setDimensions(int m, int n, int k);
  void setCountOnly(bool countOnly);
  void setArrangement(const std::vector<PiecePlacement>& arrangement, bool hasSelection);

  void setAlpha(double alpha);
  void setZoom(double zoom);
  void setSliceMode(SliceMode mode);
  void setSliceT(double t);

  double zoom() const;
  double alpha() const;
  double sliceT() const;
  SliceMode sliceMode() const;

 signals:
  void zoomAdjusted(double zoom);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  static double Clamp(double value, double minValue, double maxValue);
  double currentSliceMax() const;

  int m_dimX = 2;
  int m_dimY = 2;
  int m_dimZ = 2;

  bool m_hasSelection = false;
  bool m_countOnly = false;
  bool m_dragging = false;

  std::vector<PiecePlacement> m_arrangement;

  double m_rotX = -0.52;
  double m_rotY = 0.82;
  double m_zoom = 1.0;
  double m_alpha = 0.5;

  SliceMode m_sliceMode = SliceMode::None;
  double m_sliceT = 1.0;

  QPointF m_lastDragPos;
};
