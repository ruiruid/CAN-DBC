#ifndef SIGNALDIALOG_H
#define SIGNALDIALOG_H

#include <QDialog>
#include <QMap>
#include "dbcparser.h"

class QTableWidget;
class QLineEdit;
class QPushButton;
class client;

class SignalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignalDialog(const Message& msg, client *clientObj, QWidget *parent = nullptr);

private slots:
    void onPhysicalValueChanged(int row, int column);
    void onSendClicked();
    void onSendSignalClicked(int row);

signals:
    void logMessage(const QString& msg);

private:
    Message message;
    client *m_client;
    QTableWidget *tableWidget;
    QLineEdit *frameHexEdit;
    QPushButton *sendButton;

    void setupUI();
    QByteArray packCanFrame(const QList<double>& physicalValues);
    QByteArray packSingleSignal(int signalIndex, double physicalValue);
    void updateFrameDisplay();
};

#endif // SIGNALDIALOG_H
