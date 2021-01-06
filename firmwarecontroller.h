#ifndef FIRMWARECONTROLLER_H
#define FIRMWARECONTROLLER_H

#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QThread>
#include <QString>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

class FirmwareController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString connectionStatus READ getConnectionStatus WRITE setConnectionStatus NOTIFY connectionStatusChanged)

private:

    const char pidCommand[2] = {static_cast<char>(0x02), static_cast<char>(0xFD)};

    bool readIdFlg = false;



    QSerialPort * serialPort = nullptr;

    QString * dataPath = nullptr;

    QTimer * checkConnectTimer = nullptr;

    QString connectionStatus;

    bool checkAck();

public:

    explicit FirmwareController(QObject *parent = nullptr);

    ~FirmwareController();

    Q_INVOKABLE QStringList getAllSerialPorts();

    Q_INVOKABLE int openPort(const QString &portName);

    Q_INVOKABLE void closePort();

    Q_INVOKABLE void readFirmwareFile(const QString &pathToFile);

    Q_INVOKABLE void backUpFirmware(const QString &pathToFile);

    Q_INVOKABLE void flashFirmware();

    void readMCUID();

    QString getConnectionStatus() const;

    void setConnectionStatus(const QString &value);

signals:

    void portClosed();

    void connectionStatusChanged();

public slots:

    void serialPortCallBack();

    void portError(QSerialPort::SerialPortError error);
};

#endif // FIRMWARECONTROLLER_H
