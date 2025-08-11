#include <QApplication>
#include "MainWindow.h"
#include "Logger.h"

int main(int argc, char* argv[]) {
    slog::Logger::init(slog::Level::Info, "serpent_gui.log");
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
