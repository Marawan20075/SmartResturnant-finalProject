#pragma once
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <cstdlib>

using namespace std;

class User {
protected:
    int id;
    string name;
    string email;


public:
    User(int id, string name, string email)
        : id(id), name(move(name)), email(move(email)) {}
    virtual ~User() = default;

    int getId() const { return id; }
    string getName() const { return name; }
    string getEmail() const { return email; }
    virtual string role() const = 0;
};

class Customer : public User {
private:
    string address;

public:
    Customer(int id, string name, string email, string address)
        : User(id, move(name), move(email)), address(move(address)) {}

    string role() const override { return "Customer"; }
    string getAddress() const { return address; }
};

class StaffMember : public User {
public:
    enum class Position { Chef, DeliveryDriver, Manager };

private:
    Position position;

public:
    StaffMember(int id, string name, string email, Position position)
        : User(id, move(name), move(email)), position(position) {}

    string role() const override { return "Staff"; }
    Position getPosition() const { return position; }
    string getPositionString() const {
        switch (position){
            case Position :: Chef: return "Chef";
            case Position :: DeliveryDriver: return "Delivery Driver";
            case Position :: Manager: return "Manager";
        }
        return "Unknown";
    }
};

class MenuItem {
private:
    int id;
    string name;
    double price;
    bool available;

public:
    MenuItem(int id, string name, double price, bool available)
        : id(id), name(move(name)), price(price), available(available) {}

    int getId() const { return id; }
    string getName() const { return name; }
    double getPrice() const { return price; }
    bool isAvailable() const { return available; }
    void setAvailable(bool value) { available = value; }
};

enum class OrderStatus { Pending, Preparing, OutForDelivery, Delivered, Cancelled };

inline string orderStatusToDb(OrderStatus status) {
    switch (status) {
        case OrderStatus::Pending: return "pending";
        case OrderStatus::Preparing: return "preparing";
        case OrderStatus::OutForDelivery: return "out_for_delivery";
        case OrderStatus::Delivered: return "delivered";
        case OrderStatus::Cancelled: return "cancelled";
    }
    throw invalid_argument("Unknown order status");
}

inline OrderStatus orderStatusFromDb(const string& status) {
    if (status == "pending") return OrderStatus::Pending;
    if (status == "preparing") return OrderStatus::Preparing;
    if (status == "out_for_delivery") return OrderStatus::OutForDelivery;
    if (status == "delivered") return OrderStatus::Delivered;
    if (status == "cancelled") return OrderStatus::Cancelled;
    throw invalid_argument("Unknown order status in database: " + status);
}

inline bool isValidStatusTransition(OrderStatus from, OrderStatus to) {
    switch (from) {
        case OrderStatus::Pending:
            return to == OrderStatus::Preparing || to == OrderStatus::Cancelled;
        case OrderStatus::Preparing:
            return to == OrderStatus::OutForDelivery || to == OrderStatus::Cancelled;
        case OrderStatus::OutForDelivery:
            return to == OrderStatus::Delivered || to == OrderStatus::Cancelled;
        default:
            return false;
    }
}

class OrderItem {
public:
    MenuItem item;
    int quantity;

    OrderItem(MenuItem item, int quantity) : item(move(item)), quantity(quantity) {}
    double subtotal() const { return item.getPrice() * quantity; }
};

class Order {
private:
    int id;
    int customerId;
    vector<OrderItem> items;
    OrderStatus status;
    string createdAt;

public:
    Order(int id, int customerId) : id(id), customerId(customerId), status(OrderStatus::Pending) {}

    Order(int id, int customerId, OrderStatus status, string createdAt)
        : id(id), customerId(customerId), status(status), createdAt(move(createdAt)) {}

    void addItem(const OrderItem& orderItem) 
    { 
        if (orderItem.quantity <= 0)
            throw invalid_argument("Quantity must be positive ");
        items.push_back(orderItem);
    }


    double total() const {
        double sum = 0;
        for (const auto& i : items) sum += i.subtotal();
        return sum;
    }

    OrderStatus getStatus() const { return status; }

    void advanceStatus() {
        switch (status) {
            case OrderStatus::Pending: status = OrderStatus::Preparing; break;
            case OrderStatus::Preparing: status = OrderStatus::OutForDelivery; break;
            case OrderStatus::OutForDelivery: status = OrderStatus::Delivered; break;
            default: break;
        }
    }

    void cancel() {
        if (status == OrderStatus::Delivered)
            throw logic_error("Cannot cancel a delivered order");
        status = OrderStatus::Cancelled;
    }

