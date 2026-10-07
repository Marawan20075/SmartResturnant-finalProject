#include "loginpage.h"
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTabWidget>


LoginPage::LoginPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel("Smart Restaurant", this);
    auto *loginTabs = new QTabWidget(this);

    auto *customerTab = new QWidget(loginTabs);
    auto *customerLayout = new QVBoxLayout(customerTab);

    customerEmailEdit = new QLineEdit(customerTab);
    customerEmailEdit->setPlaceholderText("Customer email address");

    auto *customerLoginButton = new QPushButton("Log in as Customer", customerTab);
    
    customerLayout->addWidget(customerEmailEdit);
    customerLayout->addWidget(customerLoginButton);

    
    auto *staffTab = new QWidget(loginTabs);
    auto *staffLayout = new QVBoxLayout(staffTab);

    staffEmailEdit = new QLineEdit(staffTab);
    staffEmailEdit->setPlaceholderText("Staff email address");
    
    auto *staffLoginButton = new QPushButton("Log in as Staff", staffTab);
    staffLayout->addWidget(staffEmailEdit);
    staffLayout->addWidget(staffLoginButton);
    

    loginTabs->addTab(customerTab, "Customer");
    loginTabs->addTab(staffTab, "Staff");

    errorLabel = new QLabel(this);
    errorLabel->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(loginTabs);
    layout->addWidget(errorLabel);

    connect(customerLoginButton, &QPushButton::clicked,
            this, &LoginPage::onCustomerLoginClicked);
    connect(customerEmailEdit, &QLineEdit::returnPressed,
            this, &LoginPage::onCustomerLoginClicked);
    connect(staffLoginButton, &QPushButton::clicked,
            this, &LoginPage::onStaffLoginClicked);
    connect(staffEmailEdit, &QLineEdit::returnPressed,
            this, &LoginPage::onStaffLoginClicked);
    connect(loginTabs, &QTabWidget::currentChanged, this, [this](int) {
        errorLabel->clear();
    });
}

void LoginPage::showError(const QString &message) {
    errorLabel->setText(message);
}

void LoginPage::onCustomerLoginClicked() {
    emit customerLoginRequested(customerEmailEdit->text().trimmed());
}

void LoginPage::onStaffLoginClicked() {
    emit staffLoginRequested(staffEmailEdit->text().trimmed());
}
