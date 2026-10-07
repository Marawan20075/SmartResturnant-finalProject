#include "staffpage.h"
#include "SmartRestaurant_Template_v2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QTabWidget>
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

StaffPage::StaffPage(QWidget *parent) : QWidget(parent) {
    tabWidget = new QTabWidget(this);

    staffTable = new QTableWidget(0, 3);
    staffTable->setHorizontalHeaderLabels({"Order ID", "Status", "Action"});
    staffTable->horizontalHeader()->setStretchLastSection(true);

    tabWidget->addTab(staffTable, "Staff Dashboard");

    auto *refreshButton = new QPushButton("Refresh");
    connect(refreshButton, &QPushButton::clicked, this, &StaffPage::refreshOrders);

    auto *testOrderButton = new QPushButton("Create Test Order");
    connect(testOrderButton, &QPushButton::clicked, this, &StaffPage::createTestOrder);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(refreshButton);
    buttonRow->addWidget(testOrderButton);

    auto *layout = new QVBoxLayout();
    layout->addWidget(tabWidget);
    layout->addLayout(buttonRow);

    setLayout(layout);

    refreshOrders();
}

void StaffPage::refreshOrders() {
    auto &orders = OrderManager::instance().getAllOrders();

    staffTable->setRowCount(0);

    for (auto &order : orders) {
        int orderId = order.getId();
        OrderStatus currentStatus = order.getStatus();
        QString statusText = statusToText(currentStatus);

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

void StaffPage::createTestOrder() {
    auto &manager = OrderManager::instance();
    Order order = manager.createOrder(1);
    MenuItem item(1, "Test Burger", 9.99, true);
    order.addItem(OrderItem(item, 2));
    manager.saveOrder(order);
    refreshOrders();
}
