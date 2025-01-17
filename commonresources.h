#ifndef COMMONRESOURCES_H
#define COMMONRESOURCES_H

#include <QDebug>
#include <QObject>
#include <QSerialPort>

#define reportError(error)      \
        interrupted = true;     \
        executionStatus = error;\
        break

class CommonResources : public QObject
{
    Q_OBJECT

public:

    explicit CommonResources(QObject *parent = nullptr);

    CommonResources(const QString &portName);

    ~CommonResources();

    virtual void run() = 0;

protected:

    QString     * portName   = nullptr;

    QSerialPort * serialPort = nullptr;

    QString executionStatus  = "w/o error";

    bool interrupted         = false;

    float progress           = 0;

    bool checkAck(int timeout);

    void setProgress(const float & value);

    void closePort();

    void initPort();

    int write(const char *data, qint64 len);

    int write(const QByteArray &data);


signals:

    void progressValue(float value);

    void finished(const QString & status);

public slots:

    void stop();

    void errorHandler(QSerialPort::SerialPortError error);
};

#endif // COMMONRESOURCES_H
