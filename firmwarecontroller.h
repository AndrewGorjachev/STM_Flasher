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

    const char connectCommand  = {0x7F};
    const char pidCommand  [2] = {static_cast<char>(0x02), static_cast<char>(0xFD)};
    const char readCommand [2] = {static_cast<char>(0x11), static_cast<char>(0xEE)};
    const char writeCommand[2] = {static_cast<char>(0x31), static_cast<char>(0xCE)};
    const char clearCommand[2] = {static_cast<char>(0x44), static_cast<char>(0xBB)};
    const char clearComConf[3] = {static_cast<char>(0xFF), static_cast<char>(0xFF), static_cast<char>(0x00)};

    bool readIdFlg = false;

    QStringList * firmwareBuffer = nullptr;

    QSerialPort * serialPort = nullptr;

    QString * dataPath = nullptr;

    QTimer * checkConnectTimer = nullptr;

    QString connectionStatus;

    bool checkAck(int timeout);

public:

    explicit FirmwareController(QObject *parent = nullptr);

    ~FirmwareController();

    Q_INVOKABLE QStringList getAllSerialPorts();

    Q_INVOKABLE int openPort(const QString &portName);

    Q_INVOKABLE void closePort();

    Q_INVOKABLE void readFirmwareFile(const QString &pathToFile);

    Q_INVOKABLE void backUpFirmware(const QString &pathToFile);

    Q_INVOKABLE void flashFirmware();

    Q_INVOKABLE void clearMCUFlash();

    void readMCUID();

    QString getConnectionStatus() const;

    void setConnectionStatus(const QString &value);

signals:

    void portClosed();

    void firmwareReadError();

    void mCUMemoryClearError();

    void mCUMemoryClearSucces();

    void firmwareReadSucces();

    void connectionStatusChanged();

public slots:

    void serialPortCallBack();

    void portError(QSerialPort::SerialPortError error);
};

#endif // FIRMWARECONTROLLER_H
