#pragma once
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>

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

public:
    Order(int id, int customerId) : id(id), customerId(customerId), status(OrderStatus::Pending) {}

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
    const vector<OrderItem>& getItems() const { return items; }
};

class OrderManager {
private:
    vector<Order> orders;
    int nextId = 1;

    OrderManager() = default;

public:
    static OrderManager& instance() {
        static OrderManager mgr;
        return mgr;
    }

    Order& createOrder(int customerId) {
        orders.emplace_back(nextId++, customerId);
        return orders.back();
    }

    Order* findOrder(int orderId) {
        auto it = find_if(orders.begin(), orders.end(),
            [orderId](const Order& o) { return o.getId() == orderId; });
        return it != orders.end() ? &(*it) : nullptr;
    }

    void advanceOrder(int orderId) {
        if (auto* o = findOrder(orderId)) o->advanceStatus();
    }

    void cancelOrder(int orderId) {
        if (auto* o = findOrder(orderId)) o->cancel();
    }

    vector<Order>& getAllOrders() { return orders; }
};

class Database {
private:
    unique_ptr<pqxx::connection> conn;

    Database() {
        conn = make_unique<pqxx::connection>(
            "dbname=smart_restaurant user=postgres password=1234 host=127.0.0.1 port=5432"
        );
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

    vector<MenuItem> loadMenuItems()
    {
        vector<MenuItem> items;

        pqxx::work txn(*conn);

        pqxx::result result = txn.exec(
            "SELECT id, name, price, available FROM menu_items"
        );

        for (auto row : result)
        {
            items.emplace_back(
                row["id"].as<int>(),
                row["name"].as<string>(),
                row["price"].as<double>(),
                row["available"].as<bool>()
            );
        }

        txn.commit();

        return items;
    }


    void addMenuItem(const string& name, double price, bool available)
    {
        pqxx::work txn(*conn);

        txn.exec_params(
            "INSERT INTO menu_items (name, price, available) VALUES ($1, $2, $3)",
            name,
            price,
            available
        );

        txn.commit();
    }


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
