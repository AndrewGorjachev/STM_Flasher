#include "commonresources.h"

CommonResources::CommonResources(QObject *parent) : QObject(parent){}

CommonResources::CommonResources(const QString &portName)
{
    this -> portName = new QString(portName);
}

CommonResources::~CommonResources()
{
    if(portName != nullptr)
    {
        delete portName;
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
    if (serialPort!=nullptr)
    {
        if(serialPort->isOpen())
        {
            serialPort->clear();

            serialPort->close();
        }
        delete serialPort;

        serialPort = nullptr;
    }
}

void CommonResources::stop()
{
    interrupted = true;
}

void CommonResources::errorHandler(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::NoError)
    {
        closePort();
    }
}

bool CommonResources::checkAck(int timeout)
{
    char buff;

    if (serialPort!=nullptr)
    {
        if(serialPort->waitForReadyRead(timeout))
        {

            if (serialPort!=nullptr)
            {
                serialPort->read(&buff, 1);

                if(buff == 0x79)
                {
                    return true;

                }
            }
        }
    }
    return false;
}

void CommonResources::closePort()
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

void CommonResources::initPort()
{
    serialPort = new QSerialPort(*portName);

    serialPort -> setBaudRate(115200);

    serialPort -> setDataBits(QSerialPort::Data8);

    serialPort -> setParity(QSerialPort::EvenParity);

    serialPort -> setStopBits(QSerialPort::OneStop);

    serialPort -> setFlowControl(QSerialPort::NoFlowControl);

    connect(serialPort, &QSerialPort::errorOccurred, this, &CommonResources::errorHandler);
}

int CommonResources::write(const char *data, qint64 len)
{
    if (serialPort == nullptr)
    {
        return -1;

    } else {

        serialPort->clear();
    }
    if(serialPort == nullptr){

        return -1;

    }else {

        serialPort->write(data, len);
    }

    if(serialPort == nullptr)
    {
        return -1;

    } else {

        serialPort->flush();
    }
    return 0;
}

int CommonResources::write(const QByteArray &data)
{
    if (serialPort == nullptr)
    {
        return -1;

    } else {

        serialPort->clear();
    }
    if(serialPort == nullptr)
    {
        return -1;

    } else {

        serialPort->write(data);
    }
    if(serialPort == nullptr)
    {
        return -1;

    } else {

        serialPort->flush();
    }
    return 0;
}
