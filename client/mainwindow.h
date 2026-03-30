#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "client.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectTriggered();
    void onLoadDbc();
    void onShowLog();                 // 新增：显示日志窗口
    void addLogEntry(const QString& entry);  // 新增：添加日志条目

private:
    Ui::MainWindow *ui;
    client *clientWindow;   // 连接客户端窗口
    QStringList m_logEntries;         // 存储所有日志
};

#endif // MAINWINDOW_H
