#include "MainWindow.h"

#include "PartitionSolver.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <exception>

SolverThread::SolverThread(QObject* parent) : QThread(parent) {}

void SolverThread::requestCancel() {
  m_cancelRequested.store(true);
}

void SolverThread::run() {
  m_cancelRequested.store(false);
  errorText.clear();
  result = {};

  try {
    result = PartitionSolver::Solve(
        params,
        m_cancelRequested,
        [this](const ProgressUpdate& update) {
          emit progressAvailable(update.elapsedMs,
                                 update.visitedNodes,
                                 update.visitedTilings,
                                 update.uniqueCount);
        });
  } catch (const std::exception& ex) {
    errorText = QString::fromUtf8(ex.what());
  } catch (...) {
    errorText = QStringLiteral("Nieznany blad podczas obliczen.");
  }
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setupUi();
  setupConnections();
  updateSliceControl();
  setStatus(QStringLiteral("Gotowe do uruchomienia."), StatusKind::Idle);
}

MainWindow::~MainWindow() {
  if (m_solverThread != nullptr) {
    m_solverThread->requestCancel();
    m_solverThread->wait(3000);
  }
}

void MainWindow::setupUi() {
  setWindowTitle(QStringLiteral("Partycje Prostopadloscianu - C++"));
  resize(1360, 820);

  setStyleSheet(QStringLiteral(
      "QMainWindow { background: #edf3f8; }"
      "QWidget { color: #1f425b; }"
      "QGroupBox {"
      "  font-weight: 600;"
      "  color: #27465e;"
      "  border: 1px solid #c8d8e5;"
      "  border-radius: 12px;"
      "  margin-top: 0.9em;"
      "  background: #f8fbfe;"
      "}"
      "QGroupBox::title {"
      "  subcontrol-origin: margin;"
      "  left: 12px;"
      "  padding: 0 4px;"
      "  color: #27465e;"
      "}"
      "QLabel {"
      "  color: #1f425b;"
      "}"
      "QCheckBox {"
      "  color: #1f425b;"
      "}"
      "QSpinBox, QComboBox {"
      "  color: #18364a;"
      "  background: #ffffff;"
      "  border: 1px solid #c8d8e5;"
      "  border-radius: 8px;"
      "  padding: 4px 6px;"
      "}"
      "QPushButton {"
      "  color: #1f425b;"
      "  border: 1px solid #6289a6;"
      "  border-radius: 8px;"
      "  padding: 6px 10px;"
      "  background: #e9f1f7;"
      "}"
      "QPushButton:disabled {"
      "  color: #7d8e9e;"
      "  background: #eef3f8;"
      "  border-color: #c7d2dc;"
      "}"
      "QPushButton#solveButton {"
      "  background: #2f6f98;"
      "  border-color: #225572;"
      "  color: #f8fbff;"
      "}"
      "QListWidget {"
      "  border: 1px solid #c8d8e5;"
      "  border-radius: 10px;"
      "  background: #ffffff;"
      "}"
      "QListWidget::item {"
      "  border-bottom: 1px solid #e0e8ef;"
      "  padding: 8px;"
      "}"
      "QListWidget::item:selected {"
      "  background: #dbe9f3;"
      "  color: #132a3e;"
      "}"
      "QLabel#vizTitle { color: #436179; }"));

  auto* central = new QWidget(this);
  setCentralWidget(central);

  auto* rootLayout = new QHBoxLayout(central);
  rootLayout->setContentsMargins(12, 12, 12, 12);
  rootLayout->setSpacing(12);

  auto* splitter = new QSplitter(Qt::Horizontal, central);
  rootLayout->addWidget(splitter);

  auto* leftPanel = new QWidget(splitter);
  leftPanel->setMinimumWidth(360);
  auto* leftLayout = new QVBoxLayout(leftPanel);
  leftLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->setSpacing(10);

  auto* paramsBox = new QGroupBox(QStringLiteral("Parametry"), leftPanel);
  auto* paramsLayout = new QVBoxLayout(paramsBox);

  auto* formLayout = new QFormLayout();
  formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  m_inputM = new QSpinBox(paramsBox);
  m_inputN = new QSpinBox(paramsBox);
  m_inputK = new QSpinBox(paramsBox);
  m_inputTime = new QSpinBox(paramsBox);

  for (QSpinBox* input : {m_inputM, m_inputN, m_inputK}) {
    input->setRange(1, 12);
    input->setValue(2);
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
  }

  m_inputTime->setRange(1, 120);
  m_inputTime->setValue(20);
  m_inputTime->setButtonSymbols(QAbstractSpinBox::NoButtons);

  formLayout->addRow(QStringLiteral("m"), m_inputM);
  formLayout->addRow(QStringLiteral("n"), m_inputN);
  formLayout->addRow(QStringLiteral("k"), m_inputK);
  formLayout->addRow(QStringLiteral("Limit czasu (s)"), m_inputTime);

  paramsLayout->addLayout(formLayout);

  m_countOnlyInput = new QCheckBox(QStringLiteral("Tylko zliczanie (bez zapisu ulozen)"), paramsBox);
  paramsLayout->addWidget(m_countOnlyInput);

  auto* buttonsLayout = new QHBoxLayout();
  m_solveButton = new QPushButton(QStringLiteral("Oblicz partycje"), paramsBox);
  m_solveButton->setObjectName(QStringLiteral("solveButton"));
  m_cancelButton = new QPushButton(QStringLiteral("Przerwij"), paramsBox);
  m_cancelButton->setEnabled(false);

  buttonsLayout->addWidget(m_solveButton);
  buttonsLayout->addWidget(m_cancelButton);
  paramsLayout->addLayout(buttonsLayout);

  m_statusLabel = new QLabel(paramsBox);
  m_statusLabel->setWordWrap(true);
  m_statusLabel->setMargin(8);
  paramsLayout->addWidget(m_statusLabel);

  m_summaryLabel = new QLabel(paramsBox);
  m_summaryLabel->setWordWrap(true);
  m_summaryLabel->setMargin(8);
  m_summaryLabel->setStyleSheet(QStringLiteral(
      "background: #e8f0f7;"
      "color: #1f425b;"
      "border: 1px solid #c2d4e3;"
      "border-radius: 8px;"));
  paramsLayout->addWidget(m_summaryLabel);

  auto* hintLabel = new QLabel(
      QStringLiteral("Dla wiekszych wymiarow liczba rozkladow rosnie bardzo szybko. "
                     "Najstabilniej dla objetosci do okolo 27 komorek."),
      paramsBox);
  hintLabel->setWordWrap(true);
  hintLabel->setMargin(8);
  hintLabel->setStyleSheet(QStringLiteral(
      "background: #eef4fa;"
      "color: #35556f;"
      "border: 1px solid #d2dfeb;"
      "border-radius: 8px;"));
  paramsLayout->addWidget(hintLabel);

  leftLayout->addWidget(paramsBox);

  auto* listBox = new QGroupBox(QStringLiteral("Partycje"), leftPanel);
  auto* listLayout = new QVBoxLayout(listBox);

  m_listEmptyLabel = new QLabel(QStringLiteral("Uruchom obliczenia, aby zobaczyc wyniki."), listBox);
  m_listEmptyLabel->setWordWrap(true);
  listLayout->addWidget(m_listEmptyLabel);

  m_partitionsList = new QListWidget(listBox);
  m_partitionsList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_partitionsList->hide();
  listLayout->addWidget(m_partitionsList, 1);

  leftLayout->addWidget(listBox, 1);

  auto* rightPanel = new QWidget(splitter);
  auto* rightLayout = new QVBoxLayout(rightPanel);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(10);

  auto* headerLayout = new QHBoxLayout();
  auto* vizHeader = new QLabel(QStringLiteral("Wizualizacja 3D"), rightPanel);
  QFont headerFont = vizHeader->font();
  headerFont.setPointSize(headerFont.pointSize() + 3);
  headerFont.setBold(true);
  vizHeader->setFont(headerFont);

  m_vizTitleLabel = new QLabel(QStringLiteral("Brak danych"), rightPanel);
  m_vizTitleLabel->setObjectName(QStringLiteral("vizTitle"));

  headerLayout->addWidget(vizHeader);
  headerLayout->addStretch(1);
  headerLayout->addWidget(m_vizTitleLabel);
  rightLayout->addLayout(headerLayout);

  auto* controlsBox = new QGroupBox(QStringLiteral("Sterowanie widokiem"), rightPanel);
  auto* controlsLayout = new QGridLayout(controlsBox);

  auto* alphaLabel = new QLabel(QStringLiteral("Przezroczystosc"), controlsBox);
  m_alphaSlider = new QSlider(Qt::Horizontal, controlsBox);
  m_alphaSlider->setRange(10, 95);
  m_alphaSlider->setSingleStep(5);
  m_alphaSlider->setValue(50);

  auto* zoomLabel = new QLabel(QStringLiteral("Zoom"), controlsBox);
  m_zoomSlider = new QSlider(Qt::Horizontal, controlsBox);
  m_zoomSlider->setRange(60, 220);
  m_zoomSlider->setSingleStep(5);
  m_zoomSlider->setValue(100);

  auto* sliceModeLabel = new QLabel(QStringLiteral("Przekroj"), controlsBox);
  m_sliceModeCombo = new QComboBox(controlsBox);
  m_sliceModeCombo->addItem(QStringLiteral("Brak"), static_cast<int>(RenderWidget::SliceMode::None));
  m_sliceModeCombo->addItem(QStringLiteral("X <= t"), static_cast<int>(RenderWidget::SliceMode::X));
  m_sliceModeCombo->addItem(QStringLiteral("Y <= t"), static_cast<int>(RenderWidget::SliceMode::Y));
  m_sliceModeCombo->addItem(QStringLiteral("Z <= t"), static_cast<int>(RenderWidget::SliceMode::Z));

  auto* sliceTLabel = new QLabel(QStringLiteral("t"), controlsBox);
  m_sliceSlider = new QSlider(Qt::Horizontal, controlsBox);
  m_sliceSlider->setRange(0, 100);
  m_sliceSlider->setValue(100);
  m_sliceSlider->setEnabled(false);

  controlsLayout->addWidget(alphaLabel, 0, 0);
  controlsLayout->addWidget(m_alphaSlider, 0, 1);
  controlsLayout->addWidget(zoomLabel, 0, 2);
  controlsLayout->addWidget(m_zoomSlider, 0, 3);
  controlsLayout->addWidget(sliceModeLabel, 1, 0);
  controlsLayout->addWidget(m_sliceModeCombo, 1, 1);
  controlsLayout->addWidget(sliceTLabel, 1, 2);
  controlsLayout->addWidget(m_sliceSlider, 1, 3);

  rightLayout->addWidget(controlsBox);

  m_renderWidget = new RenderWidget(rightPanel);
  m_renderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  rightLayout->addWidget(m_renderWidget, 1);

  auto* helpLabel = new QLabel(QStringLiteral("Przeciagnij myszka, aby obrocic. Kolko myszy zmienia zoom."), rightPanel);
  helpLabel->setStyleSheet(QStringLiteral("color: #4a6a83;"));
  rightLayout->addWidget(helpLabel);

  splitter->addWidget(leftPanel);
  splitter->addWidget(rightPanel);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_renderWidget->setDimensions(m_dimM, m_dimN, m_dimK);
  m_renderWidget->setCountOnly(false);
  m_renderWidget->setArrangement({}, false);
}

