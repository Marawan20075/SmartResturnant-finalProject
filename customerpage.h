#pragma once

#include <QWidget>
#include <optional>
#include "SmartRestaurant_Template_v2.hpp"

class QLabel;
class QPushButton;
class QTableWidget;
class QTabWidget;

class CustomerPage : public QWidget {
    Q_OBJECT

public:
    explicit CustomerPage(QWidget *parent = nullptr);
    void startCustomerSession(int customerId);

private slots:
    void refreshOrders();
    void refreshMenuItems();
    void addToCart();
    void increaseQty();
    void decreaseQty();
    void deleteCartItem();
    void checkout();
    
    
private:
    void refreshCart();
    optional<int> selectedItemId(QTableWidget *table) const;
    const OrderItem *selectedCartItem() const;

    optional<Order> cartOrder;
    vector<MenuItem> menuItems;
    QTabWidget *tabWidget = nullptr;
    QTableWidget *menuTable = nullptr;
    QTableWidget *cartTable = nullptr;
    QTableWidget *customerTable = nullptr;
    QLabel *emptyLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *cartTotalLabel = nullptr;
    QPushButton *checkoutButton = nullptr;
};
