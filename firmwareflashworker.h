#ifndef FIRMWAREFLASHWORKER_H
#define FIRMWAREFLASHWORKER_H

#include <QObject>
#include <QtDebug>
#include <QString>
#include <QRunnable>
#include <QSerialPort>

class FirmwareFlashWorker : public QObject, public QRunnable
{
    Q_OBJECT

private:

    QString * portName           = nullptr;

    QStringList * firmwareBuffer = nullptr;

    QSerialPort * serialPort     = nullptr;

    const char writeCommand[2]   = {static_cast<char>(0x31), static_cast<char>(0xCE)};

    void errorHandler();

    bool checkAck(int timeout);


public:
    explicit FirmwareFlashWorker(QObject *parent = nullptr);

    explicit FirmwareFlashWorker(const QString & port, const QStringList & firmwareBuffer);

    ~FirmwareFlashWorker();

    void run() override;

signals:

};

#endif // FIRMWAREFLASHWORKER_H