void MainWindow::setupConnections() {
  connect(m_solveButton, &QPushButton::clicked, this, &MainWindow::startSolve);

  connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
    if (!m_isRunning || m_solverThread == nullptr) {
      return;
    }
    m_solverThread->requestCancel();
    setStatus(QStringLiteral("Wysylam zadanie przerwania..."), StatusKind::Warn);
  });

  connect(m_partitionsList, &QListWidget::currentRowChanged, this, &MainWindow::selectPartition);

  connect(m_alphaSlider, &QSlider::valueChanged, this, [this](const int value) {
    m_renderWidget->setAlpha(static_cast<double>(value) / 100.0);
  });

  connect(m_zoomSlider, &QSlider::valueChanged, this, [this](const int value) {
    m_renderWidget->setZoom(static_cast<double>(value) / 100.0);
  });

  connect(m_renderWidget, &RenderWidget::zoomAdjusted, this, [this](const double zoom) {
    const int sliderValue = static_cast<int>(std::round(zoom * 100.0));
    if (m_zoomSlider->value() == sliderValue) {
      return;
    }
    const QSignalBlocker blocker(m_zoomSlider);
    m_zoomSlider->setValue(sliderValue);
  });

  connect(m_sliceModeCombo,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this](const int) { updateSliceControl(); });

  connect(m_sliceSlider, &QSlider::valueChanged, this, [this](const int value) {
    m_renderWidget->setSliceT(static_cast<double>(value) / 100.0);
  });
}

