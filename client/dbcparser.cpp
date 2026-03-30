#include "dbcparser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

bool DbcParser::parseFile(const QString& filePath, QList<Message>& messages) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open DBC file:" << filePath;
        return false;
    }

    messages.clear();
    QTextStream in(&file);

    QRegularExpression boRe(R"(^BO_\s+(\d+)\s+(\w+)\s*:\s*(\d+)\s+\w+)");
    QRegularExpression sgRe(R"(^SG_\s+(\w+)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s+\(([-0-9.eE+]+),([-0-9.eE+]+)\)\s+\[([-0-9.eE+]+)\|([-0-9.eE+]+)\]\s+\"([^\"]*)\")");

    Message currentMsg;
    bool inMessage = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("/*"))
            continue;

        // 匹配 BO_ 行
        QRegularExpressionMatch boMatch = boRe.match(line);
        if (boMatch.hasMatch()) {
            if (inMessage) {
                // 保存之前的 message
                messages.append(currentMsg);
            }
            currentMsg.id = boMatch.captured(1).toUInt();
            currentMsg.name = boMatch.captured(2);
            currentMsg.dlc = static_cast<uint8_t>(boMatch.captured(3).toInt());
            currentMsg.signalList.clear();
            inMessage = true;
            continue;
        }

        // 匹配 SG_ 行
        QRegularExpressionMatch sgMatch = sgRe.match(line);
        if (sgMatch.hasMatch() && inMessage) {
            Signal sig;
            sig.name = sgMatch.captured(1);
            sig.startBit = sgMatch.captured(2).toInt();
            sig.length = sgMatch.captured(3).toInt();
            sig.isIntel = (sgMatch.captured(4) == "1");
            sig.isSigned = (sgMatch.captured(5) == "-");
            sig.factor = sgMatch.captured(6).toDouble();
            sig.offset = sgMatch.captured(7).toDouble();
            sig.min = sgMatch.captured(8).toDouble();
            sig.max = sgMatch.captured(9).toDouble();
            sig.unit = sgMatch.captured(10);
            currentMsg.signalList.append(sig);
        }
    }

    if (inMessage) {
        messages.append(currentMsg);
    }

    file.close();
    return !messages.isEmpty();
}

bool DbcParser::checkSignalOverlap(const Signal& sig1, const Signal& sig2) {
    int start1 = sig1.startBit;
    int end1 = sig1.startBit + sig1.length - 1;
    
    int start2 = sig2.startBit;
    int end2 = sig2.startBit + sig2.length - 1;
    
    return !(end1 < start2 || end2 < start1);
}

QStringList DbcParser::findOverlappingSignals(const Message& msg) {
    QStringList overlappingPairs;
    
    for (int i = 0; i < msg.signalList.size(); ++i) {
        for (int j = i + 1; j < msg.signalList.size(); ++j) {
            if (checkSignalOverlap(msg.signalList[i], msg.signalList[j])) {
                QString pair = QString("%1 (位%2-%3) 和 %4 (位%5-%6)")
                    .arg(msg.signalList[i].name)
                    .arg(msg.signalList[i].startBit)
                    .arg(msg.signalList[i].startBit + msg.signalList[i].length - 1)
                    .arg(msg.signalList[j].name)
                    .arg(msg.signalList[j].startBit)
                    .arg(msg.signalList[j].startBit + msg.signalList[j].length - 1);
                overlappingPairs.append(pair);
            }
        }
    }
    
    return overlappingPairs;
}

void DbcParser::validateMessageSignals(const Message& msg) {
    QStringList overlaps = findOverlappingSignals(msg);
    
    if (!overlaps.isEmpty()) {
        qWarning() << "====== 信号位重叠警告 ======";
        qWarning() << "Message:" << msg.name << "ID: 0x" << QString::number(msg.id, 16);
        qWarning() << "检测到以下信号位重叠:";
        
        for (const QString& overlap : overlaps) {
            qWarning() << "  -" << overlap;
        }
        
        qWarning() << "============================";
    } else {
        qDebug() << "Message" << msg.name << ": 信号位布局正常，无重叠";
    }
}

bool DbcParser::validateSignalRange(const Signal& sig) {
    bool isValid = true;
    
    // 计算原始值能表示的最大物理值
    quint64 maxRaw = (sig.length == 64) ? ~0ULL : ((1ULL << sig.length) - 1);
    double maxRepresentable = maxRaw * sig.factor + sig.offset;
    double minRepresentable = sig.offset;
    
    // 检查 DBC 定义的物理值范围是否超出原始值能表示的范围
    if (sig.max > maxRepresentable) {
        qWarning() << "警告: 信号" << sig.name 
                   << "的 max (" << sig.max << ") 超出可表示的最大值 (" << maxRepresentable << ")";
        qWarning() << "  信号长度:" << sig.length << "位, factor:" << sig.factor << ", offset:" << sig.offset;
        isValid = false;
    }
    
    if (sig.min < minRepresentable) {
        qWarning() << "警告: 信号" << sig.name 
                   << "的 min (" << sig.min << ") 小于可表示的最小值 (" << minRepresentable << ")";
        qWarning() << "  信号长度:" << sig.length << "位, factor:" << sig.factor << ", offset:" << sig.offset;
        isValid = false;
    }
    
    // 检查 factor 是否为 0
    if (sig.factor == 0) {
        qWarning() << "错误: 信号" << sig.name << "的 factor 为 0，这将导致除零错误！";
        isValid = false;
    }
    
    // 检查 min 是否大于 max
    if (sig.min > sig.max) {
        qWarning() << "错误: 信号" << sig.name << "的 min (" << sig.min << ") 大于 max (" << sig.max << ")";
        isValid = false;
    }
    
    return isValid;
}

void DbcParser::validateMessageRanges(const Message& msg) {
    qDebug() << "====== 验证信号范围定义 ======";
    qDebug() << "Message:" << msg.name << "ID: 0x" << QString::number(msg.id, 16);
    
    bool allValid = true;
    for (const Signal& sig : msg.signalList) {
        if (!validateSignalRange(sig)) {
            allValid = false;
        }
    }
    
    if (allValid) {
        qDebug() << "所有信号范围定义合理";
    }
    
    qDebug() << "==============================";
}
