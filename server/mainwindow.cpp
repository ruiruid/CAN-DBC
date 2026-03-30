#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "server.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_textEdit(new QTextEdit(this))
    , m_server(nullptr)
{
    ui->setupUi(this);

    // 创建文本显示区域
    m_textEdit->setReadOnly(true);
    m_textEdit->setPlaceholderText("Waiting for messages...");
    
    // 设置字体
    QFont font("Courier New", 10);
    m_textEdit->setFont(font);

    // 将文本框添加到布局中
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_textEdit);
    
    // 创建一个中心widget并设置布局
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    // 设置窗口大小
    resize(800, 600);
    setWindowTitle("CAN Server - WebSocket on port 8080");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onMessageReceived(const QString &message)
{
    m_textEdit->append(message);
    m_textEdit->append(""); // 添加空行分隔
}

void MainWindow::on_pushButton_clicked()
{
    // 清空文本框
    m_textEdit->clear();
}
