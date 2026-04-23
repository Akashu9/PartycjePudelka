#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("Partycje Prostopadloscianu"));
  app.setOrganizationName(QStringLiteral("PartycjePudelka"));

  MainWindow window;
  window.show();

  return app.exec();
}
