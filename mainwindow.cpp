#include "mainwindow.h"
#include "loginpage.h"
#include "customerpage.h"
#include "staffpage.h"
#include <QStackedWidget>
#include <QRandomGenerator>

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

    // Mohemmm very important todo: only for now should be replaced when UserManager is implemented 
    const int customerId = QRandomGenerator::global()->bounded(1, 1000000);

    customerPage->startCustomerSession(customerId);
    loginPage->showError("");
    pages->setCurrentWidget(customerPage);
}

void MainWindow::handleStaffLoginRequested(const QString &email) {
    if (email.trimmed().isEmpty() || !email.contains('@')) {
        loginPage->showError("Enter a valid email.");
        return;
    }

    staffPage->refreshOrders();
    loginPage->showError("");
    pages->setCurrentWidget(staffPage);
}
