-- Upgrades a database created with the ORIGINAL schema.sql to the current one.
-- Only needed if you already ran the old schema and want to keep your data;
-- for a fresh database just run schema.sql (and seed.sql).
-- Run once. Everything is in one transaction, so a failure changes nothing.
--
-- It will fail (and roll back) if existing data breaks the new rules, e.g.
-- a staff user whose position is not 'Chef' / 'Delivery Driver' / 'Manager',
-- or the same menu item appearing twice in one order. Fix those rows first.

BEGIN;

ALTER TABLE users
    ADD CONSTRAINT users_position_check CHECK (
        (role = 'staff' AND position IN ('Chef', 'Delivery Driver', 'Manager'))
        OR (role = 'customer' AND position IS NULL)
    );

ALTER TABLE orders ALTER COLUMN created_at TYPE TIMESTAMPTZ;

ALTER TABLE orders DROP CONSTRAINT orders_customer_id_fkey;
ALTER TABLE orders
    ADD CONSTRAINT orders_customer_id_fkey
    FOREIGN KEY (customer_id) REFERENCES users(id) ON DELETE RESTRICT;

ALTER TABLE order_items DROP CONSTRAINT order_items_menu_item_id_fkey;
ALTER TABLE order_items
    ADD CONSTRAINT order_items_menu_item_id_fkey
    FOREIGN KEY (menu_item_id) REFERENCES menu_items(id) ON DELETE RESTRICT;

-- Backfill historical prices from the current menu price.
ALTER TABLE order_items ADD COLUMN unit_price NUMERIC(10,2);
UPDATE order_items oi
SET unit_price = m.price
FROM menu_items m
WHERE m.id = oi.menu_item_id;
ALTER TABLE order_items ALTER COLUMN unit_price SET NOT NULL;
ALTER TABLE order_items ADD CONSTRAINT order_items_unit_price_check CHECK (unit_price >= 0);

ALTER TABLE order_items
    ADD CONSTRAINT order_items_order_menu_item_key UNIQUE (order_id, menu_item_id);

CREATE INDEX IF NOT EXISTS idx_orders_customer_id ON orders(customer_id);
CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
CREATE INDEX IF NOT EXISTS idx_order_items_order_id ON order_items(order_id);

CREATE OR REPLACE FUNCTION enforce_order_status_transition() RETURNS trigger AS $$
BEGIN
    IF NEW.status = OLD.status THEN
        RETURN NEW;
    END IF;

    IF (OLD.status = 'pending'          AND NEW.status = 'preparing')
    OR (OLD.status = 'preparing'        AND NEW.status = 'out_for_delivery')
    OR (OLD.status = 'out_for_delivery' AND NEW.status = 'delivered')
    OR (OLD.status IN ('pending', 'preparing', 'out_for_delivery') AND NEW.status = 'cancelled') THEN
        RETURN NEW;
    END IF;

    RAISE EXCEPTION 'Invalid order status transition: % -> %', OLD.status, NEW.status;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS orders_status_transition ON orders;
CREATE TRIGGER orders_status_transition
    BEFORE UPDATE OF status ON orders
    FOR EACH ROW EXECUTE FUNCTION enforce_order_status_transition();

COMMIT;
