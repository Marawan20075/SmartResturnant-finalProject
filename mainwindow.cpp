#include "mainwindow.h"
#include "SmartRestaurant_Template_v2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QHeaderView>
#include <QMessageBox>

// Customer #1 is created by seed.sql. Replace with the logged-in user once login exists.
static const int currentCustomerId = 1;

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
    vector<Order> customerOrders;
    vector<Order> activeOrders;

    try {
        customerOrders = OrderManager::instance().getCustomerOrders(currentCustomerId);
        activeOrders = OrderManager::instance().getActiveOrders();
    } catch (const exception &e) {
        QMessageBox::critical(this, "Database Error", QString::fromStdString(e.what()));
        return;
    }

    customerTable->setRowCount(0);
    staffTable->setRowCount(0);

    for (const auto &order : customerOrders) {
        int custRow = customerTable->rowCount();
        customerTable->insertRow(custRow);
        customerTable->setItem(custRow, 0, new QTableWidgetItem(QString::number(order.getId())));
        customerTable->setItem(custRow, 1, new QTableWidgetItem(statusToText(order.getStatus())));
    }

    // Staff dashboard shows active orders only, so every row can be advanced.
    for (const auto &order : activeOrders) {
        int orderId = order.getId();

        int staffRow = staffTable->rowCount();
        staffTable->insertRow(staffRow);
        staffTable->setItem(staffRow, 0, new QTableWidgetItem(QString::number(orderId)));
        staffTable->setItem(staffRow, 1, new QTableWidgetItem(statusToText(order.getStatus())));

        QPushButton *advanceBtn = new QPushButton("Advance Status");
        connect(advanceBtn, &QPushButton::clicked, [this, orderId]() {
            try {
                OrderManager::instance().advanceOrder(orderId);
            } catch (const exception &e) {
                QMessageBox::warning(this, "Cannot Advance Order", QString::fromStdString(e.what()));
            }
            this->refreshOrders();
        });
        staffTable->setCellWidget(staffRow, 2, advanceBtn);
    }
}

void MainWindow::createTestOrder() {
    try {
        vector<MenuItem> menu = OrderManager::instance().getAvailableMenu();
        if (menu.empty()) {
            QMessageBox::warning(this, "No Menu Items",
                                 "There are no available menu items. Run seed.sql first.");
            return;
        }
        OrderManager::instance().createOrder(currentCustomerId, {OrderItem(menu.front(), 2)});
    } catch (const exception &e) {
        QMessageBox::critical(this, "Could Not Create Order", QString::fromStdString(e.what()));
    }
    refreshOrders();
}