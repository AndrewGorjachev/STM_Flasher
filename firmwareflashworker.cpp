#include "firmwareflashworker.h"

FirmwareFlashWorker::FirmwareFlashWorker(const QString & port, const QStringList & firmwareBuffer) : CommonResources(port)
{
    this -> firmwareBuffer = new QStringList(firmwareBuffer);
}

FirmwareFlashWorker::~FirmwareFlashWorker()
{
    if(firmwareBuffer != nullptr)
    {
        delete firmwareBuffer;
    }
    if(payload != nullptr)
    {
        delete payload;

        payload = nullptr;
    }
    if(address != nullptr)
    {
        delete address;

        address = nullptr;
    }
}

void FirmwareFlashWorker::run()
{
    initPort();

    if (serialPort -> open(QIODevice::ReadWrite))
    {
        for(int i = 0; i < firmwareBuffer->length(); i++)
        {
            if(!interrupted)
            {
                const QString * s = &(firmwareBuffer->at(i));

                if (s->startsWith(":02000004"))
                {
                    globalAddressOffSet = s->mid(9,4).toInt(nullptr, 16);

                    globalAddressOffSet = (globalAddressOffSet << 16);

                    qDebug() << hex << globalAddressOffSet;

                } else {

                    int payloadLenght = s->mid(1,2).toInt(nullptr, 16);

                    int addressOffSet = s->mid(3,4).toInt(nullptr, 16);

                    addressOffSet += globalAddressOffSet;

                    qDebug() << hex << addressOffSet;

                    address = new QByteArray();

                    address->append((addressOffSet >> 24) & 0xFF);
                    address->append((addressOffSet >> 16) & 0xff);
                    address->append((addressOffSet >> 8) & 0xff);
                    address->append((addressOffSet & 0xff));

                    address->append(address->at(0)^address->at(1)^address->at(2)^address->at(3));

                    payload = new QByteArray();

                    payload->append(payloadLenght - 1);

                    uint8_t crcAcc = 0 ^ payload->at(0);

                    for (int i = 0; i<payloadLenght; i++){

                        uint8_t buff = s->mid(9+(i*2),2).toInt(nullptr, 16);

                        crcAcc ^= buff;

                        crcAcc &= 0xFF;

                        payload->append(buff);
                    }
                    payload->append(crcAcc);


                    if(write(writeCommand, 2) == 0)
                    {
                        if(checkAck(15))
                        {                                              // Empirically defined answer delay
                            if(write(*address) == 0)
                            {
                                if(address != nullptr)
                                {
                                    delete address;

                                    address = nullptr;
                                }
                                if(checkAck(60))
                                {                                          // Empirically defined answer delay
                                    if(write(*payload) == 0)
                                    {
                                        if(payload!=nullptr)
                                        {
                                            delete payload;

                                            payload = nullptr;
                                        }
                                        if(checkAck(750))                  // Empirically defined answer delay
                                        {
                                            emit progressValue(static_cast<float>(i)/static_cast<float>(firmwareBuffer->length()));

                                        } else {

                                            reportError("Write payload error");
                                        }
                                    } else {

                                        reportError("Write payload error");
                                    }
                                } else {

                                    reportError("Write address error");
                                }
                            } else {

                                reportError("Write address error");
                            }
                        } else {

                            reportError("Write command error 1");
                        }
                    } else {

                        reportError("Write command error 2");
                    }
                }
            }
        }
    } else {

        executionStatus = "error";
    }
    closePort();

    emit finished(executionStatus);
}
