#include "staffpage.h"
#include "SmartRestaurant_Template_v2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QHeaderView>
#include <QMessageBox>

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
    staffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

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
    vector<Order> orders;
    try {
        orders = OrderManager::instance().getActiveOrders();
    } catch (const exception &error) {
        QMessageBox::critical(this, "Database Error", QString::fromUtf8(error.what()));
        return;
    }

    staffTable->setRowCount(0);

    for (const auto &order : orders) {
        int orderId = order.getId();
        OrderStatus currentStatus = order.getStatus();
        QString statusText = statusToText(currentStatus);

        int staffRow = staffTable->rowCount();
        staffTable->insertRow(staffRow);
        staffTable->setItem(staffRow, 0, new QTableWidgetItem(QString::number(orderId)));
        staffTable->setItem(staffRow, 1, new QTableWidgetItem(statusText));

        QPushButton *advanceBtn = new QPushButton("Advance Status");

        connect(advanceBtn, &QPushButton::clicked, [this, orderId]() {
            try {
                OrderManager::instance().advanceOrder(orderId);
            } catch (const exception &error) {
                QMessageBox::warning(this, "Cannot Advance Order", QString::fromUtf8(error.what()));
            }
            refreshOrders();
        });
        staffTable->setCellWidget(staffRow, 2, advanceBtn);
    }
}

void StaffPage::createTestOrder() {
    try {
        auto &manager = OrderManager::instance();
        const auto menu = manager.getAvailableMenu();
        if (menu.empty()) {
            QMessageBox::warning(this, "No Menu Items",
                                 "There are no available menu items. Run seed.sql first.");
            return;
        }

        Order order = manager.createOrder(1);
        order.addItem(OrderItem(menu.front(), 2));
        manager.saveOrder(order);
    } catch (const exception &error) {
        QMessageBox::critical(this, "Could Not Create Order", QString::fromUtf8(error.what()));
    }
    refreshOrders();
}
