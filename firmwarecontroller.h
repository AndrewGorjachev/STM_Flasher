#ifndef FIRMWARECONTROLLER_H
#define FIRMWARECONTROLLER_H

#include <QDebug>
#include <QString>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

class FirmwareController : public QObject
{
    Q_OBJECT

private:

    QSerialPort * serialPort = nullptr;

    bool checkAck();

public:

    explicit FirmwareController(QObject *parent = nullptr);

    ~FirmwareController();

    Q_INVOKABLE QStringList getAllSerialPorts();

    Q_INVOKABLE int openPort(const QString &portName);

    Q_INVOKABLE void closePort();

    Q_INVOKABLE void readFirmwareFile(const QString &pathToFile);

signals:

public slots:

    void incomeDataProcessing();

    void portError(QSerialPort::SerialPortError error);
};

#endif // FIRMWARECONTROLLER_H
