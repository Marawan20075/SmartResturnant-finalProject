#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QTabWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshOrders();
    void createTestOrder();


private:
    QLabel *statusLabel;


    QTabWidget *tabWidget;
    QTableWidget *customerTable;
    QTableWidget *staffTable;

    QPushButton *refreshButton;
    QPushButton *testOrderButton;
};
