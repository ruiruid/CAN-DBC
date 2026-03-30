#include "dbcdialog.h"
#include <QVBoxLayout>

DbcDisplayWindow::DbcDisplayWindow(const QList<Message>& messages, QWidget *parent)
    : QMdiSubWindow(parent), messages(messages)
{
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabels(QStringList() << "名称" << "起始位" << "长度" << "字节序"
                               << "因子" << "偏移" << "最小值" << "最大值" << "单位");
    treeWidget->setColumnWidth(0, 150);
    setWidget(treeWidget);

    // 填充树形结构
    for (const Message& msg : messages) {
        QString msgTitle = QString("ID: %1  %2  DLC: %3")
                               .arg(msg.id)
                               .arg(msg.name)
                               .arg(msg.dlc);
        QTreeWidgetItem* msgItem = new QTreeWidgetItem(treeWidget);
        msgItem->setText(0, msgTitle);
        msgItem->setFirstColumnSpanned(true);
        // 存储消息指针（通过 UserRole 或者直接存储索引，这里存储索引）
        msgItem->setData(0, Qt::UserRole, QVariant::fromValue(msg.id)); // 可选，用于快速查找

        for (const Signal& sig : msg.signalList) {
            QTreeWidgetItem* sigItem = new QTreeWidgetItem(msgItem);
            sigItem->setText(0, sig.name);
            sigItem->setText(1, QString::number(sig.startBit));
            sigItem->setText(2, QString::number(sig.length));
            sigItem->setText(3, sig.isIntel ? "Intel" : "Motorola");
            sigItem->setText(4, QString::number(sig.factor));
            sigItem->setText(5, QString::number(sig.offset));
            sigItem->setText(6, QString::number(sig.min));
            sigItem->setText(7, QString::number(sig.max));
            sigItem->setText(8, sig.unit);
        }
        msgItem->setExpanded(true);
    }

    connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &DbcDisplayWindow::onItemDoubleClicked);
}

void DbcDisplayWindow::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    // 寻找最顶层的消息项
    QTreeWidgetItem *msgItem = item;
    while (msgItem && msgItem->parent() != nullptr) {
        msgItem = msgItem->parent();
    }
    if (!msgItem) return;

    // 根据消息标题找到对应的 Message 对象
    QString msgTitle = msgItem->text(0);
    // 从标题中提取 ID 或名称（简单匹配）
    for (const Message& msg : messages) {
        QString title = QString("ID: %1  %2  DLC: %3")
                            .arg(msg.id)
                            .arg(msg.name)
                            .arg(msg.dlc);
        if (title == msgTitle) {
            emit messageSelected(msg);
            break;
        }
    }
}
