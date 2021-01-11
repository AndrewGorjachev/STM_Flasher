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
    if(checkConnectTimer!=nullptr){

        checkConnectTimer->stop();

        delete checkConnectTimer;
    }

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

            checkConnectTimer->start();

            return 0;

        } else {

            closePort("Connection Error");

            return 1;
        }
    } else {

        setConnectionStatus("Port Open Error");

        return 1;
    }
}

bool FirmwareController::checkAck(int timeout)
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

void FirmwareController::closePort(const QString & closeStatus)
{
    emit portClosed();

    setConnectionStatus(closeStatus);

    if(checkConnectTimer->isActive()){

        checkConnectTimer->stop();
    }
    if(serialPort != nullptr) {

        if(serialPort->isOpen()){

            serialPort->close();
        }
        delete serialPort;

        serialPort = nullptr;
    }
}

void FirmwareController::portError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::NoError)
    {
        qDebug() << error;

        closePort("Target device connection error");
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

    serialPort->clear();

    serialPort->close();

    FirmwareFlashWorker * worker = new FirmwareFlashWorker(serialPort->portName(), *firmwareBuffer);

    QThread * thread = new QThread( );

    connect(thread, &QThread::started, worker, &FirmwareFlashWorker::run);

    worker->moveToThread(thread);

    thread->start();




//    for(int i = 0; i < firmwareBuffer->length(); i++) {

//        //QThread::yieldCurrentThread();

//        const QString * s = &(firmwareBuffer->at(i));

//        int payloadLenght = s->mid(1,2).toInt(nullptr, 16);

//        int addressOffSet = s->mid(3,4).toInt(nullptr, 16);

//        addressOffSet += 0x08000000;

//        QByteArray * address = new QByteArray();

//        address->append((addressOffSet >> 24) & 0xFF);
//        address->append((addressOffSet >> 16) & 0xff);
//        address->append((addressOffSet >> 8) & 0xff);
//        address->append((addressOffSet & 0xff));

//        address->append(address->at(0)^address->at(1)^address->at(2)^address->at(3));

//        //qDebug()<< address->toHex();

//        QByteArray * payload = new QByteArray();

//        payload->append(payloadLenght - 1);

//        uint8_t crcAcc = 0 ^ payload->at(0);

//        for (int i = 0; i<payloadLenght; i++){

//            uint8_t buff = s->mid(9+(i*2),2).toInt(nullptr, 16);

//            crcAcc ^= buff;

//            crcAcc &= 0xFF;

//            payload->append(buff);
//        }

//        payload->append(crcAcc);

//        //qDebug() << payload->toHex();

//        serialPort->write(writeCommand, 2);

//        serialPort->flush();

//        if(checkAck(10)){

//            serialPort->write(*address);

//            serialPort->flush();

//            if(checkAck(50)){

//                serialPort->write(*payload);

//                serialPort->flush();

//                if(checkAck(500)){

//                    setProgress(static_cast<float>(i)/static_cast<float>(firmwareBuffer->length()));

//                } else {

//                    qDebug()<<"write data error";

//                    break;
//                }
//            } else {

//                qDebug()<<"write adress error";

//                break;
//            }
//        } else {

//            qDebug()<<"write command error";

//            break;
//        }


//        delete payload;

//    }
//    qDebug()<< *dataPath;
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

            closePort("Firmware space wiping error");
        }
    } else {

        emit mCUMemoryClearError();

        closePort("Firmware space wiping error");
    }
}

void FirmwareController::readMCUID()
{
    serialPort->clear();

    serialPort->write(pidCommand, 2);

    serialPort->flush();

    if(checkAck(10)){

        char * buff = new char[4];

        for(int i = 0; i<4; i++){

            if(serialPort->read(&buff[i], 1)!=1){

                if(serialPort->waitForReadyRead(5)){

                    serialPort->read(&buff[i], 1);

                } else{

                    closePort("Target device connection loss");
                }
            }
        }
        if(!((buff[0] == 0x01) &&
             (buff[1] == 0x04) &&
             (buff[2] == 'i' ) &&
             (buff[3] == 'y' )))
        {
            closePort("Target device connection loss");
        }

        qDebug() << hex << QString(buff);

        delete [] buff;

    } else {

        closePort("Target device connection loss");
    }
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

float FirmwareController::getProgress() const
{
    return progress;
}

void FirmwareController::setProgress(const float &value)
{
    qDebug()<< value;
    if(value != progress)
    {
        progress = value;

        emit progressChanged();
    }
}
