#pragma once

#include <QWidget>

class QTableWidget;
class QTabWidget;

class StaffPage : public QWidget {
    Q_OBJECT

public:
    explicit StaffPage(QWidget *parent = nullptr);

public slots:
    void refreshOrders();

private slots:
    void createTestOrder();

private:
    QTabWidget *tabWidget = nullptr;
    QTableWidget *staffTable = nullptr;
};
