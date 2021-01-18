#include "firmwarebackupworker.h"

FirmwareBackUpWorker::FirmwareBackUpWorker(const QString &portName, const QString &pathToBackUp) : CommonResources(portName)
{
    this -> pathToBackUp   = new QString(pathToBackUp);

    this -> firmwareBuffer = new QStringList();
}

FirmwareBackUpWorker::~FirmwareBackUpWorker()
{
    if(pathToBackUp != nullptr)
    {
        delete pathToBackUp;

        pathToBackUp = nullptr;
    }
    if(firmwareBuffer != nullptr)
    {
        delete firmwareBuffer;

        firmwareBuffer = nullptr;
    }
    if(size!=nullptr)
    {
        delete size;

        size = nullptr;
    }
    if(address!=nullptr)
    {
        delete address;

        address = nullptr;
    }
}

void FirmwareBackUpWorker::run()
{
    initPort();

    if (serialPort -> open(QIODevice::ReadWrite))
    {
         while(offset < flasSize)
         {
             if(!interrupted)
             {
                 int currentAddress = startAddress + offset;

                 if (offset + readSize > flasSize)
                 {
                     readSize = flasSize - offset;
                 }
                 offset += readSize + 1;

                 address = new QByteArray();

                 address->append((currentAddress >> 24) & 0xFF);
                 address->append((currentAddress >> 16) & 0xff);
                 address->append((currentAddress >> 8)  & 0xff);
                 address->append((currentAddress        & 0xff));

                 address->append(address->at(0)^address->at(1)^address->at(2)^address->at(3));

                 size = new QByteArray();

                 size->append(readSize);

                 size->append(readSize^readSize);

                 if(write(readCommand,2)==0)
                 {
                     if(checkAck(500))                       // Empirically defined answer delay
                     {
                         if(write(*address)==0)
                         {
                             if(address != nullptr)
                             {
                                 delete address;

                                 address = nullptr;
                             }
                             if(checkAck(60))                // Empirically defined answer delay
                             {
                                 if(write(*size)==0)
                                 {
                                     if(size != nullptr)
                                     {
                                         delete size;

                                         size = nullptr;
                                     }
                                     if(serialPort->waitForReadyRead(20))
                                     {
                                         emit progressValue(static_cast<float>(offset)/static_cast<float>(flasSize));

                                         bool continueWaiting = true;

                                         int counter = 0;

                                         while(continueWaiting)
                                         {
                                             if (serialPort!=nullptr)
                                             {
                                                 if(serialPort->bytesAvailable()<(readSize+2))
                                                 {
                                                     continueWaiting = true;

                                                     counter++;

                                                     if (serialPort!=nullptr)
                                                     {
                                                         serialPort->waitForReadyRead(10);

                                                     } else {

                                                         reportError("Read payload dead lock");
                                                     }
                                                 } else {

                                                     continueWaiting = false;
                                                 }
                                                 if(counter>500){

                                                     reportError("Read payload dead lock");
                                                 }
                                             } else {

                                                 reportError("Read payload error");
                                             }
                                         }
                                         char buff;

                                         if(serialPort!=nullptr)
                                         {
                                             serialPort->read(&buff, 1);

                                             if (buff == 0x79)
                                             {
                                                 if(serialPort!=nullptr)
                                                 {
                                                     QByteArray payloadBuffer = serialPort->readAll();

                                                     firmwareEntityCreator(currentAddress, payloadBuffer);

                                                 } else {

                                                     reportError("Read payload error");
                                                 }
                                             } else {

                                                 reportError("Read payload error");
                                             }
                                         } else {

                                             reportError("Read payload error");
                                         }
                                     } else {

                                         reportError("Write size error");
                                     }
                                 } else {

                                     reportError("Write size error");
                                 }
                             } else {

                                 reportError("Write address error");
                             }
                         } else {

                             reportError("Write address error");
                         }
                     } else {

                         reportError("Write command error");
                     }
                 } else {

                     reportError("Write command error");
                 }
             }
         }
    } else {

        interrupted = true;

        executionStatus = "error";
    }
    if (!interrupted){

        if(writeToFile() != 0)
        {
            executionStatus = "Write to file error";
        }
    }
    closePort();

    emit finished(executionStatus);
}

void FirmwareBackUpWorker::firmwareEntityCreator(int currentAddress, const QByteArray &payloadBuffer)
{
    if ((((currentAddress >> 16) & 0xFFF) > 0x800) && ((currentAddress & 0xFFFF) == 0))
    {
        uint16_t globalAddress = ((currentAddress >> 16) & 0xFFF);

        uint8_t ceckSum = 0x06;

        ceckSum += globalAddress & 0xFF;

        ceckSum += (globalAddress>>8) & 0xFF;

        ceckSum = 0x01 + ~ceckSum;

        QString * entity = new QString(":02000004"+
                                       QString("%1").arg(globalAddress, 4, 16, QLatin1Char('0'))+
                                       QString("%1").arg(ceckSum, 2, 16, QLatin1Char('0')));
        firmwareBuffer->append(*entity);

        delete entity;
    }
    for(int i = 0; i < 0x100; i = i+0x10)
    {
        uint8_t calculatedChecksum  = 0x10;

        uint16_t stringAddress = (currentAddress+i) & 0xFFFF;

        calculatedChecksum += stringAddress & 0xFF;

        calculatedChecksum += (stringAddress>>8) & 0xFF;

        QByteArray * localBuffer = new QByteArray();

        for (int j = i; j<i+0x10; j++ )
        {
            localBuffer->append(payloadBuffer.at(j));

            calculatedChecksum += payloadBuffer.at(j);
        }
        calculatedChecksum = 0x01 + ~calculatedChecksum;

        QString * entity = new QString(":10"+
                                       QString("%1").arg(stringAddress, 4, 16, QLatin1Char('0'))+
                                       "00"+
                                       localBuffer->toHex()+
                                       QString("%1").arg(calculatedChecksum, 2, 16, QLatin1Char('0')));

        firmwareBuffer->append(*entity);

        delete entity;

        delete localBuffer;
    }
}

int FirmwareBackUpWorker::writeToFile()
{
    QString * fileName = nullptr;

    if(QSysInfo::productType()=="windows")
    {
        fileName = new QString();
        fileName->append(pathToBackUp->remove("file:///"));
        fileName->append("/BackUP_");
        fileName->append(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));
        fileName->append(".hex");

    } else {
        fileName = new QString();
        fileName->append(pathToBackUp->remove("file:///"));
        fileName->append("\\BackUP_");
        fileName->append(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));
        fileName->append(".hex");
    }
    if (QFile::exists(*fileName))
    {
        QFile::remove(*fileName);
    }
    QFile backUp(*fileName);

    if (backUp.open(QFile::WriteOnly | QFile::Text))
    {
        QTextStream stream(&backUp);

        stream << ":020000040800F2" << '\n';

        if (firmwareBuffer!=nullptr){

            foreach (QString string, *firmwareBuffer){

                stream << string << '\n';
            }
        }
        stream << ":00000001FF" << '\n';
    } else {

        return 1;
    }
    backUp.close();

    delete fileName;

    return 0;
}
