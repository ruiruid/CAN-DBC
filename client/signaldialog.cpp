#include "signaldialog.h"
#include "client.h"
#include "can_message.pb.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <cmath>
#include <QDateTime>

SignalDialog::SignalDialog(const Message& msg, client *clientObj, QWidget *parent)
    : QDialog(parent), message(msg), m_client(clientObj)
{
    setupUI();
}

void SignalDialog::setupUI()
{
    setWindowTitle(QString("设置信号值 - %1 (ID:0x%2)").arg(message.name).arg(message.id, 0, 16));
    resize(800, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 表格：信号名、物理值、原始值、单位、操作
    tableWidget = new QTableWidget(this);
    int rowCount = message.signalList.size();
    tableWidget->setRowCount(rowCount);
    tableWidget->setColumnCount(5);
    tableWidget->setHorizontalHeaderLabels(QStringList() << "信号名" << "物理值" << "原始值" << "单位" << "操作");

    // 为每个信号添加一行
    for (int i = 0; i < rowCount; ++i) {
        const Signal& sig = message.signalList[i];
        tableWidget->setItem(i, 0, new QTableWidgetItem(sig.name));
        QLineEdit *physEdit = new QLineEdit();
        QString rangeHint = QString("范围: %1~%2").arg(sig.min).arg(sig.max);
        physEdit->setPlaceholderText(rangeHint);
        tableWidget->setCellWidget(i, 1, physEdit);
        tableWidget->setItem(i, 2, new QTableWidgetItem(""));   // 原始值暂空
        tableWidget->setItem(i, 3, new QTableWidgetItem(sig.unit));

        QPushButton *singleSendBtn = new QPushButton("发送");
        connect(singleSendBtn, &QPushButton::clicked, this, [this, i]() {
            onSendSignalClicked(i);
        });
        tableWidget->setCellWidget(i, 4, singleSendBtn);

        // 连接物理值编辑框的文本变化信号，带范围校验
        connect(physEdit, &QLineEdit::textChanged, this, [this, i, physEdit]() {
            const Signal& currentSig = message.signalList[i];
            QString text = physEdit->text();
            if (!text.isEmpty()) {
                bool ok;
                double value = text.toDouble(&ok);
                if (ok) {
                    if (value < currentSig.min || value > currentSig.max) {
                        physEdit->setStyleSheet("background-color: #ffcccc;");
                    } else {
                        physEdit->setStyleSheet("background-color: #ccffcc;");
                    }
                } else {
                    physEdit->setStyleSheet("");
                }
            } else {
                physEdit->setStyleSheet("");
            }
            updateFrameDisplay();
        });
    }
    tableWidget->resizeColumnsToContents();

    // 帧数据显示区
    QHBoxLayout *frameLayout = new QHBoxLayout();
    frameLayout->addWidget(new QLabel("打包后CAN帧数据(hex):"));
    frameHexEdit = new QLineEdit();
    frameHexEdit->setReadOnly(true);
    frameLayout->addWidget(frameHexEdit);
    mainLayout->addLayout(frameLayout);

    // 发送按钮
    sendButton = new QPushButton("发送");
    connect(sendButton, &QPushButton::clicked, this, &SignalDialog::onSendClicked);
    mainLayout->addWidget(sendButton);

    mainLayout->addWidget(tableWidget);
    setLayout(mainLayout);

    // 初始更新一次
    updateFrameDisplay();
}

void SignalDialog::onPhysicalValueChanged(int row, int column)
{
    if (column == 1) { // 只有物理值列改变
        updateFrameDisplay();
    }
}

QByteArray SignalDialog::packCanFrame(const QList<double>& physicalValues)
{
    // 打包前检测信号位重叠
    QStringList overlaps = DbcParser::findOverlappingSignals(message);
    if (!overlaps.isEmpty()) {
        qWarning() << "警告: 检测到信号位重叠，打包结果可能不正确！";
        for (const QString& overlap : overlaps) {
            qWarning() << "  -" << overlap;
        }
    }
    
    int dlc = message.dlc;
    QByteArray data(dlc, 0);
    char* bytes = data.data();

    for (int i = 0; i < message.signalList.size(); ++i) {
        const Signal& sig = message.signalList[i];
        double physical = physicalValues[i];
        double raw = (physical - sig.offset) / sig.factor;
        quint64 rawValue = static_cast<quint64>(std::round(raw));
        quint64 maxVal = (sig.length == 64) ? ~0ULL : ((1ULL << sig.length) - 1);
        if (rawValue > maxVal) rawValue = maxVal;

        qDebug() << "打包信号:" << sig.name 
                 << "物理值:" << physical 
                 << "原始值:" << rawValue 
                 << "startBit:" << sig.startBit 
                 << "length:" << sig.length 
                 << "isIntel:" << sig.isIntel;

        if (sig.isIntel) {
            // Intel 小端打包（保持不变）
            int startByte = sig.startBit / 8;
            int startBitInByte = sig.startBit % 8;
            for (int b = 0; b < sig.length; ++b) {
                int bytePos = startByte + (startBitInByte + b) / 8;
                int bitPos = (startBitInByte + b) % 8;
                if (bytePos < dlc) {
                    if (rawValue & (1ULL << b))
                        bytes[bytePos] |= (1 << bitPos);
                    else
                        bytes[bytePos] &= ~(1 << bitPos);
                }
            }
        } else {
            // Motorola 大端打包
            // startBit 是最高有效位（MSB）的位索引
            for (int b = 0; b < sig.length; ++b) {
                // 计算该位在 CAN 数据中的位索引（从最高位向最低位遍历）
                int bitIdx = sig.startBit - b;
                int bytePos = bitIdx / 8;
                int bitInByte = bitIdx % 8;
                if (bytePos < dlc) {
                    // rawValue 的第 (sig.length-1-b) 位对应信号的 b 位（最高位对应 b=0）
                    int rawBit = sig.length - 1 - b;
                    if (rawValue & (1ULL << rawBit))
                        bytes[bytePos] |= (1 << bitInByte);
                    else
                        bytes[bytePos] &= ~(1 << bitInByte);
                }
            }
        }
    }
    return data;
}

QByteArray SignalDialog::packSingleSignal(int signalIndex, double physicalValue)
{
    // 打包前检测信号位重叠
    QStringList overlaps = DbcParser::findOverlappingSignals(message);
    if (!overlaps.isEmpty()) {
        qWarning() << "警告: 检测到信号位重叠，打包结果可能不正确！";
        for (const QString& overlap : overlaps) {
            qWarning() << "  -" << overlap;
        }
    }
    
    int dlc = message.dlc;
    QByteArray data(dlc, 0);
    char* bytes = data.data();

    const Signal& sig = message.signalList[signalIndex];
    double raw = (physicalValue - sig.offset) / sig.factor;
    quint64 rawValue = static_cast<quint64>(std::round(raw));
    quint64 maxVal = (sig.length == 64) ? ~0ULL : ((1ULL << sig.length) - 1);
    if (rawValue > maxVal) rawValue = maxVal;

    if (sig.isIntel) {
        int startByte = sig.startBit / 8;
        int startBitInByte = sig.startBit % 8;
        for (int b = 0; b < sig.length; ++b) {
            int bytePos = startByte + (startBitInByte + b) / 8;
            int bitPos = (startBitInByte + b) % 8;
            if (bytePos < dlc) {
                if (rawValue & (1ULL << b))
                    bytes[bytePos] |= (1 << bitPos);
                else
                    bytes[bytePos] &= ~(1 << bitPos);
            }
        }
    } else {
        for (int b = 0; b < sig.length; ++b) {
            int bitIdx = sig.startBit - b;
            int bytePos = bitIdx / 8;
            int bitInByte = bitIdx % 8;
            if (bytePos < dlc) {
                int rawBit = sig.length - 1 - b;
                if (rawValue & (1ULL << rawBit))
                    bytes[bytePos] |= (1 << bitInByte);
                else
                    bytes[bytePos] &= ~(1 << bitInByte);
            }
        }
    }

    return data;
}

void SignalDialog::updateFrameDisplay()
{
    // 收集当前物理值
    QList<double> physicalValues;
    for (int i = 0; i < message.signalList.size(); ++i) {
        QLineEdit *edit = qobject_cast<QLineEdit*>(tableWidget->cellWidget(i, 1));
        if (edit) {
            bool ok;
            double val = edit->text().toDouble(&ok);
            if (!ok) {
                // 输入无效，暂时设为0
                val = 0.0;
            }
            physicalValues.append(val);
            
            // 计算原始值并限制范围（与打包逻辑一致）
            const Signal& sig = message.signalList[i];
            double raw = (val - sig.offset) / sig.factor;
            quint64 rawValue = static_cast<quint64>(std::round(raw));
            quint64 maxVal = (sig.length == 64) ? ~0ULL : ((1ULL << sig.length) - 1);
            
            // 检查物理值是否超出范围
            bool physicalOutOfRange = (val < sig.min || val > sig.max);
            // 检查原始值是否超出范围
            bool rawOutOfRange = (rawValue > maxVal);
            
            if (physicalOutOfRange && rawOutOfRange) {
                // 物理值和原始值都超出范围
                tableWidget->item(i, 2)->setText(QString::number(maxVal, 'f', 0) + " (已限制)");
                tableWidget->item(i, 2)->setToolTip(QString("物理值 %1 超出范围 [%2, %3]\n原始值 %4 超出范围，已限制为 %5")
                    .arg(val).arg(sig.min).arg(sig.max).arg(rawValue).arg(maxVal));
                tableWidget->item(i, 2)->setBackground(QColor(255, 200, 200));
            } else if (physicalOutOfRange) {
                // 只有物理值超出范围
                tableWidget->item(i, 2)->setText(QString::number(rawValue) + " (物理值超限)");
                tableWidget->item(i, 2)->setToolTip(QString("物理值 %1 超出范围 [%2, %3]")
                    .arg(val).arg(sig.min).arg(sig.max));
                tableWidget->item(i, 2)->setBackground(QColor(255, 200, 200));
            } else if (rawOutOfRange) {
                // 只有原始值超出范围
                tableWidget->item(i, 2)->setText(QString::number(maxVal, 'f', 0) + " (已限制)");
                tableWidget->item(i, 2)->setToolTip(QString("原始值 %1 超出范围，已限制为 %2").arg(rawValue).arg(maxVal));
                tableWidget->item(i, 2)->setBackground(QColor(255, 200, 200));
            } else {
                // 正常情况
                tableWidget->item(i, 2)->setText(QString::number(rawValue));
                tableWidget->item(i, 2)->setToolTip("");
                tableWidget->item(i, 2)->setBackground(QColor(255, 255, 255));
            }
        } else {
            physicalValues.append(0.0);
        }
    }

    QByteArray frame = packCanFrame(physicalValues);
    frameHexEdit->setText(frame.toHex(' ').toUpper());
}


void SignalDialog::onSendClicked()
{
    if (!m_client || !m_client->isConnected()) {
        QMessageBox::warning(this, "发送失败", "WebSocket 未连接，请先连接服务器。");
        return;
    }

    // 获取当前物理值列表并检查范围
    QList<double> physicalValues;
    QStringList physicalOutOfRangeSignals;
    QStringList rawOutOfRangeSignals;
    
    for (int i = 0; i < message.signalList.size(); ++i) {
        QLineEdit *edit = qobject_cast<QLineEdit*>(tableWidget->cellWidget(i, 1));
        double val = 0.0;
        if (edit) {
            bool ok;
            val = edit->text().toDouble(&ok);
            if (!ok) val = 0.0;
            
            const Signal& sig = message.signalList[i];
            
            // 检查物理值范围
            if (val < sig.min || val > sig.max) {
                physicalOutOfRangeSignals.append(QString("%1: %2 (范围: %3~%4)")
                    .arg(sig.name).arg(val).arg(sig.min).arg(sig.max));
            }
            
            // 检查原始值范围
            double raw = (val - sig.offset) / sig.factor;
            quint64 rawValue = static_cast<quint64>(std::round(std::abs(raw)));
            quint64 maxVal = (sig.length == 64) ? ~0ULL : ((1ULL << sig.length) - 1);
            
            if (rawValue > maxVal) {
                rawOutOfRangeSignals.append(QString("%1: 原始值 %2 > 最大值 %3 (将被截断)")
                    .arg(sig.name).arg(rawValue).arg(maxVal));
            }
        }
        physicalValues.append(val);
    }

    // 如果有超出范围的信号，提示用户
    if (!physicalOutOfRangeSignals.isEmpty() || !rawOutOfRangeSignals.isEmpty()) {
        QString message = "检测到以下问题:\n\n";
        
        if (!physicalOutOfRangeSignals.isEmpty()) {
            message += "【物理值超出范围】\n" + physicalOutOfRangeSignals.join("\n") + "\n\n";
        }
        
        if (!rawOutOfRangeSignals.isEmpty()) {
            message += "【原始值超出范围 - 数据将被截断】\n" + rawOutOfRangeSignals.join("\n") + "\n\n";
        }
        
        message += "是否仍要发送?";
        
        QMessageBox::StandardButton reply = QMessageBox::warning(
            this, 
            "信号值异常",
            message,
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            return;
        }
    }

    // 打包成 CAN 帧（用于显示）
    QByteArray frame = packCanFrame(physicalValues);
    QString hexStr = frame.toHex(' ').toUpper();

    // 构造 Protobuf 消息（复杂格式）
    canproto::CanFrame canFrame;
    canFrame.set_message_id(message.id);
    canFrame.set_message_name(message.name.toStdString());

    for (int i = 0; i < message.signalList.size(); ++i) {
        const Signal& sig = message.signalList[i];
        double physical = physicalValues[i];
        double raw = (physical - sig.offset) / sig.factor;
        qint64 rawValue = static_cast<qint64>(std::round(raw));

        canproto::Signal* protoSignal = canFrame.add_signal_list();
        protoSignal->set_name(sig.name.toStdString());
        protoSignal->set_physical_value(physical);
        protoSignal->set_unit(sig.unit.toStdString());
        protoSignal->set_raw_value(rawValue);
    }

    // 序列化 Protobuf 消息
    std::string serialized;
    if (!canFrame.SerializeToString(&serialized)) {
        QMessageBox::warning(this, "发送失败", "Protobuf 序列化失败");
        return;
    }

    QByteArray protoData = QByteArray::fromStdString(serialized);
    m_client->sendBinaryMessage(protoData);

    // 生成日志条目
    QString logEntry = QString("[%1] 发送 CAN 帧 [Protobuf] ID:0x%2 Name:%3 数据:%4")
                           .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                           .arg(message.id, 0, 16)
                           .arg(message.name)
                           .arg(hexStr);
    emit logMessage(logEntry);
}

void SignalDialog::onSendSignalClicked(int row)
{
    if (!m_client || !m_client->isConnected()) {
        QMessageBox::warning(this, "发送失败", "WebSocket 未连接，请先连接服务器。");
        return;
    }

    // 获取单个信号的物理值
    QLineEdit *edit = qobject_cast<QLineEdit*>(tableWidget->cellWidget(row, 1));
    double physicalValue = 0.0;
    if (edit) {
        bool ok;
        physicalValue = edit->text().toDouble(&ok);
        if (!ok) physicalValue = 0.0;
    }

    const Signal& sig = message.signalList[row];
    
    // 检查物理值范围
    bool physicalOutOfRange = (physicalValue < sig.min || physicalValue > sig.max);
    
    // 检查原始值范围
    double raw = (physicalValue - sig.offset) / sig.factor;
    quint64 rawValue = static_cast<quint64>(std::round(std::abs(raw)));
    quint64 maxVal = (sig.length == 64) ? ~0ULL : ((1ULL << sig.length) - 1);
    bool rawOutOfRange = (rawValue > maxVal);
    
    // 如果有超出范围的情况，提示用户
    if (physicalOutOfRange || rawOutOfRange) {
        QString message = QString("信号 %1:\n\n").arg(sig.name);
        
        if (physicalOutOfRange) {
            message += QString("【物理值超出范围】\n值 %1 超出范围 [%2, %3]\n\n")
                .arg(physicalValue).arg(sig.min).arg(sig.max);
        }
        
        if (rawOutOfRange) {
            message += QString("【原始值超出范围 - 数据将被截断】\n原始值 %1 > 最大值 %2\n\n")
                .arg(rawValue).arg(maxVal);
        }
        
        message += "是否仍要发送?";
        
        QMessageBox::StandardButton reply = QMessageBox::warning(
            this,
            "信号值异常",
            message,
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            return;
        }
    }

    // 打包单个信号到 CAN 帧
    QByteArray frame = packSingleSignal(row, physicalValue);
    QString hexStr = frame.toHex(' ').toUpper();

    // 构造 Protobuf 消息（只包含单个信号）
    canproto::CanFrame canFrame;
    canFrame.set_message_id(message.id);
    canFrame.set_message_name(message.name.toStdString());

    canproto::Signal* protoSignal = canFrame.add_signal_list();
    protoSignal->set_name(sig.name.toStdString());
    protoSignal->set_physical_value(physicalValue);
    protoSignal->set_unit(sig.unit.toStdString());
    protoSignal->set_raw_value(static_cast<qint64>(rawValue));

    // 序列化 Protobuf 消息
    std::string serialized;
    if (!canFrame.SerializeToString(&serialized)) {
        QMessageBox::warning(this, "发送失败", "Protobuf 序列化失败");
        return;
    }

    QByteArray protoData = QByteArray::fromStdString(serialized);
    m_client->sendBinaryMessage(protoData);

    // 生成日志条目
    QString logEntry = QString("[%1] 发送单个信号 [Protobuf] ID:0x%2 Signal:%3 Value:%4%5 数据:%6")
                           .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                           .arg(message.id, 0, 16)
                           .arg(sig.name)
                           .arg(physicalValue)
                           .arg(sig.unit)
                           .arg(hexStr);
    emit logMessage(logEntry);
}
