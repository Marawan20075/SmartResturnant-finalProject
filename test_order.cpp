#include <cassert>
#include <cmath>
#include <iostream>
#include "SmartRestaurant_Template_v2.hpp"


int main()
{
	Order order(1, 100);
	order.addItem(OrderItem(MenuItem(1, "Burger", 5.50, true), 2));
	order.addItem(OrderItem(MenuItem(2, "Fries", 2.00, true), 3));

	assert(order.getItems().size() == 2);
	assert(fabs(order.total() - 17.00) < 0.001);


	assert(order.getStatus() == OrderStatus::Pending);
	order.advanceStatus();

	assert(order.getStatus() == OrderStatus::Preparing);
	order.advanceStatus();

	assert(order.getStatus() == OrderStatus::OutForDelivery);
	order.advanceStatus();

	assert(order.getStatus() == OrderStatus::Delivered);
	order.advanceStatus();


	bool threw = false;
	try { order.cancel(); }catch (const logic_error&) { threw = true; }
	assert(threw);
	assert(order.getStatus() == OrderStatus::Delivered);

	Order o2(2, 100);
	o2.cancel();
	assert(o2.getStatus() == OrderStatus::Cancelled);

	cout << "All tests passed!\n";
	return 0;




}
