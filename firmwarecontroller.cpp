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
    if(checkConnectTimer != nullptr){

        checkConnectTimer->stop();

        delete checkConnectTimer;
    }
    if (serialPort != nullptr)
    {
        serialPort->close();

        delete serialPort;
    }
    if (firmwareBuffer != nullptr)
    {
        delete firmwareBuffer;
    }
    if (thread != nullptr)
    {
        thread->quit();

        thread->wait();

        thread->deleteLater();

        delete thread;
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

        if (checkAck(1))         // Empirically defined answer delay
        {
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
        }
    }
    return false;
}

void FirmwareController::closePort(const QString & closeStatus)
{
    emit portClosed();

    setConnectionStatus(closeStatus);

    if(checkConnectTimer->isActive())
    {
        checkConnectTimer->stop();
    }
    if(serialPort != nullptr)
    {
        if(serialPort->isOpen())
        {
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
            if(firmwareBuffer->at(firmwareBuffer->length()-2).startsWith(":04000005")){

                firmwareBuffer->removeAt(firmwareBuffer->length()-2);
            }
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

void FirmwareController::backUpFirmware(const QString & pathToBackUp)
{
    checkConnectTimer->stop();

    if (serialPort!=nullptr)
    {
        if(serialPort->isOpen())
        {
            serialPort->clear();

            serialPort->close();
        }

        backUpWorker = new FirmwareBackUpWorker(serialPort->portName(), pathToBackUp);

        if(thread != nullptr)
        {
            thread->quit();

            thread->wait();

            thread->deleteLater();

            thread = nullptr;
        }
        thread = new QThread( );

        connect(thread, &QThread::started, backUpWorker, &FirmwareBackUpWorker::run);

        connect(backUpWorker, &FirmwareBackUpWorker::progressValue, this, &FirmwareController::setProgress);

        connect(backUpWorker, &FirmwareBackUpWorker::finished, backUpWorker, &QObject::deleteLater);

        connect(backUpWorker, &FirmwareBackUpWorker::finished, this, &FirmwareController::backUpFirmwareHasFinished);

        backUpWorker->moveToThread(thread);

        thread->start();

    } else{

        closePort("Target device connection loss");
    }
}

void FirmwareController::flashFirmware()
{
    if (serialPort!=nullptr)
    {
        clearMCUFlash();

        checkConnectTimer->stop();

        serialPort->clear();

        serialPort->close();

        flashWorker = new FirmwareFlashWorker(serialPort->portName(), *firmwareBuffer);

        if(thread != nullptr){

            thread->quit();

            thread->wait();

            thread->deleteLater();

            thread = nullptr;
        }
        thread = new QThread();

        connect(thread, &QThread::started, flashWorker, &FirmwareFlashWorker::run);

        connect(flashWorker, &FirmwareFlashWorker::progressValue, this, &FirmwareController::setProgress);

        connect(flashWorker, &FirmwareFlashWorker::finished, flashWorker, &QObject::deleteLater);

        connect(flashWorker, &FirmwareFlashWorker::finished, this, &FirmwareController::flashFirmwareHasFinished);

        flashWorker->moveToThread(thread);

        thread->start();

    } else {

        closePort("Target device connection loss");
    }
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

            qDebug()<<"enter mCUMemoryClearError";

            emit mCUMemoryClearError();

            closePort("Firmware space wiping error");
        }
    } else {

        emit mCUMemoryClearError();

        closePort("Firmware space wiping error");
    }
}

bool FirmwareController::isPosibleToFlash()
{
    if(firmwareBuffer!=nullptr)
    {
        return true;
    }
    return false;
}

void FirmwareController::readMCUID()
{
    serialPort->clear();

    serialPort->write(pidCommand, 2);

    serialPort->flush();

    if(checkAck(10))
    {
        char * buff = new char[4];

        for(int i = 0; i<4; i++){

            if(serialPort->read(&buff[i], 1)!=1)
            {
                if(serialPort->waitForReadyRead(5))
                {
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

        } else {

            setConnectionStatus("Connected");
        }
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

void FirmwareController::flashFirmwareHasFinished(const QString & errorStatus)
{
    if(errorStatus=="w/o error"){

        if(serialPort->open(QIODevice::ReadWrite))
        {
            checkConnectTimer->start();

            emit firmwareFlashSucces();

        } else {

            setConnectionStatus("Firmware flash error");

            emit firmwareFlashError();
        }
    } else {

        setConnectionStatus("Firmware flash error");

        emit firmwareFlashError();
    }
    if(thread != nullptr)
    {
        thread->quit();

        thread->wait();

        thread->deleteLater();

        thread = nullptr;
    }
}

void FirmwareController::backUpFirmwareHasFinished(const QString & errorStatus)
{
    if(errorStatus=="w/o error")
    {
        if(serialPort->open(QIODevice::ReadWrite))
        {
            checkConnectTimer->start();

            emit firmwareBackUpSucces();

        } else {

            setConnectionStatus("Firmware backup error");

            emit firmwareBackUpError();
        }
    } else {

        setConnectionStatus("Firmware backup error");

        emit firmwareBackUpError();
    }
    if(thread != nullptr)
    {
        thread->quit();

        thread->wait();

        thread->deleteLater();

        thread = nullptr;
    }
}

float FirmwareController::getProgress() const
{
    return progress;
}

void FirmwareController::setProgress(const float &value)
{
    if(value != progress)
    {
        progress = value;

        emit progressChanged();
    }
}
