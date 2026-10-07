#include "mainwindow.h"
#include "loginpage.h"
#include "customerpage.h"
#include "staffpage.h"
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    setWindowTitle("Smart Restaurant - Order Tracking");
    resize(700, 500);

    pages = new QStackedWidget(this);

    loginPage = new LoginPage(pages);
    customerPage = new CustomerPage(pages);
    staffPage = new StaffPage(pages);

    pages->addWidget(loginPage);
    pages->addWidget(customerPage);
    pages->addWidget(staffPage);

    setCentralWidget(pages);
    pages->setCurrentWidget(loginPage);

    connect(loginPage, &LoginPage::staffLoginRequested,
            this, &MainWindow::handleStaffLoginRequested);
    connect(loginPage, &LoginPage::customerLoginRequested,
            this, &MainWindow::handleCustomerLoginRequested);
}

void MainWindow::handleCustomerLoginRequested(const QString &email) {
    if (email.trimmed().isEmpty() || !email.contains('@')) {
        loginPage->showError("Enter a valid email.");
        return;
    }

    try {
        auto user = OrderManager::instance().findUserByEmail(email.trimmed().toStdString());
        if (!user) {
            loginPage->showError("Email not found.");
            return;
        }
        if (user->role() != "Customer") {
            loginPage->showError("This is a staff email. Use the Staff tab.");
            return;
        }

        customerPage->startCustomerSession(user->getId());
        loginPage->showError("");
        pages->setCurrentWidget(customerPage);
    } catch (const std::exception &error) {
        loginPage->showError("Could not log in: " + QString::fromUtf8(error.what()));
    }
}

void MainWindow::handleStaffLoginRequested(const QString &email) {
    if (email.trimmed().isEmpty() || !email.contains('@')) {
        loginPage->showError("Enter a valid email.");
        return;
    }

    try {
        auto user = OrderManager::instance().findUserByEmail(email.trimmed().toStdString());
        if (!user) {
            loginPage->showError("Email not found.");
            return;
        }
        if (user->role() != "Staff") {
            loginPage->showError("This is a customer email. Use the Customer tab.");
            return;
        }

        staffPage->refreshOrders();
        loginPage->showError("");
        pages->setCurrentWidget(staffPage);
    } catch (const std::exception &error) {
        loginPage->showError("Could not log in: " + QString::fromUtf8(error.what()));
    }
}
