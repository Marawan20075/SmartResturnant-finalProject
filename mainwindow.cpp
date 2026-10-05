#include "mainwindow.h"
#include "SmartRestaurant_Template_v2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QHeaderView>

static QString statusToText(OrderStatus status) {
    switch (status) {
    case OrderStatus::Pending: return "Pending";
    case OrderStatus::Preparing: return "Preparing";
    case OrderStatus::OutForDelivery: return "Out for Delivery";
    case OrderStatus::Delivered: return "Delivered";
    case OrderStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Smart Restaurant - Order Tracking");
    resize(700, 500);


    tabWidget = new QTabWidget(this);


    customerTable = new QTableWidget(0, 2);
    customerTable->setHorizontalHeaderLabels({"Order ID", "Status"});
    customerTable->horizontalHeader()->setStretchLastSection(true);


    staffTable = new QTableWidget(0, 3);
    staffTable->setHorizontalHeaderLabels({"Order ID", "Status", "Action"});
    staffTable->horizontalHeader()->setStretchLastSection(true);
    tabWidget->addTab(customerTable, "Customer Tracking");
    tabWidget->addTab(staffTable, "Staff Dashboard");

    refreshButton = new QPushButton("Refresh");
    testOrderButton = new QPushButton("Create Test Order");

    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshOrders);
    connect(testOrderButton, &QPushButton::clicked, this, &MainWindow::createTestOrder);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(refreshButton);
    buttonRow->addWidget(testOrderButton);

    auto *layout = new QVBoxLayout();
    layout->addWidget(tabWidget);
    layout->addLayout(buttonRow);

    auto *central = new QWidget();
    central->setLayout(layout);
    setCentralWidget(central);

    refreshOrders();
}

void MainWindow::refreshOrders() {
    auto &orders = OrderManager::instance().getAllOrders();


    customerTable->setRowCount(0);
    staffTable->setRowCount(0);

    for (auto &order : orders) {
        int orderId = order.getId();
        OrderStatus currentStatus = order.getStatus();
        QString statusText = statusToText(currentStatus);


        int custRow = customerTable->rowCount();
        customerTable->insertRow(custRow);
        customerTable->setItem(custRow, 0, new QTableWidgetItem(QString::number(orderId)));
        customerTable->setItem(custRow, 1, new QTableWidgetItem(statusText));


        int staffRow = staffTable->rowCount();
        staffTable->insertRow(staffRow);
        staffTable->setItem(staffRow, 0, new QTableWidgetItem(QString::number(orderId)));
        staffTable->setItem(staffRow, 1, new QTableWidgetItem(statusText));


        QPushButton *advanceBtn = new QPushButton("Advance Status");

        if (currentStatus == OrderStatus::Delivered) {
            advanceBtn->setEnabled(false);
            advanceBtn->setText("Completed");
        } else {

            connect(advanceBtn, &QPushButton::clicked, [this, orderId]() {
                OrderManager::instance().advanceOrder(orderId);
                this->refreshOrders();
            });
        }
        staffTable->setCellWidget(staffRow, 2, advanceBtn);
    }
}

void MainWindow::createTestOrder() {
    Order &order = OrderManager::instance().createOrder(1);
    MenuItem item(1, "Test Burger", 9.99, true);
    order.addItem(OrderItem(item, 2));
    refreshOrders();
}