#pragma once

#include <QMainWindow>
#include <QString>

class QStackedWidget;
class LoginPage;
class CustomerPage;
class StaffPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void handleCustomerLoginRequested(const QString &email);
    void handleStaffLoginRequested(const QString &email);

private:
    QStackedWidget *pages = nullptr;
    LoginPage *loginPage = nullptr;
    CustomerPage *customerPage = nullptr;
    StaffPage *staffPage = nullptr;
};
