#ifndef DBCDIALOG_H
#define DBCDIALOG_H

#include <QMdiSubWindow>
#include <QTreeWidget>
#include "dbcparser.h"

class DbcDisplayWindow : public QMdiSubWindow
{
    Q_OBJECT

public:
    explicit DbcDisplayWindow(const QList<Message>& messages, QWidget *parent = nullptr);

signals:
    void messageSelected(const Message& msg);   // 双击消息时发射

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidget *treeWidget;
    QList<Message> messages;    // 存储消息列表，供查询
};

#endif // DBCDIALOG_H
