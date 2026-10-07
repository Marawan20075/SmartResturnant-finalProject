#include "customerpage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <algorithm>
#include <limits>

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

CustomerPage::CustomerPage(QWidget *parent) : QWidget(parent) {
    tabWidget = new QTabWidget(this);
    auto *menuPage = new QWidget(tabWidget);
    auto *menuTab = new QVBoxLayout(menuPage);

    menuTable = new QTableWidget(0, 3, menuPage);
    menuTable->setHorizontalHeaderLabels({"Item ID", "Name", "Price"});
    menuTable->horizontalHeader()->setStretchLastSection(true);
    menuTable->setObjectName("menuTable");

    emptyLabel = new QLabel("Log in to load the menu.", menuPage);
    auto *addToCartButton = new QPushButton("Add To Cart", menuPage);
    connect(addToCartButton, &QPushButton::clicked, this, &CustomerPage::addToCart);
    auto *refreshMenuButton = new QPushButton("Refresh Menu", menuPage);
    connect(refreshMenuButton, &QPushButton::clicked, this, &CustomerPage::refreshMenuItems);
    auto *menuButtonRow = new QHBoxLayout();
    menuButtonRow->addWidget(addToCartButton);
    menuButtonRow->addWidget(refreshMenuButton);

    menuTab->addWidget(menuTable);
    menuTab->addWidget(emptyLabel);
    menuTab->addLayout(menuButtonRow);

    auto *cartPage = new QWidget(tabWidget);
    auto *cartTab = new QVBoxLayout(cartPage);

    cartTable = new QTableWidget(0, 5, cartPage);
    cartTable->setHorizontalHeaderLabels({"Item ID", "Name", "Qty", "Unit Price", "Subtotal"});
    cartTable->horizontalHeader()->setStretchLastSection(true);
    cartTable->setObjectName("cartTable");

    auto *decreaseQtyButton = new QPushButton("Decrease Qty", cartPage);
    connect(decreaseQtyButton, &QPushButton::clicked, this, &CustomerPage::decreaseQty);
    auto *increaseQtyButton = new QPushButton("Increase Qty", cartPage);
    connect(increaseQtyButton, &QPushButton::clicked, this, &CustomerPage::increaseQty);
    auto *deleteCartItemButton = new QPushButton("Remove Item", cartPage);
    connect(deleteCartItemButton, &QPushButton::clicked, this, &CustomerPage::deleteCartItem);
    auto *cartButtonRow = new QHBoxLayout();
    cartButtonRow->addWidget(decreaseQtyButton);
    cartButtonRow->addWidget(increaseQtyButton);
    cartButtonRow->addWidget(deleteCartItemButton);

    cartTotalLabel = new QLabel(cartPage);
    cartTotalLabel->setObjectName("cartTotalLabel");

    checkoutButton = new QPushButton("Place Order", cartPage);
    connect(checkoutButton, &QPushButton::clicked, this, &CustomerPage::checkout);

    cartTab->addWidget(cartTable);
    cartTab->addLayout(cartButtonRow);
    cartTab->addWidget(cartTotalLabel);
    cartTab->addWidget(checkoutButton);

    auto *customerPage = new QWidget(tabWidget);
    auto *customerTab = new QVBoxLayout(customerPage);

    customerTable = new QTableWidget(0, 2, customerPage);
    customerTable->setHorizontalHeaderLabels({"Order ID", "Status"});
    customerTable->horizontalHeader()->setStretchLastSection(true);
    customerTable->setObjectName("customerTable");

    auto *refreshButton = new QPushButton("Refresh", customerPage);
    connect(refreshButton, &QPushButton::clicked, this, &CustomerPage::refreshOrders);
    customerTab->addWidget(customerTable);
    customerTab->addWidget(refreshButton);

    for (auto *table : {menuTable, cartTable, customerTable}) {
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    tabWidget->addTab(menuPage, "Menu");
    tabWidget->addTab(cartPage, "Cart");
    tabWidget->addTab(customerPage, "Customer Tracking");

    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabWidget);
    layout->addWidget(statusLabel);

    refreshCart();
}

void CustomerPage::startCustomerSession(int customerId) {
    if (customerId <= 0) throw invalid_argument("A valid customer ID is required");
    cartOrder = OrderManager::instance().createOrder(customerId);
    statusLabel->clear();
    refreshCart();
    refreshOrders();
    refreshMenuItems();
    tabWidget->setCurrentIndex(0);
}

void CustomerPage::refreshOrders() {
    customerTable->setRowCount(0);
    try {
        for (const auto &order : OrderManager::instance().getCustomerOrders(cartOrder->getCustomerId())) {
            const int row = customerTable->rowCount();
            customerTable->insertRow(row);
            customerTable->setItem(row, 0, new QTableWidgetItem(QString::number(order.getId())));
            customerTable->setItem(row, 1, new QTableWidgetItem(statusToText(order.getStatus())));
        }
    } catch (const exception &error) {
        statusLabel->setText("Could not load orders: " + QString::fromUtf8(error.what()));
    }
}

