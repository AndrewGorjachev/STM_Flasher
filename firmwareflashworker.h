#ifndef FIRMWAREFLASHWORKER_H
#define FIRMWAREFLASHWORKER_H

#include <QObject>
#include <QtDebug>
#include <QString>
#include <QRunnable>
#include <QSerialPort>

class FirmwareFlashWorker : public QObject
{
    Q_OBJECT

private:

    bool interrupted = false;

    QString     * portName       = nullptr;

    QStringList * firmwareBuffer = nullptr;

    QSerialPort * serialPort     = nullptr;

    QString status               = "w/o error";

    const char writeCommand[2]   = {static_cast<char>(0x31), static_cast<char>(0xCE)};

    bool checkAck(int timeout);

    void closePort();

public:
    explicit FirmwareFlashWorker(QObject *parent = nullptr);

    explicit FirmwareFlashWorker(const QString & port, const QStringList & pathToBackUp);

    ~FirmwareFlashWorker();

    void run();

signals:

    void progressValue(float value);

    void finished(const QString & status);

    void errorHappen(QSerialPort::SerialPortError error);

public slots:

    void stop();

    void errorHandler(QSerialPort::SerialPortError error);
};

#endif // FIRMWAREFLASHWORKER_H