void MainWindow::startSolve() {
  if (m_isRunning) {
    return;
  }

  SolveParams params;
  params.m = m_inputM->value();
  params.n = m_inputN->value();
  params.k = m_inputK->value();
  params.maxSeconds = m_inputTime->value();
  params.countOnly = m_countOnlyInput->isChecked();

  if (params.m <= 0 || params.n <= 0 || params.k <= 0 || params.maxSeconds <= 0) {
    setStatus(QStringLiteral("Podaj poprawne dodatnie liczby dla m, n, k i limitu czasu."), StatusKind::Error);
    return;
  }

  m_dimM = params.m;
  m_dimN = params.n;
  m_dimK = params.k;

  m_result = {};
  m_partitionsList->clear();
  m_partitionsList->hide();
  m_listEmptyLabel->setText(QStringLiteral("Obliczenia w toku..."));
  m_listEmptyLabel->show();

  m_renderWidget->setDimensions(m_dimM, m_dimN, m_dimK);
  m_renderWidget->setCountOnly(params.countOnly);
  m_renderWidget->setArrangement({}, false);

  m_vizTitleLabel->setText(QStringLiteral("Brak danych"));
  m_summaryLabel->clear();

  updateSliceControl();

  m_solverThread = new SolverThread(this);
  m_solverThread->params = params;

  connect(m_solverThread,
          &SolverThread::progressAvailable,
          this,
          [this](const qint64,
                 const qint64 visitedNodes,
                 const qint64 visitedTilings,
                 const int uniqueCount) {
            if (!m_isRunning) {
              return;
            }
            setStatus(QStringLiteral("Obliczanie... unikalnych: %1, odwiedzonych ulozen: %2, wezlow: %3")
                          .arg(uniqueCount)
                          .arg(visitedTilings)
                          .arg(visitedNodes),
                      StatusKind::Running);
          });

  connect(m_solverThread, &QThread::finished, this, &MainWindow::handleSolveFinished);

  m_isRunning = true;
  m_solveButton->setEnabled(false);
  m_cancelButton->setEnabled(true);

  if (params.m * params.n * params.k > 72) {
    setStatus(QStringLiteral("Objetosc jest duza; obliczenia moga byc bardzo wolne."), StatusKind::Warn);
  } else {
    setStatus(QStringLiteral("Start obliczen..."), StatusKind::Running);
  }

  m_solverThread->start();
}

