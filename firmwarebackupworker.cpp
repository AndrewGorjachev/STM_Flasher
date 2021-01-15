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
    }
    if(firmwareBuffer != nullptr)
    {
        delete firmwareBuffer;
    }
}

void FirmwareBackUpWorker::run()
{
    qDebug()<< "Thread has started";

    initPort();

    if (serialPort -> open(QIODevice::ReadWrite))
    {
         while(offset < flasSize){

             if(!interrupted){

                 int currentAddress = startAddress + offset;

                 if (offset + readSize > flasSize)
                 {
                     readSize = flasSize - offset;
                 }
                 offset += readSize + 1;

                 QByteArray * address = new QByteArray();

                 address->append((currentAddress >> 24) & 0xFF);
                 address->append((currentAddress >> 16) & 0xff);
                 address->append((currentAddress >> 8)  & 0xff);
                 address->append((currentAddress        & 0xff));

                 address->append(address->at(0)^address->at(1)^address->at(2)^address->at(3));

                 QByteArray * size = new QByteArray();

                 size->append(readSize);
                 size->append(readSize^readSize);

                 serialPort->clear();

                 serialPort->write(readCommand,2);

                 serialPort->flush();

                 if(checkAck(500)){                   // Empirically defined answer delay

                     serialPort->write(*address);

                     serialPort->flush();

                     if(address != nullptr)
                     {
                         delete address;

                         address = nullptr;
                     }
                     if(checkAck(60))                // Empirically defined answer delay
                     {
                         serialPort->write(*size);

                         serialPort->flush();

                         if(size != nullptr)
                         {
                             delete size;

                             size = nullptr;
                         }
                         if(serialPort->waitForReadyRead(10)){

                             int counter = 0;

                             qDebug() << (static_cast<float>(offset)/static_cast<float>(flasSize));

                             emit progressValue(static_cast<float>(offset)/static_cast<float>(flasSize));

                             while(serialPort->bytesAvailable()<(readSize+2))
                             {
                                 serialPort->waitForReadyRead(10);

                                 counter++;

                                 if(counter>500){

                                     executionStatus = "Read payload dead lock";

                                     interrupted = true;

                                     break;
                                 }
                             }
                             char buff;

                             serialPort->read(&buff, 1);

                             if (buff==0x79)
                             {
                                 QByteArray payloadBuffer = serialPort->readAll();

                                 //qDebug() << payloadBuffer.length();

                                 qDebug() << payloadBuffer.toHex();

                             } else {

                                 interrupted = true;

                                 executionStatus = "Read payload error";

                                 break;
                             }
                         } else{

                             interrupted = true;

                             executionStatus = "Write size error";

                             break;
                         }
                     } else {

                         if(size!=nullptr)
                         {
                             delete size;

                             size = nullptr;
                         }

                         interrupted = true;

                         executionStatus = "Write address error";

                         break;
                     }
                 } else {

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

                     interrupted = true;

                     executionStatus = "Write command error";

                     break;
                 }
             }
         }
    } else {

        interrupted = true;

        executionStatus = "error";
    }


    if (!interrupted){

        /* here will be writing firmware back up into file*/
    }
    closePort();

    emit finished(executionStatus);
}
