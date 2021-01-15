#ifndef FIRMWAREBACKUPWORKER_H
#define FIRMWAREBACKUPWORKER_H

#include <QObject>
#include <QtDebug>
#include <QString>
#include <QRunnable>
#include <QSerialPort>

#include "commonresources.h"

class FirmwareBackUpWorker : public CommonResources
{

public:

    explicit FirmwareBackUpWorker(const QString & portName, const QString & pathToBackUp);

    ~FirmwareBackUpWorker();

    void run() override;

private:

    const char readCommand [2]   = {static_cast<char>(0x11), static_cast<char>(0xEE)};

    QStringList * firmwareBuffer = nullptr;

    QString * pathToBackUp       = nullptr;

    int startAddress             = 0x08000000;  // Firmware space flash memory start address

    int flasSize                 = 512 * 1024;  // Flash memory size is 512 kBytes for STM32G474RET6

    int readSize                 = 255;         // Quantum of reading in kBytes

    int offset                   = 0;           //
};

#endif // FIRMWAREBACKUPWORKER_H
