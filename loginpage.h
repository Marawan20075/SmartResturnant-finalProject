#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;

class LoginPage : public QWidget {
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);
    void showError(const QString &message);

signals:
    void customerLoginRequested(const QString &email);
    void staffLoginRequested(const QString &email);

private slots:
    void onCustomerLoginClicked();
    void onStaffLoginClicked();

private:
    QLineEdit *customerEmailEdit = nullptr;
    QLineEdit *staffEmailEdit = nullptr;
    QLabel *errorLabel = nullptr;
};
