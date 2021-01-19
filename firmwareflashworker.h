#ifndef FIRMWAREFLASHWORKER_H
#define FIRMWAREFLASHWORKER_H

#include <QObject>
#include <QtDebug>
#include <QString>
#include <QRunnable>
#include <QSerialPort>

#include "commonresources.h"

class FirmwareFlashWorker :  public CommonResources
{

private:

    int globalAddressOffSet      = 0x08000000;

    QByteArray * address         = nullptr;

    QByteArray * payload         = nullptr;

    QStringList * firmwareBuffer = nullptr;

    const char writeCommand[2]   = {static_cast<char>(0x31), static_cast<char>(0xCE)};

public:
    explicit FirmwareFlashWorker(QObject *parent = nullptr);

    explicit FirmwareFlashWorker(const QString & port, const QStringList & pathToBackUp);

    ~FirmwareFlashWorker();

    void run();
};

#endif // FIRMWAREFLASHWORKER_H
