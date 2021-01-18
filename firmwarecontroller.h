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
#include <QEventLoop>
#include <firmwareflashworker.h>
#include <firmwarebackupworker.h>

class FirmwareController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString connectionStatus READ getConnectionStatus WRITE setConnectionStatus NOTIFY connectionStatusChanged)

    Q_PROPERTY(float progress READ getProgress WRITE setProgress NOTIFY progressChanged)

private:

    const char connectCommand  = {0x7F};
    const char pidCommand  [2] = {static_cast<char>(0x02), static_cast<char>(0xFD)};
    const char readCommand [2] = {static_cast<char>(0x11), static_cast<char>(0xEE)};
    const char clearCommand[2] = {static_cast<char>(0x44), static_cast<char>(0xBB)};
    const char clearComConf[3] = {static_cast<char>(0xFF), static_cast<char>(0xFF), static_cast<char>(0x00)};

    float progress             = 0;

    FirmwareFlashWorker * flashWorker   = nullptr;

    FirmwareBackUpWorker * backUpWorker = nullptr;

    QTimer * checkConnectTimer          = nullptr;

    QStringList * firmwareBuffer        = nullptr;

    QSerialPort * serialPort            = nullptr;

    QThread * thread                    = nullptr;

    QString * dataPath                  = nullptr;

    QString connectionStatus;

    bool checkAck(int timeout);

    void setProgress(const float & value);

public:

    explicit FirmwareController(QObject *parent = nullptr);

    ~FirmwareController();

    Q_INVOKABLE QStringList getAllSerialPorts();

    Q_INVOKABLE int openPort(const QString &portName);

    Q_INVOKABLE void closePort(const QString & closeStatus);

    Q_INVOKABLE void readFirmwareFile(const QString &pathToFile);

    Q_INVOKABLE void backUpFirmware(const QString &pathToFile);

    Q_INVOKABLE void clearMCUFlash();

    Q_INVOKABLE bool isPosibleToFlash();

    QString getConnectionStatus() const;

    float getProgress() const;

    void readMCUID();

    void setConnectionStatus(const QString &value);

signals:

    void portClosed();

    void firmwareReadError();

    void mCUMemoryClearError();

    void mCUMemoryClearSucces();

    void firmwareFlashError();

    void firmwareFlashSucces();

    void firmwareBackUpSucces();

    void firmwareBackUpError();

    void firmwareReadSucces();

    void connectionStatusChanged();

    void progressChanged();

public slots:

    void flashFirmware();

    void flashFirmwareHasFinished(const QString & errorStatus);

    void backUpFirmwareHasFinished(const QString & errorStatus);

    void portError(QSerialPort::SerialPortError error);
};

#endif // FIRMWARECONTROLLER_H