    int getId() const { return id; }
    int getCustomerId() const { return customerId; }
    string getCreatedAt() const { return createdAt; }
    const vector<OrderItem>& getItems() const { return items; }
};

class Database {
private:
    unique_ptr<pqxx::connection> conn;

    static constexpr const char* orderSelectSql =
        "SELECT o.id AS order_id, o.customer_id, o.status, "
        "       to_char(o.created_at, 'YYYY-MM-DD HH24:MI') AS created_at, "
        "       oi.quantity, oi.unit_price, "
        "       m.id AS menu_item_id, m.name AS menu_item_name, m.available AS menu_item_available "
        "FROM orders o "
        "LEFT JOIN order_items oi ON oi.order_id = o.id "
        "LEFT JOIN menu_items m ON m.id = oi.menu_item_id ";

    static string envOr(const char* name, const string& fallback) {
        const char* value = getenv(name);
        return (value && *value) ? string(value) : fallback;
    }

    static string connectionString() {
        string full = envOr("SMART_RESTAURANT_DB", "");
        if (!full.empty()) return full;

        return "host=" + envOr("PGHOST", "127.0.0.1") +
               " port=" + envOr("PGPORT", "5432") +
               " dbname=" + envOr("PGDATABASE", "smart_restaurant") +
               " user=" + envOr("PGUSER", "postgres");
    }

    Database() {
        try {
            conn = make_unique<pqxx::connection>(connectionString());
        } catch (const exception& e) {
            throw runtime_error(
                string("Could not connect to the PostgreSQL database.\n"
                       "Check that the server is running and that SMART_RESTAURANT_DB "
                       "or the PGHOST/PGPORT/PGDATABASE/PGUSER/PGPASSWORD environment "
                       "variables are set correctly.\n\nDetails: ") + e.what());
        }
    }

    template <typename Row>
    static MenuItem menuItemFromRow(const Row& row) {
        return MenuItem(
            row["id"].as<int>(),
            row["name"].as<string>(),
            row["price"].as<double>(),
            row["available"].as<bool>()
        );
    }

    static vector<Order> ordersFromRows(const pqxx::result& result) {
        vector<Order> orders;

        for (const auto& row : result)
        {
            int orderId = row["order_id"].as<int>();

            if (orders.empty() || orders.back().getId() != orderId)
            {
                orders.emplace_back(
                    orderId,
                    row["customer_id"].as<int>(),
                    orderStatusFromDb(row["status"].as<string>()),
                    row["created_at"].as<string>()
                );
            }

            if (!row["menu_item_id"].is_null())
            {
                MenuItem item(
                    row["menu_item_id"].as<int>(),
                    row["menu_item_name"].as<string>(),
                    row["unit_price"].as<double>(),
                    row["menu_item_available"].as<bool>()
                );
                orders.back().addItem(OrderItem(item, row["quantity"].as<int>()));
            }
        }

        return orders;
    }

public:
    static Database& instance() {
        static Database db;
        return db;
    }

    pqxx::result query(const string& sql) {
        pqxx::work txn(*conn);
        pqxx::result r = txn.exec(sql);
        txn.commit();
        return r;
    }

    void execute(const string& sql) {
        pqxx::work txn(*conn);
        txn.exec(sql);
        txn.commit();
    }

    // ---------- Menu items ----------

    vector<MenuItem> loadAllMenuItems()
    {
        vector<MenuItem> items;

        pqxx::work txn(*conn);

        pqxx::result result = txn.exec(
            "SELECT id, name, price, available FROM menu_items ORDER BY id"
        );

        for (const auto& row : result)
        {
            items.push_back(menuItemFromRow(row));
        }

        txn.commit();

        return items;
    }

    vector<MenuItem> loadAvailableMenuItems()
    {
        vector<MenuItem> items;

        pqxx::work txn(*conn);

        pqxx::result result = txn.exec(
            "SELECT id, name, price, available FROM menu_items "
            "WHERE available = true ORDER BY id"
        );

        for (const auto& row : result)
        {
            items.push_back(menuItemFromRow(row));
        }

        txn.commit();

        return items;
    }

    vector<MenuItem> loadMenuItems()
    {
        return loadAllMenuItems();
    }