void MainWindow::finishSolve() {
  m_isRunning = false;
  m_solveButton->setEnabled(true);
  m_cancelButton->setEnabled(false);
}

void MainWindow::handleSolveFinished() {
  SolverThread* thread = m_solverThread;
  if (thread == nullptr) {
    return;
  }
  m_solverThread = nullptr;

  finishSolve();

  if (!thread->errorText.isEmpty()) {
    setStatus(QStringLiteral("Blad: %1").arg(thread->errorText), StatusKind::Error);
    thread->deleteLater();
    return;
  }

  m_result = std::move(thread->result);

  m_dimM = m_result.m;
  m_dimN = m_result.n;
  m_dimK = m_result.k;

  m_renderWidget->setDimensions(m_dimM, m_dimN, m_dimK);
  m_renderWidget->setCountOnly(m_result.countOnly);
  updateSliceControl();

  const QString elapsed = formatMs(m_result.elapsedMs);

  if (m_result.timedOut) {
    setStatus(QStringLiteral("Przerwano po limicie czasu (%1). Zebrano %2 unikalnych partycji.")
                  .arg(elapsed)
                  .arg(m_result.uniqueCount),
              StatusKind::Warn);
  } else if (m_result.cancelledByUser) {
    setStatus(QStringLiteral("Przerwano recznie. Zebrano %1 unikalnych partycji.").arg(m_result.uniqueCount),
              StatusKind::Warn);
  } else {
    setStatus(QStringLiteral("Zakonczono: %1 unikalnych partycji w %2.")
                  .arg(m_result.uniqueCount)
                  .arg(elapsed),
              StatusKind::Ok);
  }

  QStringList summaryParts;
  summaryParts << QStringLiteral("Wymiary: %1x%2x%3").arg(m_result.m).arg(m_result.n).arg(m_result.k)
               << QStringLiteral("Objetosc: %1").arg(m_result.volume)
               << QStringLiteral("Unikalne partycje: %1").arg(m_result.uniqueCount)
               << QStringLiteral("Odwiedzone ulozenia: %1").arg(m_result.visitedTilings)
               << QStringLiteral("Wezly przeszukiwania: %1").arg(m_result.visitedNodes)
               << (m_result.countOnly ? QStringLiteral("Tryb: tylko zliczanie")
                                      : QStringLiteral("Tryb: pelny (z ulozeniami)"));
  m_summaryLabel->setText(summaryParts.join(QStringLiteral(" | ")));

  renderPartitionList();

  if (!m_result.partitions.empty()) {
    m_partitionsList->setCurrentRow(0);
  } else {
    m_vizTitleLabel->setText(QStringLiteral("Brak partycji do wizualizacji"));
    m_renderWidget->setArrangement({}, false);
  }

  thread->deleteLater();
}

