#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include <QtSql/QSqlDatabase>

namespace Ui {
class Login;
}

class Login : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_pushButton_clicked();       // 登录按钮
//    void on_registerButton_clicked();  // 注册按钮

private:
    Ui::Login *ui;
    QSqlDatabase mydb;
    void initializeDatabase();
};

#endif // LOGIN_H
