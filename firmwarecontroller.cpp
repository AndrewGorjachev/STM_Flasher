#include "firmwarecontroller.h"

FirmwareController::FirmwareController(QObject *parent) : QObject(parent)
{
    connectionStatus = "Not Connected";

    checkConnectTimer = new QTimer();

    checkConnectTimer->setInterval(1000);

    connect(checkConnectTimer, &QTimer::timeout, this, &FirmwareController::readMCUID);
}

FirmwareController::~FirmwareController()
{
    checkConnectTimer->stop();

    delete checkConnectTimer;

    if (serialPort!=nullptr)
    {
        serialPort->close();

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

    serialPort -> setParity(QSerialPort::EvenParity);

    serialPort -> setStopBits(QSerialPort::OneStop);

    serialPort -> setFlowControl(QSerialPort::NoFlowControl);

    connect(serialPort, &QSerialPort::errorOccurred, this, &FirmwareController::portError);

    if (serialPort -> open(QIODevice::ReadWrite)) {

        char buff(0x7F);  // Connect command

        serialPort->write(&buff, 1);

        serialPort->flush();

        if (checkAck()){

            setConnectionStatus("Connected");

            connect(serialPort, &QSerialPort::readyRead, this, &FirmwareController::serialPortCallBack);

            checkConnectTimer->start();

            return 0;

        } else {

            setConnectionStatus("Connection Error");

            closePort();

            return 1;
        }
    } else {

        setConnectionStatus("Port Open Error");

        return 1;
    }
}

bool FirmwareController::checkAck()
{
    QByteArray buff;

    if(serialPort->waitForReadyRead(1)){

        buff = serialPort->read(1);

        if(buff.at(0) == 0x79){

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
    emit portClosed();

    setConnectionStatus("Not Connected");

    checkConnectTimer->stop();

    serialPort->close();

    delete serialPort;

    serialPort = nullptr;
}

void FirmwareController::portError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::NoError){

        qDebug()<<error;
    }
}

void FirmwareController::readFirmwareFile(const QString &pathToFile)
{
    dataPath = new QString(pathToFile);

    if(QSysInfo::productType()=="windows")
    {
        dataPath -> remove("file:///");

    } else {

        dataPath -> remove("file://");
    }
}

void FirmwareController::backUpFirmware(const QString &pathToFile)
{
    qDebug()<< pathToFile;
}

void FirmwareController::flashFirmware()
{
    qDebug()<< *dataPath;
}

void FirmwareController::readMCUID()
{
    serialPort->clear();

    readIdFlg = true;

    serialPort->write(pidCommand, 2);

    serialPort->flush();
}

QString FirmwareController::getConnectionStatus() const
{
    return connectionStatus;
}

void FirmwareController::setConnectionStatus(const QString &value)
{
    if(value != connectionStatus)
    {
        connectionStatus = value;

        emit connectionStatusChanged();
    }
}

void FirmwareController::serialPortCallBack()
{
    if (readIdFlg){

        if (serialPort->bytesAvailable() < 5)
        {
            return;

        } else {

            QByteArray buff;

            buff = serialPort->read(5);

            if((buff.at(0) == 'y')  &&
               (buff.at(1) == 0x01) &&
               (buff.at(2) == 0x04) &&
               (buff.at(3) == 'i')  &&
               (buff.at(4) == 'y'))
            {
                readIdFlg = false;

            } else {

                readIdFlg = false;

                closePort();
            }
        }
    }


}
