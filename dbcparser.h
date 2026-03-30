#ifndef DBCPARSER_H
#define DBCPARSER_H


#include <QString>
#include <QList>
#include <QMetaType>

struct Signal {
    QString name;
    int startBit;
    int length;
    bool isIntel;       // true: Intel (小端), false: Motorola (大端)
    bool isSigned;      // '+' 表示无符号，'-' 表示有符号
    double factor;
    double offset;
    double min;
    double max;
    QString unit;
};

struct Message {
    uint32_t id;
    QString name;
    uint8_t dlc;
    QList<Signal> signalList;
};

class DbcParser {
public:
    static bool parseFile(const QString& filePath, QList<Message>& messages);
    
    // 信号重叠检测
    static bool checkSignalOverlap(const Signal& sig1, const Signal& sig2);
    static QStringList findOverlappingSignals(const Message& msg);
    static void validateMessageSignals(const Message& msg);
    
    // 信号范围验证
    static bool validateSignalRange(const Signal& sig);
    static void validateMessageRanges(const Message& msg);
};

#endif // DBCPARSER_H