void MainWindow::renderPartitionList() {
  m_partitionsList->clear();

  if (m_result.partitions.empty()) {
    m_partitionsList->hide();
    m_listEmptyLabel->setText(QStringLiteral("Brak znalezionych partycji w podanym limicie."));
    m_listEmptyLabel->show();
    return;
  }

  m_listEmptyLabel->hide();
  m_partitionsList->show();

  for (std::size_t i = 0; i < m_result.partitions.size(); ++i) {
    const PartitionResult& partition = m_result.partitions[i];

    const QString title = QStringLiteral("Partycja %1: %2")
                              .arg(static_cast<int>(i) + 1)
                              .arg(formatCounts(partition.counts));
    const QString subtitle = QStringLiteral("Liczba klockow: %1").arg(partition.pieceCount);

    auto* item = new QListWidgetItem(title + QStringLiteral("\n") + subtitle, m_partitionsList);
    item->setData(Qt::UserRole, static_cast<int>(i));
  }
}

void MainWindow::selectPartition(const int index) {
  if (index < 0 || index >= static_cast<int>(m_result.partitions.size())) {
    m_vizTitleLabel->setText(QStringLiteral("Brak danych"));
    m_renderWidget->setArrangement({}, false);
    return;
  }

  const PartitionResult& partition = m_result.partitions[static_cast<std::size_t>(index)];
  m_vizTitleLabel->setText(
      QStringLiteral("Partycja %1 | klocki: %2").arg(index + 1).arg(partition.pieceCount));
  m_renderWidget->setArrangement(partition.arrangement, true);
}