    optional<MenuItem> findMenuItemById(int id)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "SELECT id, name, price, available FROM menu_items WHERE id = $1",
            id
        );

        txn.commit();

        if (result.empty())
        {
            return nullopt;
        }

        return menuItemFromRow(result[0]);
    }

    int addMenuItem(const string& name, double price, bool available)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "INSERT INTO menu_items (name, price, available) VALUES ($1, $2, $3) RETURNING id",
            name,
            price,
            available
        );

        txn.commit();

        return result[0][0].as<int>();
    }

    bool updateMenuItem(int id, const string& name, double price, bool available)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "UPDATE menu_items SET name = $1, price = $2, available = $3 WHERE id = $4",
            name,
            price,
            available,
            id
        );

        txn.commit();

        return result.affected_rows() == 1;
    }

    bool setMenuItemAvailability(int id, bool available)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "UPDATE menu_items SET available = $1 WHERE id = $2",
            available,
            id
        );

        txn.commit();

        return result.affected_rows() == 1;
    }

    // ---------- Orders ----------

    int insertOrder(int customerId, const vector<OrderItem>& items)
    {
        if (items.empty())
        {
            throw invalid_argument("An order must contain at least one item");
        }

        pqxx::work txn(*conn);

        int orderId = txn.exec_params(
            "INSERT INTO orders (customer_id) VALUES ($1) RETURNING id",
            customerId
        )[0][0].as<int>();

        for (const auto& orderItem : items)
        {
            pqxx::result inserted = txn.exec_params(
                "INSERT INTO order_items (order_id, menu_item_id, quantity, unit_price) "
                "SELECT $1, id, $3, price FROM menu_items WHERE id = $2 AND available = true",
                orderId,
                orderItem.item.getId(),
                orderItem.quantity
            );

            if (inserted.affected_rows() != 1)
            {
                throw runtime_error(
                    "Menu item \"" + orderItem.item.getName() + "\" is not available");
            }
        }

        txn.commit();

        return orderId;
    }

    optional<Order> loadOrderById(int orderId)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            string(orderSelectSql) + "WHERE o.id = $1 ORDER BY o.id, oi.id",
            orderId
        );

        txn.commit();

        vector<Order> orders = ordersFromRows(result);
        if (orders.empty())
        {
            return nullopt;
        }

        return orders.front();
    }

    vector<Order> loadCustomerOrders(int customerId)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            string(orderSelectSql) +
            "WHERE o.customer_id = $1 ORDER BY o.created_at DESC, o.id DESC, oi.id",
            customerId
        );

        txn.commit();

        return ordersFromRows(result);
    }

    vector<Order> loadActiveOrders()
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec(
            string(orderSelectSql) +
            "WHERE o.status IN ('pending', 'preparing', 'out_for_delivery') "
            "ORDER BY o.created_at, o.id, oi.id"
        );

        txn.commit();

        return ordersFromRows(result);
    }

    vector<Order> loadAllOrders()
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec(
            string(orderSelectSql) + "ORDER BY o.created_at DESC, o.id DESC, oi.id"
        );

        txn.commit();

        return ordersFromRows(result);
    }

    vector<OrderItem> loadOrderItems(int orderId)
    {
        vector<OrderItem> items;

        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "SELECT m.id, m.name, oi.unit_price, m.available, oi.quantity "
            "FROM order_items oi "
            "JOIN menu_items m ON m.id = oi.menu_item_id "
            "WHERE oi.order_id = $1 ORDER BY oi.id",
            orderId
        );

        for (const auto& row : result)
        {
            MenuItem item(
                row["id"].as<int>(),
                row["name"].as<string>(),
                row["unit_price"].as<double>(),
                row["available"].as<bool>()
            );
            items.emplace_back(item, row["quantity"].as<int>());
        }

        txn.commit();

        return items;
    }

    bool updateOrderStatus(int orderId, OrderStatus from, OrderStatus to)
    {
        if (!isValidStatusTransition(from, to))
        {
            throw logic_error(
                "Invalid order status transition: " +
                orderStatusToDb(from) + " -> " + orderStatusToDb(to));
        }

        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "UPDATE orders SET status = $1 WHERE id = $2 AND status = $3",
            orderStatusToDb(to),
            orderId,
            orderStatusToDb(from)
        );

        txn.commit();

        return result.affected_rows() == 1;
    }

    // ---------- Users ----------

    unique_ptr<User> findUserByEmail(const string& email)
    {
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "SELECT id, name, email, role, address, position "
            "FROM users WHERE email = $1",
            email
        );

        if (result.empty())
        {
            return nullptr;
        }

        auto row = result[0];

        string role = row["role"].as<string>();

        if (role == "customer")
        {
            return make_unique<Customer>(
                row["id"].as<int>(),
                row["name"].as<string>(),
                row["email"].as<string>(),
                row["address"].is_null() ? "" : row["address"].as<string>()
            );
        }
        else if (role == "staff")
        {
            string pos = row["position"].is_null()
                ? ""
                : row["position"].as<string>();

            StaffMember::Position position;

            if (pos == "Chef")
                position = StaffMember::Position::Chef;
            else if (pos == "Delivery Driver")
                position = StaffMember::Position::DeliveryDriver;
            else
                position = StaffMember::Position::Manager;

            return make_unique<StaffMember>(
                row["id"].as<int>(),
                row["name"].as<string>(),
                row["email"].as<string>(),
                position
            );
        }

        return nullptr;
    }

    Database(const Database&) = delete;
    void operator=(const Database&) = delete;
};

