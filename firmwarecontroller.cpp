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
    if (firmwareBuffer!=nullptr)
    {
        delete firmwareBuffer;
    }
}

QStringList FirmwareController::getAllSerialPorts()
{
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

        serialPort->write(&connectCommand, 1);

        serialPort->flush();

        if (checkAck(1)){      // Empirically defined answer delay

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

bool FirmwareController::checkAck(int timeout)
{
    QByteArray buff;

    if(serialPort->waitForReadyRead(timeout)){

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
    QFile firmwareFile(*dataPath);

    if(firmwareFile.open(QIODevice::ReadOnly)){

        QTextStream firmwareReadStream(&firmwareFile);

        if (firmwareBuffer!=nullptr){

            delete firmwareBuffer;
        }
        firmwareBuffer = new QStringList();

        while (!firmwareReadStream.atEnd()) {

            QString buff = firmwareReadStream.readLine();

            if(buff.startsWith(":")){

                firmwareBuffer->append(buff);
            } else  {

                delete firmwareBuffer;

                firmwareBuffer = nullptr;

                emit firmwareReadError();

                return;
            }
        }

        if((firmwareBuffer->at(0)==":020000040800F2")&&
           (firmwareBuffer->at(firmwareBuffer->length()-1)==":00000001FF"))
        {
            firmwareBuffer->removeAt(0);

            firmwareBuffer->removeAt(firmwareBuffer->length()-2);

            firmwareBuffer->removeAt(firmwareBuffer->length()-1);

            emit firmwareReadSucces();
        } else {

            delete firmwareBuffer;

            firmwareBuffer = nullptr;

            emit firmwareReadError();
        }

    } else {

        emit firmwareReadError();
    }
}

void FirmwareController::backUpFirmware(const QString &pathToFile)
{
    qDebug()<< pathToFile;
}

void FirmwareController::flashFirmware()
{
    clearMCUFlash();

    checkConnectTimer->stop();

    foreach(QString s, *firmwareBuffer){

        int payloadLenght = s.mid(1,2).toInt(nullptr, 16);

        int addressOffSet = s.mid(3,4).toInt(nullptr, 16);

        addressOffSet += 0x08000000;

        QByteArray * address = new QByteArray();

        address->append((addressOffSet >> 24) & 0xFF);
        address->append((addressOffSet >> 16) & 0xff);
        address->append((addressOffSet >> 8) & 0xff);
        address->append((addressOffSet & 0xff));

        address->append(address->at(0)^address->at(1)^address->at(2)^address->at(3));

        qDebug()<< address->toHex();

        QByteArray * payload = new QByteArray();

        payload->append(payloadLenght - 1);

        uint8_t crcAcc = 0 ^ payload->at(0);

        for (int i = 0; i<payloadLenght; i++){

            uint8_t buff = s.mid(9+(i*2),2).toInt(nullptr, 16);

            crcAcc ^= buff;

            crcAcc &= 0xFF;

            payload->append(buff);
        }

        payload->append(crcAcc);

        qDebug() << payload->toHex();

        serialPort->write(writeCommand, 2);

        serialPort->flush();

        if(checkAck(10)){

            serialPort->write(*address);

            serialPort->flush();

            if(checkAck(50)){

                serialPort->write(*payload);

                serialPort->flush();

                if(checkAck(750)){

                }else {

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
    qDebug()<< *dataPath;
}

void FirmwareController::clearMCUFlash()
{
    checkConnectTimer->stop();

    serialPort->clear();

    serialPort->write(clearCommand, 2);

    serialPort->flush();

    if (checkAck(2)){                       // Empirically defined answer delay for STM32G474

        serialPort->write(clearComConf, 3);

        serialPort->flush();

        if (checkAck(30)){                  // Empirically defined answer delay for STM32G474

            emit mCUMemoryClearSucces();

            checkConnectTimer->start();

        } else {

            emit mCUMemoryClearError();

            closePort();
        }
    } else {

        emit mCUMemoryClearError();

        closePort();
    }
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

            qDebug() <<buff.toHex();


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
