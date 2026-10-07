#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "SmartRestaurant_Template_v2.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    try {
        OrderManager::instance().connectToDatabase();
    } catch (const exception &e) {
        QMessageBox::critical(nullptr, "Database Connection Error", QString::fromStdString(e.what()));
        return 1;
    }

    MainWindow window;
    window.show();
    return app.exec();
}