void MainWindow::updateSliceControl() {
  const auto mode =
      static_cast<RenderWidget::SliceMode>(m_sliceModeCombo->currentData().toInt());
  m_renderWidget->setSliceMode(mode);

  if (mode == RenderWidget::SliceMode::None) {
    const QSignalBlocker blocker(m_sliceSlider);
    m_sliceSlider->setEnabled(false);
    m_sliceSlider->setRange(0, 100);
    m_sliceSlider->setValue(100);
    m_renderWidget->setSliceT(1.0);
    return;
  }

  double maxValue = 1.0;
  if (mode == RenderWidget::SliceMode::X) {
    maxValue = m_dimM;
  } else if (mode == RenderWidget::SliceMode::Y) {
    maxValue = m_dimN;
  } else if (mode == RenderWidget::SliceMode::Z) {
    maxValue = m_dimK;
  }

  const int maxSliderValue = std::max(1, static_cast<int>(std::lround(maxValue * 100.0)));

  int newValue = 0;
  {
    const QSignalBlocker blocker(m_sliceSlider);
    m_sliceSlider->setEnabled(true);
    m_sliceSlider->setRange(0, maxSliderValue);
    newValue = std::clamp(m_sliceSlider->value(), 0, maxSliderValue);
    m_sliceSlider->setValue(newValue);
  }

  m_renderWidget->setSliceT(static_cast<double>(newValue) / 100.0);
}

void MainWindow::setStatus(const QString& message, const StatusKind kind) {
  m_statusLabel->setText(message);

  QString style = QStringLiteral(
      "padding: 8px;"
      "border-radius: 8px;"
      "border: 1px solid #c3d3e0;");

  switch (kind) {
    case StatusKind::Idle:
      style += QStringLiteral("background: #edf3f8; color: #27465e;");
      break;
    case StatusKind::Running:
      style += QStringLiteral("background: #deedf7; color: #214e6d;");
      break;
    case StatusKind::Ok:
      style += QStringLiteral("background: #deefe5; color: #144a34;");
      break;
    case StatusKind::Warn:
      style += QStringLiteral("background: #f4ead7; color: #5f4619;");
      break;
    case StatusKind::Error:
      style += QStringLiteral("background: #f7dde0; color: #6f1820;");
      break;
  }

  m_statusLabel->setStyleSheet(style);
}

QString MainWindow::formatCounts(const std::vector<PartitionCount>& counts) {
  QStringList pieces;
  pieces.reserve(static_cast<int>(counts.size()));

  for (const PartitionCount& entry : counts) {
    const QString dimsText = QStringLiteral("(%1,%2,%3)")
                                 .arg(entry.dims[0])
                                 .arg(entry.dims[1])
                                 .arg(entry.dims[2]);
    if (entry.count == 1) {
      pieces << dimsText;
    } else {
      pieces << QStringLiteral("%1x%2").arg(dimsText).arg(entry.count);
    }
  }

  return QStringLiteral("{%1}").arg(pieces.join(QStringLiteral(", ")));
}

QString MainWindow::formatMs(const std::int64_t ms) {
  if (ms < 1000) {
    return QStringLiteral("%1 ms").arg(ms);
  }

  const double seconds = static_cast<double>(ms) / 1000.0;
  return QStringLiteral("%1 s").arg(seconds, 0, 'f', 2);
}