class OrderManager {
private:
    OrderManager() = default;

    static Database& db() { return Database::instance(); }

    static void requireManager(const StaffMember& staff) {
        if (staff.getPosition() != StaffMember::Position::Manager)
            throw logic_error("Only a manager can add or update menu items");
    }

    static void validateMenuItem(const string& name, double price) {
        if (name.empty()) throw invalid_argument("Menu item name cannot be empty");
        if (price < 0) throw invalid_argument("Menu item price cannot be negative");
    }

    Order requireOrder(int orderId) {
        auto order = db().loadOrderById(orderId);
        if (!order) throw invalid_argument("Order #" + to_string(orderId) + " does not exist");
        return *order;
    }

    void persistStatus(int orderId, OrderStatus from, OrderStatus to) {
        if (!db().updateOrderStatus(orderId, from, to))
            throw runtime_error("Order #" + to_string(orderId) +
                                " was changed by someone else. Refresh and try again.");
    }

public:
    static OrderManager& instance() {
        static OrderManager mgr;
        return mgr;
    }

    void connectToDatabase() { db(); }

    // ---------- Orders ----------

    Order createOrder(int customerId, const vector<OrderItem>& items) {
        if (items.empty()) throw invalid_argument("An order must contain at least one item");

        vector<OrderItem> merged;
        for (const auto& orderItem : items) {
            if (orderItem.quantity <= 0)
                throw invalid_argument("Quantity must be greater than zero");

            auto it = find_if(merged.begin(), merged.end(), [&](const OrderItem& m) {
                return m.item.getId() == orderItem.item.getId();
            });
            if (it != merged.end()) it->quantity += orderItem.quantity;
            else merged.push_back(orderItem);
        }

        int orderId = db().insertOrder(customerId, merged);
        return requireOrder(orderId);
    }

    optional<Order> findOrder(int orderId) { return db().loadOrderById(orderId); }

    void advanceOrder(int orderId) {
        Order order = requireOrder(orderId);
        OrderStatus from = order.getStatus();
        order.advanceStatus();
        if (order.getStatus() == from)
            throw logic_error("Order #" + to_string(orderId) + " cannot be advanced any further");
        persistStatus(orderId, from, order.getStatus());
    }

    void cancelOrder(int orderId) {
        Order order = requireOrder(orderId);
        OrderStatus from = order.getStatus();
        if (from == OrderStatus::Cancelled)
            throw logic_error("Order #" + to_string(orderId) + " is already cancelled");
        order.cancel();
        persistStatus(orderId, from, order.getStatus());
    }

    vector<Order> getCustomerOrders(int customerId) { return db().loadCustomerOrders(customerId); }
    vector<Order> getActiveOrders() { return db().loadActiveOrders(); }
    vector<Order> getAllOrders() { return db().loadAllOrders(); }

    // ---------- Menu ----------

    vector<MenuItem> getAvailableMenu() { return db().loadAvailableMenuItems(); }
    vector<MenuItem> getFullMenu() { return db().loadAllMenuItems(); }

    int addMenuItem(const StaffMember& staff, const string& name, double price, bool available) {
        requireManager(staff);
        validateMenuItem(name, price);
        return db().addMenuItem(name, price, available);
    }

    void updateMenuItem(const StaffMember& staff, int id, const string& name, double price, bool available) {
        requireManager(staff);
        validateMenuItem(name, price);
        if (!db().updateMenuItem(id, name, price, available))
            throw invalid_argument("Menu item #" + to_string(id) + " does not exist");
    }

    void setMenuItemAvailability(const StaffMember& staff, int id, bool available) {
        requireManager(staff);
        if (!db().setMenuItemAvailability(id, available))
            throw invalid_argument("Menu item #" + to_string(id) + " does not exist");
    }

    // ---------- Users ----------

    unique_ptr<User> findUserByEmail(const string& email) { return db().findUserByEmail(email); }

    OrderManager(const OrderManager&) = delete;
    void operator=(const OrderManager&) = delete;
};
