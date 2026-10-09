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

   On Windows with a Qt MinGW kit, the build copies libpqxx's runtime DLLs
   (libpq, libstdc++, libwinpthread, OpenSSL, ...) next to the exe. This stops
   the "entry point nanosleep64 could not be located" error, which happens
   when libpqxx (e.g. from MSYS2) was built with a newer MinGW than the Qt kit.
   If you still see it, delete the build folder and run CMake again.
3. Create the database and load the schema and test data:
   ```
   createdb -U postgres smart_restaurant
   psql -U postgres -d smart_restaurant -f schema.sql
   psql -U postgres -d smart_restaurant -f seed.sql
   ```
   If you already ran an older `schema.sql` and want to keep your data, run
   `migrate_existing_db.sql` once instead of `schema.sql`.
4. Tell the app how to connect. Credentials are **not** stored in the source
   code; set environment variables instead (never commit real passwords):
   - Either `SMART_RESTAURANT_DB`, a full libpq connection string, e.g.
     `host=127.0.0.1 port=5432 dbname=smart_restaurant user=postgres password=...`
   - Or the standard PostgreSQL variables `PGHOST`, `PGPORT`, `PGDATABASE`,
     `PGUSER`, `PGPASSWORD` (defaults: `127.0.0.1`, `5432`, `smart_restaurant`,
     `postgres`, no password). A `pgpass.conf` file also works for the password.

   PowerShell example (current session only):
   ```
   $env:PGPASSWORD = "your-local-password"
   ```
   If the connection fails, the app shows a "Database Connection Error" dialog
   with the reason and exits.
5. Optional: build and run the database integration tests (they create and then
   delete their own test rows):
   ```
   cmake -DSMART_RESTAURANT_BUILD_TESTS=ON ..
   cmake --build . --target DatabaseTests
   ./DatabaseTests
   ```
6. Each member works on their own branch:
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
