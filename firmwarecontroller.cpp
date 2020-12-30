#include "firmwarecontroller.h"


FirmwareController::FirmwareController(QObject *parent) : QObject(parent)
{

}

FirmwareController::~FirmwareController()
{
    if (serialPort!=nullptr)
    {
        delete serialPort;
    }
}

QStringList FirmwareController::getAllSerialPorts()
{
    qDebug()<< "Getting all serial ports";

    QStringList qStringList;

    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
    {
        qStringList.append(port.portName());
    }
    return qStringList;
}

int FirmwareController::openPort(const QString &portName)
{
    qDebug()<< "Open port " + portName;

    serialPort = new QSerialPort(portName);

    serialPort -> setBaudRate(115200);

    serialPort -> setDataBits(QSerialPort::Data8);

    serialPort -> setParity(QSerialPort::NoParity);

    serialPort -> setStopBits(QSerialPort::OneStop);

    serialPort -> setFlowControl(QSerialPort::NoFlowControl);


    connect(serialPort, &QSerialPort::errorOccurred, this, &FirmwareController::portError);

    if (serialPort -> open(QIODevice::ReadWrite)) {

        char buff(0x7F);

        serialPort->write(&buff, 1);

        serialPort->waitForBytesWritten(100);

        if (checkAck()){

            connect(serialPort, &QSerialPort::readyRead, this, &FirmwareController::incomeDataProcessing);

            return 0;

        } else {

            closePort();

            return 1;

            qDebug() << "Open port error!!!!!!!!!";

        }
    } else {

        return 1;

        qDebug() << "Open port error!";
    }
}

bool FirmwareController::checkAck()
{
    char buff;

    if(serialPort->waitForReadyRead(50)){

        serialPort->read(&buff, 1);

        qDebug()<< buff;

        if(buff == 0x79){

            return true;

        } else{

            return false;
        }
    } else {
        qDebug()<< "Serial port timeout";

        return false;
    }
}






void FirmwareController::closePort()
{
    serialPort->close();

    delete serialPort;

    serialPort = nullptr;
}

void FirmwareController::incomeDataProcessing()
{
    const QByteArray incomingData = serialPort->readAll();

    qDebug()<<incomingData;
}

void FirmwareController::portError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::NoError){

        qDebug()<<error;
    }
}

void FirmwareController::readFirmwareFile(const QString &pathToFile)
{

}
