#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Server;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onMessageReceived(const QString &message);

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QTextEdit *m_textEdit;
    Server *m_server;
};
#endif // MAINWINDOW_H
