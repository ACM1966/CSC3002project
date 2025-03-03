#include <bits/stdc++.h>

#include "login.h"
#include "ui_login.h"
#include "dialog.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QCryptographicHash>

Login::Login(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);

    mydb = QSqlDatabase::addDatabase("QSQLITE");
    mydb.setDatabaseName(QCoreApplication::applicationDirPath() + "/database.db");

    if (mydb.open()) {
        ui->label_4->setText("Database Connected... :)");
        initializeDatabase(); // 初始化数据库和表
    } else {
        ui->label_4->setText("Database Dis-Connected... :(");
    }
}

Login::~Login()
{
    delete ui;
}

void Login::initializeDatabase()
{
    if (!mydb.isOpen()) {
        if (!mydb.open()) {
            qDebug() << "Failed to open database:" << mydb.lastError().text();
            return;
        }
    }

    QSqlQuery query;

    // 创建表
    QString createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS login (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username VARCHAR(255) NOT NULL UNIQUE,
            password VARCHAR(255) NOT NULL
        );
    )";

    qDebug() << "Database path:" << mydb.databaseName();
    if (!query.exec(createTableSQL)) {
        qDebug() << "Error creating table:" << query.lastError().text();
        return;
    } else {
        qDebug() << "Table 'login' created successfully or already exists.";
    }

    // 检查是否有用户数据
    QString checkUserSQL = "SELECT COUNT(*) FROM login";
    if (query.exec(checkUserSQL) && query.next()) {
        int count = query.value(0).toInt();
        if (count == 0) {
            // 插入默认用户
            QString insertDefaultUserSQL = R"(
                INSERT INTO login (username, password)
                VALUES ('user1', 'user123')
            )";
            if (!query.exec(insertDefaultUserSQL)) {
                qDebug() << "Error inserting default user:" << query.lastError().text();
            } else {
                qDebug() << "Default user inserted successfully.";
            }
        }
    } else {
        qDebug() << "Error checking user count:" << query.lastError().text();
    }
}

void Login::on_pushButton_clicked()
{
    QString username, password;
    username = ui->lineEdit_username->text();
    password = ui->lineEdit_2_password->text();

    if (!mydb.open()) {
        qDebug() << "Not Connected to Database :(";
        return;
    }

    QSqlQuery qry;

    // 查询用户
    if (qry.exec("SELECT * FROM login WHERE username = '" + username + "' AND password = '" + password + "'")) {
        int count = 0;
        if (qry.next()) {
            count++;
        }

        if (count == 1) {
            ui->label_4->setText("Username and password is correct");

            // 执行外部文件
            QString program = QCoreApplication::applicationDirPath() + "/CrustyPirate.exe"; // 修改为你的文件名
            QProcess process;
            if (!process.startDetached(program)) {
                qDebug() << "Failed to start program:" << program;
            } else {
                qDebug() << "Program started successfully:" << program;
            }

            // 显示对话框
            {
                Dialog dialog;
                dialog.setModal(true);
                dialog.exec();
            }

            // 隐藏当前窗口
            this->hide();
        }
        else if (count > 1) {
            ui->label_4->setText("Duplicate username and password detected");
        }
        else {
            ui->label_4->setText("Username or password is incorrect");
        }
    } else {
        qDebug() << "Error executing query:" << qry.lastError().text();
    }
}
