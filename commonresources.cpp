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
