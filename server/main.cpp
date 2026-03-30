#include "mainwindow.h"
#include "server.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    MainWindow w;
    w.show();
    
    Server server(8080);
    QObject::connect(&server, &Server::messageReceived,
                     &w, &MainWindow::onMessageReceived);
    
    return a.exec();
}
