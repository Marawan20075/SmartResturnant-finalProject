// Integration tests for Database and OrderManager against a real PostgreSQL.
// Needs a database with schema.sql applied (seed data is not required).
// Uses the same connection settings as the app (SMART_RESTAURANT_DB or PG*).
// Everything the tests create is deleted at the end.

#include "SmartRestaurant_Template_v2.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) {                                                          \
            cout << "  ok    " << #cond << "\n";                             \
        } else {                                                             \
            cout << "  FAIL  " << #cond << "  (line " << __LINE__ << ")\n";  \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

#define CHECK_THROWS(expr)                                                   \
    do {                                                                     \
        bool threw = false;                                                  \
        try { expr; } catch (const exception&) { threw = true; }             \
        if (threw) {                                                         \
            cout << "  ok    throws: " << #expr << "\n";                     \
        } else {                                                             \
            cout << "  FAIL  did not throw: " << #expr                       \
                 << "  (line " << __LINE__ << ")\n";                         \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static bool sameAmount(double a, double b) { return fabs(a - b) < 0.001; }

static bool containsMenuItem(const vector<MenuItem>& items, int id) {
    return any_of(items.begin(), items.end(), [id](const MenuItem& m) { return m.getId() == id; });
}

static bool containsOrder(const vector<Order>& orders, int id) {
    return any_of(orders.begin(), orders.end(), [id](const Order& o) { return o.getId() == id; });
}

int main() {
    Database* dbPtr = nullptr;
    try {
        dbPtr = &Database::instance();
    } catch (const exception& e) {
        cerr << e.what() << "\n";
        return 2;
    }
    Database& db = *dbPtr;
    OrderManager& mgr = OrderManager::instance();

    string suffix = to_string(chrono::system_clock::now().time_since_epoch().count());
    StaffMember manager(0, "Test Manager", "m@test", StaffMember::Position::Manager);
    StaffMember chef(0, "Test Chef", "c@test", StaffMember::Position::Chef);

    int customerId = db.query(
        "INSERT INTO users (name, email, role, address) VALUES "
        "('Test Customer', 'test-" + suffix + "@smartrestaurant.test', 'customer', 'Test St') "
        "RETURNING id")[0][0].as<int>();

    int burgerId = 0, juiceId = 0, hiddenId = 0;

    try {
        cout << "Menu items\n";
        burgerId = mgr.addMenuItem(manager, "Test Burger " + suffix, 10.00, true);
        juiceId = mgr.addMenuItem(manager, "Test Juice " + suffix, 2.50, true);
        hiddenId = mgr.addMenuItem(manager, "Test Hidden " + suffix, 5.00, false);
        CHECK(containsMenuItem(mgr.getAvailableMenu(), burgerId));
        CHECK(!containsMenuItem(mgr.getAvailableMenu(), hiddenId));
        CHECK(containsMenuItem(mgr.getFullMenu(), hiddenId));
        CHECK_THROWS(mgr.addMenuItem(chef, "Not allowed", 1.0, true));
        CHECK_THROWS(mgr.updateMenuItem(chef, burgerId, "Not allowed", 1.0, true));
        CHECK_THROWS(mgr.addMenuItem(manager, "Negative", -1.0, true));
        CHECK_THROWS(mgr.updateMenuItem(manager, -1, "Missing", 1.0, true));

        MenuItem burger = *db.findMenuItemById(burgerId);
        MenuItem juice = *db.findMenuItemById(juiceId);
        MenuItem hidden = *db.findMenuItemById(hiddenId);

        cout << "Create order (transactional)\n";
        Order order = mgr.createOrder(customerId, {
            OrderItem(burger, 2), OrderItem(juice, 1), OrderItem(burger, 1)
        });
        CHECK(order.getStatus() == OrderStatus::Pending);
        CHECK(order.getCustomerId() == customerId);
        CHECK(order.getItems().size() == 2);           // burger lines merged
        CHECK(sameAmount(order.total(), 3 * 10.00 + 2.50));
        CHECK(!order.getCreatedAt().empty());
        CHECK(db.loadOrderItems(order.getId()).size() == 2);

        size_t ordersBefore = mgr.getCustomerOrders(customerId).size();
        CHECK_THROWS(mgr.createOrder(customerId, {OrderItem(juice, 1), OrderItem(hidden, 1)}));
        CHECK(mgr.getCustomerOrders(customerId).size() == ordersBefore);   // rolled back
        CHECK_THROWS(mgr.createOrder(customerId, {OrderItem(burger, 0)}));
        CHECK_THROWS(mgr.createOrder(customerId, {}));

        cout << "Status transitions\n";
        int id = order.getId();
        CHECK(containsOrder(mgr.getActiveOrders(), id));
        CHECK_THROWS(db.updateOrderStatus(id, OrderStatus::Pending, OrderStatus::Delivered));
        CHECK_THROWS(db.execute("UPDATE orders SET status = 'delivered' WHERE id = " + to_string(id)));
        CHECK(!db.updateOrderStatus(id, OrderStatus::Preparing, OrderStatus::OutForDelivery));  // stale

        mgr.advanceOrder(id);
        CHECK(mgr.findOrder(id)->getStatus() == OrderStatus::Preparing);
        mgr.advanceOrder(id);
        CHECK(mgr.findOrder(id)->getStatus() == OrderStatus::OutForDelivery);
        mgr.advanceOrder(id);
        CHECK(mgr.findOrder(id)->getStatus() == OrderStatus::Delivered);
        CHECK(!containsOrder(mgr.getActiveOrders(), id));
        CHECK_THROWS(mgr.advanceOrder(id));
        CHECK_THROWS(mgr.cancelOrder(id));
        CHECK_THROWS(db.execute("UPDATE orders SET status = 'pending' WHERE id = " + to_string(id)));

        cout << "Cancel\n";
        Order second = mgr.createOrder(customerId, {OrderItem(juice, 3)});
        mgr.cancelOrder(second.getId());
        CHECK(mgr.findOrder(second.getId())->getStatus() == OrderStatus::Cancelled);
        CHECK(!containsOrder(mgr.getActiveOrders(), second.getId()));
        CHECK_THROWS(mgr.cancelOrder(second.getId()));
        CHECK_THROWS(mgr.advanceOrder(second.getId()));

        cout << "Menu update keeps historical prices\n";
        mgr.updateMenuItem(manager, burgerId, "Test Burger Deluxe " + suffix, 99.00, true);
        CHECK(sameAmount(db.findMenuItemById(burgerId)->getPrice(), 99.00));
        CHECK(sameAmount(mgr.findOrder(id)->total(), 3 * 10.00 + 2.50));
        mgr.setMenuItemAvailability(manager, burgerId, false);
        CHECK(!containsMenuItem(mgr.getAvailableMenu(), burgerId));

        cout << "Users\n";
        auto user = mgr.findUserByEmail("test-" + suffix + "@smartrestaurant.test");
        CHECK(user && user->getId() == customerId && user->role() == "Customer");
        CHECK(mgr.findUserByEmail("nobody-" + suffix + "@smartrestaurant.test") == nullptr);
    } catch (const exception& e) {
        cout << "  FAIL  unexpected exception: " << e.what() << "\n";
        ++failures;
    }

    // Clean up (order_items are removed by ON DELETE CASCADE).
    db.execute("DELETE FROM orders WHERE customer_id = " + to_string(customerId));
    db.execute("DELETE FROM menu_items WHERE id IN (" +
               to_string(burgerId) + "," + to_string(juiceId) + "," + to_string(hiddenId) + ")");
    db.execute("DELETE FROM users WHERE id = " + to_string(customerId));

    cout << (failures == 0 ? "\nAll tests passed\n" : "\n" + to_string(failures) + " test(s) failed\n");
    return failures == 0 ? 0 : 1;
}
