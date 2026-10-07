BEGIN;

INSERT INTO users (id, name, email, role, address, position) VALUES
    (1, 'Test Customer',  'customer@smartrestaurant.test', 'customer', '12 Nile St, Cairo', NULL),
    (2, 'Second Customer','customer2@smartrestaurant.test','customer', '5 Tahrir Sq, Cairo', NULL),
    (3, 'Head Chef',      'chef@smartrestaurant.test',     'staff',    NULL, 'Chef'),
    (4, 'Delivery Driver','driver@smartrestaurant.test',   'staff',    NULL, 'Delivery Driver'),
    (5, 'Manager',        'manager@smartrestaurant.test',  'staff',    NULL, 'Manager')
ON CONFLICT (id) DO NOTHING;

INSERT INTO menu_items (id, name, price, available) VALUES
    (1, 'Classic Burger',   9.99,  true),
    (2, 'Chicken Shawarma', 7.50,  true),
    (3, 'Margherita Pizza', 11.25, true),
    (4, 'Koshary',          4.00,  true),
    (5, 'Fresh Orange Juice', 2.50, true),
    (6, 'Seasonal Special', 14.00, false)   -- hidden from customers
ON CONFLICT (id) DO NOTHING;

INSERT INTO orders (id, customer_id) VALUES
    (1, 1)
ON CONFLICT (id) DO NOTHING;

INSERT INTO order_items (order_id, menu_item_id, quantity, unit_price)
SELECT 1, m.id, q.quantity, m.price
FROM (VALUES (1, 2), (5, 1)) AS q(menu_item_id, quantity)
JOIN menu_items m ON m.id = q.menu_item_id
ON CONFLICT (order_id, menu_item_id) DO NOTHING;

SELECT setval(pg_get_serial_sequence('users', 'id'),       (SELECT MAX(id) FROM users));
SELECT setval(pg_get_serial_sequence('menu_items', 'id'),  (SELECT MAX(id) FROM menu_items));
SELECT setval(pg_get_serial_sequence('orders', 'id'),      (SELECT MAX(id) FROM orders));

COMMIT;
