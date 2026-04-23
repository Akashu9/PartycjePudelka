#pragma once

#include <QMainWindow>
#include <QThread>

#include <atomic>
#include <string>

#include "Model.h"
#include "RenderWidget.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QSpinBox;

class SolverThread : public QThread {
  Q_OBJECT

 public:
  explicit SolverThread(QObject* parent = nullptr);

  void requestCancel();

  SolveParams params;
  SolveResult result;
  QString errorText;

 signals:
  void progressAvailable(qint64 elapsedMs, qint64 visitedNodes, qint64 visitedTilings, int uniqueCount);

 protected:
  void run() override;

 private:
  std::atomic_bool m_cancelRequested{false};
};

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

 private:
  enum class StatusKind {
    Idle,
    Running,
    Ok,
    Warn,
    Error,
  };

  void setupUi();
  void setupConnections();

  void startSolve();
  void finishSolve();
  void handleSolveFinished();

  void renderPartitionList();
  void selectPartition(int index);
  void updateSliceControl();

  void setStatus(const QString& message, StatusKind kind);

  static QString formatCounts(const std::vector<PartitionCount>& counts);
  static QString formatMs(std::int64_t ms);

  QSpinBox* m_inputM = nullptr;
  QSpinBox* m_inputN = nullptr;
  QSpinBox* m_inputK = nullptr;
  QSpinBox* m_inputTime = nullptr;

  QCheckBox* m_countOnlyInput = nullptr;

  QPushButton* m_solveButton = nullptr;
  QPushButton* m_cancelButton = nullptr;

  QLabel* m_statusLabel = nullptr;
  QLabel* m_summaryLabel = nullptr;

  QLabel* m_listEmptyLabel = nullptr;
  QListWidget* m_partitionsList = nullptr;

  QLabel* m_vizTitleLabel = nullptr;
  RenderWidget* m_renderWidget = nullptr;

  QSlider* m_alphaSlider = nullptr;
  QSlider* m_zoomSlider = nullptr;
  QComboBox* m_sliceModeCombo = nullptr;
  QSlider* m_sliceSlider = nullptr;

  bool m_isRunning = false;
  SolveResult m_result;
  SolverThread* m_solverThread = nullptr;

  int m_dimM = 2;
  int m_dimN = 2;
  int m_dimK = 2;
};
