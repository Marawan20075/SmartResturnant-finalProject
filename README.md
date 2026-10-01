# Smart Restaurant & Order Tracking System

Final Project — Summer Training 2026 (C++ | PostgreSQL | GUI | OOP | SOLID | Design Patterns)

## Repository Structure

```
/core
   SmartRestaurant_Template_v2.hpp   -> all core classes (User, MenuItem, Order, OrderManager, Database)
/database
   schema.sql                        -> PostgreSQL schema (users, menu_items, orders, order_items)
/docs
   SRS_SmartRestaurant.docx          -> Software Requirements Specification
/gui
   (Qt screens go here)
README.md
```

## Team & Responsibilities

| Team | Members | Responsibility |
|---|---|---|
| Core / Logic | Rahma Ayman, Salma Ezzat, Khairy | User/Customer/StaffMember, MenuItem, Order/OrderItem, OrderManager |
| Database | Ereny Emad, Youssef Ahmed | Run schema.sql, write queries, extend the Database class |
| GUI | Marawan Mohamed (Leader), Marwan Osama | Qt screens, wiring UI events to OrderManager |

## Architecture Rule

GUI -> OrderManager -> Database

The GUI never talks to the database directly. All business rules live in
OrderManager and the core classes.

## Getting Started

1. Clone the repo:
   ```
   git clone <repo-url>
   ```
2. Install dependencies:
   - PostgreSQL
   - libpqxx (`vcpkg install libpqxx` on Windows, `apt install libpqxx-dev` on Linux)
   - Qt (for the GUI team)
3. Run `database/schema.sql` against your PostgreSQL instance.
4. Update the connection string inside `Database`'s constructor in
   `SmartRestaurant_Template_v2.hpp` with your local credentials.
5. Each member works on their own branch:
   ```
   git checkout -b <your-name>-<feature>
   ```
   and opens a pull request into `main` when ready.

## Design Patterns Used

- Singleton (OrderManager, Database)
- State (Order status: Pending -> Preparing -> OutForDelivery -> Delivered)
- Inheritance / Polymorphism (User -> Customer / StaffMember)

## Deadline

Final submission: 10 Oct 2026
