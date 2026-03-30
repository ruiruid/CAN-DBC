#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dbcparser.h"
#include "dbcdialog.h"
#include "signaldialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , clientWindow(new client)
{
    ui->setupUi(this);
    connect(ui->actionopen, &QAction::triggered, this, &MainWindow::onLoadDbc);
    connect(ui->actionconnect, &QAction::triggered, this, &MainWindow::onConnectTriggered);
    connect(ui->actionlog, &QAction::triggered, this, &MainWindow::onShowLog);
    connect(ui->actiondisconnect, &QAction::triggered, clientWindow, &client::disconnectFromServer);

    QLabel *statusLabel = new QLabel("状态: 断开");
    statusLabel->setStyleSheet("color: gray; font-weight: bold; padding: 5px;");
    ui->statusbar->addWidget(statusLabel);

    connect(clientWindow, &client::connected, this, [this, statusLabel](){
        ui->actionconnect->setEnabled(false);
        ui->actiondisconnect->setEnabled(true);
        statusLabel->setText("状态: 已连接");
        statusLabel->setStyleSheet("color: green; font-weight: bold; padding: 5px;");
    });
    connect(clientWindow, &client::disconnected, this, [this, statusLabel](){
        ui->actionconnect->setEnabled(true);
        ui->actiondisconnect->setEnabled(false);
        QString currentText = statusLabel->text();
        if (!currentText.contains("重连中")) {
            statusLabel->setText("状态: 断开");
            statusLabel->setStyleSheet("color: gray; font-weight: bold; padding: 5px;");
        }
    });
    connect(clientWindow, &client::reconnecting, this, [statusLabel](int current, int max){
        statusLabel->setText(QString("状态: 重连中 (%1/%2)...").arg(current).arg(max));
        statusLabel->setStyleSheet("color: orange; font-weight: bold; padding: 5px;");
    });
    connect(clientWindow, &client::logMessage, this, &MainWindow::addLogEntry);

    ui->actiondisconnect->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnectTriggered()
{
    clientWindow->clearAddress();  // 清空地址输入框
    clientWindow->show();
}

void MainWindow::onLoadDbc()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择 DBC 文件", "", "DBC Files (*.dbc)");
    if (filePath.isEmpty())
        return;

    QList<Message> messages;
    if (!DbcParser::parseFile(filePath, messages)) {
        QMessageBox::warning(this, "解析失败", "无法解析 DBC 文件，请检查格式。");
        return;
    }

    // 验证所有 Message 的信号位重叠
    for (const Message& msg : messages) {
        DbcParser::validateMessageSignals(msg);
        DbcParser::validateMessageRanges(msg);
    }

    DbcDisplayWindow *subWindow = new DbcDisplayWindow(messages);
    ui->mdiArea->addSubWindow(subWindow);
    subWindow->setWindowTitle(QFileInfo(filePath).fileName());
    subWindow->resize(800, 600);
    subWindow->show();

    connect(subWindow, &DbcDisplayWindow::messageSelected, this, [this](const Message& msg){
        SignalDialog *dialog = new SignalDialog(msg, clientWindow, this);
        // 连接日志信号
        connect(dialog, &SignalDialog::logMessage, this, &MainWindow::addLogEntry);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
}

void MainWindow::addLogEntry(const QString& entry)
{
    m_logEntries.append(entry);
    // 可选：同时输出到调试控制台
    qDebug() << entry;
}

void MainWindow::onShowLog()
{
    QDialog *logDialog = new QDialog(this);
    logDialog->setWindowTitle("发送日志");
    logDialog->setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    logDialog->resize(600, 400);
    QVBoxLayout *layout = new QVBoxLayout(logDialog);
    QTextEdit *textEdit = new QTextEdit(logDialog);
    textEdit->setReadOnly(true);

    if (m_logEntries.isEmpty()) {
        textEdit->setPlainText("暂无发送记录。");
    } else {
        textEdit->setPlainText(m_logEntries.join("\n"));
    }

    QPushButton *closeBtn = new QPushButton("关闭", logDialog);
    layout->addWidget(textEdit);
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, logDialog, &QDialog::accept);
    logDialog->setAttribute(Qt::WA_DeleteOnClose);
    logDialog->show();
}