void CustomerPage::refreshMenuItems() {
    menuItems.clear();
    menuTable->setRowCount(0);
    try {
        menuItems = OrderManager::instance().getAvailableMenuItems();
        for (const auto &item : menuItems) {
            const int row = menuTable->rowCount();
            menuTable->insertRow(row);
            menuTable->setItem(row, 0, new QTableWidgetItem(QString::number(item.getId())));
            menuTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.getName())));
            menuTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.getPrice(), 'f', 2)));
        }
        emptyLabel->setText(menuItems.empty() ? "No menu items are currently available." : "");
    } catch (const exception &error) {
        emptyLabel->setText("Could not load the menu. Use Refresh Menu to try again.");
        statusLabel->setText(QString::fromUtf8(error.what()));
    }
}

optional<int> CustomerPage::selectedItemId(QTableWidget *table) const {
    const int row = table->currentRow();
    if (row < 0 || !table->item(row, 0))
        return nullopt;
    bool valid = false;
    const int id = table->item(row, 0)->text().toInt(&valid);
    return valid ? optional<int>(id) : nullopt;
}

const OrderItem *CustomerPage::selectedCartItem() const {
    const auto id = selectedItemId(cartTable);
    if (!id)
        return nullptr;
    const auto &items = cartOrder->getItems();
    const auto item = find_if(items.begin(), items.end(), [id](const OrderItem &entry) {
        return entry.item.getId() == *id;
    });
    return item == items.end() ? nullptr : &*item;
}

void CustomerPage::refreshCart() {
    const auto selectedId = selectedItemId(cartTable);
    cartTable->setRowCount(0);

    if (cartOrder) {
        for (const auto &entry : cartOrder->getItems()) {
            const int row = cartTable->rowCount();
            cartTable->insertRow(row);
            cartTable->setItem(row, 0, new QTableWidgetItem(QString::number(entry.item.getId())));
            cartTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(entry.item.getName())));
            cartTable->setItem(row, 2, new QTableWidgetItem(QString::number(entry.quantity)));
            cartTable->setItem(row, 3, new QTableWidgetItem(QString::number(entry.item.getPrice(), 'f', 2)));
            cartTable->setItem(row, 4, new QTableWidgetItem(QString::number(entry.subtotal(), 'f', 2)));

            if (selectedId && *selectedId == entry.item.getId()) cartTable->selectRow(row);
        }
    }

    const double total = cartOrder ? cartOrder->total() : 0.0;
    cartTotalLabel->setText("Total: " + QString::number(total, 'f', 2));
    checkoutButton->setEnabled(cartOrder && !cartOrder->getItems().empty());
}

void CustomerPage::addToCart() {
    const auto id = selectedItemId(menuTable);
    if (!id) {
        statusLabel->setText("Select a menu item first.");
        return;
    }
    const auto item = find_if(menuItems.begin(), menuItems.end(), [id](const MenuItem &entry) {
        return entry.getId() == *id;
    });
    if (item == menuItems.end())
        return;
    try {
        cartOrder->addItem(OrderItem(*item, 1));
        refreshCart();
        statusLabel->setText(QString::fromStdString(item->getName()) + " added to cart.");
    } catch (const exception &error) {
        statusLabel->setText(QString::fromUtf8(error.what()));
    }
}

void CustomerPage::increaseQty() {
    const auto *item = selectedCartItem();
    if (!item) {
        statusLabel->setText("Select a cart item first.");
        return;
    }
    if (item->quantity == numeric_limits<int>::max()) {
        statusLabel->setText("Quantity is too large.");
        return;
    }
    cartOrder->setItemQuantity(item->item.getId(), item->quantity + 1);
    refreshCart();
    statusLabel->clear();
}

void CustomerPage::decreaseQty() {
    const auto *item = selectedCartItem();
    if (!item) {
        statusLabel->setText("Select a cart item first.");
        return;
    }
    if (item->quantity == 1) {
        statusLabel->setText("Quantity must stay at least 1. Use Remove Item to delete it.");
        return;
    }
    cartOrder->setItemQuantity(item->item.getId(), item->quantity - 1);
    refreshCart();
    statusLabel->clear();
}

void CustomerPage::deleteCartItem() {
    const auto *item = selectedCartItem();
    if (!item) {
        statusLabel->setText("Select a cart item first.");
        return;
    }
    cartOrder->removeItem(item->item.getId());
    refreshCart();
    statusLabel->setText("Item removed from cart.");
}

void CustomerPage::checkout() {
    if (cartOrder->getItems().empty()) {
        statusLabel->setText("Add an item to your cart before placing an order.");
        return;
    }
    try {
        auto &manager = OrderManager::instance();
        const int customerId = cartOrder->getCustomerId();
        const Order savedOrder = manager.saveOrder(*cartOrder);
        cartOrder = manager.createOrder(customerId);
        refreshCart();
        refreshOrders();
        tabWidget->setCurrentIndex(2);
        statusLabel->setText("Order #" + QString::number(savedOrder.getId()) + " placed.");
    } catch (const exception &error) {
        statusLabel->setText("Could not place the order: " + QString::fromUtf8(error.what()));
    }
}
