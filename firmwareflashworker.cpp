#include "firmwareflashworker.h"

FirmwareFlashWorker::FirmwareFlashWorker(QObject *parent) : QObject(parent){}

FirmwareFlashWorker::FirmwareFlashWorker(const QString & port, const QStringList & firmwareBuffer)
{
    this -> portName = new QString(port);

    this -> firmwareBuffer = new QStringList(firmwareBuffer);
}

FirmwareFlashWorker::~FirmwareFlashWorker()
{
    if(portName != nullptr)
    {
        delete portName;
    }
    if(firmwareBuffer != nullptr)
    {
        delete firmwareBuffer;
    }
    if (serialPort != nullptr)
    {
        if(serialPort->isOpen())
        {
            serialPort->clear();

            serialPort->close();
        }
        delete serialPort;
    }
}

void FirmwareFlashWorker::run()
{
    serialPort = new QSerialPort(*portName);

    serialPort -> setBaudRate(115200);

    serialPort -> setDataBits(QSerialPort::Data8);

    serialPort -> setParity(QSerialPort::EvenParity);

    serialPort -> setStopBits(QSerialPort::OneStop);

    serialPort -> setFlowControl(QSerialPort::NoFlowControl);

    connect(serialPort, &QSerialPort::errorOccurred, this, &FirmwareFlashWorker::errorHandler);

    if (serialPort -> open(QIODevice::ReadWrite))
    {
        for(int i = 0; i < firmwareBuffer->length(); i++)
        {
            if(!interrupted)
            {
                const QString * s = &(firmwareBuffer->at(i));

                int payloadLenght = s->mid(1,2).toInt(nullptr, 16);

                int addressOffSet = s->mid(3,4).toInt(nullptr, 16);

                addressOffSet += 0x08000000;

                QByteArray * address = new QByteArray();

                address->append((addressOffSet >> 24) & 0xFF);
                address->append((addressOffSet >> 16) & 0xff);
                address->append((addressOffSet >> 8) & 0xff);
                address->append((addressOffSet & 0xff));

                address->append(address->at(0)^address->at(1)^address->at(2)^address->at(3));

                QByteArray * payload = new QByteArray();

                payload->append(payloadLenght - 1);

                uint8_t crcAcc = 0 ^ payload->at(0);

                for (int i = 0; i<payloadLenght; i++){

                    uint8_t buff = s->mid(9+(i*2),2).toInt(nullptr, 16);

                    crcAcc ^= buff;

                    crcAcc &= 0xFF;

                    payload->append(buff);
                }

                payload->append(crcAcc);

                serialPort->write(writeCommand, 2);

                serialPort->flush();

                if(checkAck(15)){                                              // Empirically defined answer delay

                    serialPort->write(*address);

                    serialPort->flush();

                    if(address != nullptr)
                    {
                        delete address;

                        address = nullptr;
                    }

                    if(checkAck(60)){                                          // Empirically defined answer delay

                        serialPort->write(*payload);

                        serialPort->flush();

                        if(payload!=nullptr)
                        {
                            delete payload;

                            payload = nullptr;
                        }
                        if(checkAck(750)){                                     // Empirically defined answer delay

                            emit progressValue(static_cast<float>(i)/static_cast<float>(firmwareBuffer->length()));

                        } else {

                            status = "Write payload error";

                            break;
                        }
                    } else {

                        status = "Write address error";

                        break;
                    }
                } else {

                    if(payload!=nullptr)
                    {
                        delete payload;

                        payload = nullptr;
                    }
                    if(address!=nullptr)
                    {
                        delete address;

                        address = nullptr;
                    }
                    status = "Write command error";

                    break;
                }
            }
        }
    } else {

        status = "error";
    }
    closePort();

    emit finished(status);
}

void FirmwareFlashWorker::stop()
{
    interrupted = true;
}

void FirmwareFlashWorker::errorHandler(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::NoError)
    {
        closePort();
    }
}

bool FirmwareFlashWorker::checkAck(int timeout)
{
    char buff;

    if(serialPort->waitForReadyRead(timeout))
    {
        serialPort->read(&buff, 1);

        if(buff == 0x79)
        {
            return true;

        } else {

            return false;
        }
    } else {

        return false;
    }
}

void FirmwareFlashWorker::closePort()
{
    if (serialPort!=nullptr){

        if(serialPort->isOpen()){

            serialPort->clear();

            serialPort->close();
        }
        delete serialPort;

        serialPort = nullptr;
    }
}
