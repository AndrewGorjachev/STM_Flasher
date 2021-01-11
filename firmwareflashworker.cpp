#include "firmwareflashworker.h"

FirmwareFlashWorker::FirmwareFlashWorker(QObject *parent) : QObject(parent){}

FirmwareFlashWorker::FirmwareFlashWorker(const QString & port, const QStringList & firmwareBuffer)
{
    qDebug() << "Worker has created";

    this -> portName = new QString(port);

    this -> firmwareBuffer = new QStringList(firmwareBuffer);
}

FirmwareFlashWorker::~FirmwareFlashWorker()
{
    delete portName;

    delete firmwareBuffer;
}

void FirmwareFlashWorker::run()
{
    qDebug() << "Worker has started";

    serialPort = new QSerialPort(*portName);

    serialPort -> setBaudRate(115200);

    serialPort -> setDataBits(QSerialPort::Data8);

    serialPort -> setParity(QSerialPort::EvenParity);

    serialPort -> setStopBits(QSerialPort::OneStop);

    serialPort -> setFlowControl(QSerialPort::NoFlowControl);

    connect(serialPort, &QSerialPort::errorOccurred, this, &FirmwareFlashWorker::errorHandler);

    if (serialPort -> open(QIODevice::ReadWrite)) {

        qDebug()<< "port has reopen";

        for(int i = 0; i < firmwareBuffer->length(); i++) {

            //QThread::yieldCurrentThread();

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

            //qDebug()<< address->toHex();

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

            //qDebug() << payload->toHex();

            serialPort->write(writeCommand, 2);

            serialPort->flush();

            if(checkAck(10)){

                serialPort->write(*address);

                serialPort->flush();

                if(checkAck(50)){

                    serialPort->write(*payload);

                    serialPort->flush();

                    if(checkAck(500)){

                        qDebug() << static_cast<float>(i)/static_cast<float>(firmwareBuffer->length());

                        //setProgress(static_cast<float>(i)/static_cast<float>(firmwareBuffer->length()));

                    } else {

                        qDebug()<<"write data error";

                        break;
                    }
                } else {

                    qDebug()<<"write adress error";

                    break;
                }
            } else {

                qDebug()<<"write command error";

                break;
            }


            delete payload;

        }
        qDebug() << "firmware has flash";
    }
}

void FirmwareFlashWorker::errorHandler()
{

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
