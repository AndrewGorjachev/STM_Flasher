import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.5

Window {
    id: firmwareWindow
    title: qsTr("STM_Flasher")
    visible: true
    width: 300
    height: 270

    signal startFlashing();

    ColumnLayout {
        id: columnLayout
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.bottomMargin: 5
        anchors.topMargin: 5
        anchors.fill: parent
        visible: true

        RowLayout {
            id: rowLayoutTarget
            Layout.fillWidth: true
            Layout.fillHeight: false

            ComboBox {
                id: serialPortChose
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                model: firmwareController.getAllSerialPorts()
            }
            Button {
                id: openPort
                text: qsTr("Open Port")
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                onClicked:
                {
                    if(firmwareController.openPort(serialPortChose.currentText)===0)
                    {
                        enableComponents();

                    } else {

                        serialOpenErrorDialog.open();
                    }
                }
            }
            Button {
                id: closePort
                text: qsTr("Close Port")
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                onClicked:
                {
                    firmwareController.closePort(qsTr("Port has been closed by the user"));

                    disableComponents();
                }
            }
        }
        Button {
            id: choseFirmware
            text: qsTr("Chose Firmware")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked:
            {
                openFileDialog.open();
            }
        }
        Button {
            id: flashFirmware
            implicitWidth: serialPortChose.width
            text: qsTr("Flash Firmware")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked:
            {
                disableWhileOperation()

                firmwareController.flashFirmware();
            }
        }
        Button {
            id: clearMCUFlash
            implicitWidth: serialPortChose.width
            text: qsTr("Clear MCU Flash")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked:
            {
                firmwareController.clearMCUFlash();
            }
        }
        Button {
            id: backUpFirmware
            text: qsTr("Back Up Firmware")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked:
            {
                disableWhileOperation();

                saveBackUpDialog.open();
            }
        }
        ProgressBar {
            id: firmwareProgressBar
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            value: firmwareController.progress
        }

        RowLayout {
            Layout.fillHeight: false
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            Label {
                id: statusLabel
                text: "Status: "
                Layout.fillWidth: false
            }
            Label {
                id: connectionStatusLabel
                text: firmwareController.connectionStatus
                Layout.fillWidth: true
            }
        }
    }
    function disableComponents()
    {
        serialPortChose.enabled = true;
        openPort.enabled        = true;
        closePort.enabled       = false;
        choseFirmware.enabled   = false;
        flashFirmware.enabled   = false;
        backUpFirmware.enabled  = false;
        clearMCUFlash.enabled   = false;
    }
    function enableComponents()
    {
        serialPortChose.enabled = false;
        openPort.enabled        = false;
        closePort.enabled       = true;
        choseFirmware.enabled   = true
        backUpFirmware.enabled  = true;
        clearMCUFlash.enabled   = true;
    }
    function disableWhileOperation()
    {
        serialPortChose.enabled = false;
        openPort.enabled        = false;
        closePort.enabled       = false;
        choseFirmware.enabled   = false;
        flashFirmware.enabled   = false;
        backUpFirmware.enabled  = false;
        clearMCUFlash.enabled   = false;
    }

    Component.onCompleted:
    {
        disableComponents();
    }

    Connections
    {
        target: firmwareController;

        onPortClosed:
        {
            disableComponents();
        }
        onFirmwareReadError:
        {
            firmwareReadErrorDioalog.open();

            flashFirmware.enabled = false;
        }
        onFirmwareReadSucces:
        {
            flashFirmware.enabled = true;
        }
        onMCUMemoryClearError:
        {
            clearMCUMemoryErrorDialog.open();

            backUpFirmware.enabled = false;
        }
        onMCUMemoryClearSucces:
        {
            backUpFirmware.enabled = false;
        }
        onFirmwareFlashError:
        {
            flashFirmwareErrorDialog.open();

            disableComponents();

            backUpFirmware.enabled = false;
        }
        onFirmwareFlashSucces:
        {
            flashFirmwareCompletedDialog.open();

            enableComponents();

            flashFirmware.enabled   = true;
        }
    }
    FileDialog {
        id: openFileDialog
        title: "Please choose a folder"
        folder: shortcuts.home
        onAccepted:
        {
            firmwareController.readFirmwareFile(openFileDialog.fileUrl)
        }
        onRejected:
        {
            console.log("Open FW file has been canceled")
        }
    }

    FileDialog {
        id: saveBackUpDialog
        title: "Please choose a folder"
        selectFolder: true
        folder: shortcuts.home
        onAccepted:
        {
            firmwareController.backUpFirmware(saveBackUpDialog.fileUrls)
        }
        onRejected:
        {
            console.log("Saving FW BackUp file has been canceled")
        }
    }

    MessageDialog{
        id: serialOpenErrorDialog
        title: "Serial port error"
        text: "Error while open serial port"
        icon: StandardIcon.Critical
        standardButtons: StandardButton.Ok
        modality: Qt.WindowModal
    }

    MessageDialog{
        id: firmwareReadErrorDioalog
        title: "Firmware read error"
        text: "Error while reading MCU firmware file"
        icon: StandardIcon.Critical
        standardButtons: StandardButton.Ok
        modality: Qt.WindowModal
    }

    MessageDialog{
        id: clearMCUMemoryErrorDialog
        title: "Cleaning MCU memory error"
        text: "Error while cleaning MCU memory"
        icon: StandardIcon.Critical
        standardButtons: StandardButton.Ok
        modality: Qt.WindowModal
    }

    MessageDialog{
        id: flashFirmwareCompletedDialog
        title: "Flash firmware completed"
        text: "Flash firmware completed without error"
        icon: StandardIcon.Information
        standardButtons: StandardButton.Ok
        modality: Qt.WindowModal
    }

    MessageDialog{
        id: flashFirmwareErrorDialog
        title: "Flash firmware error"
        text: "Error while firmware flashing"
        icon: StandardIcon.Critical
        standardButtons: StandardButton.Ok
        modality: Qt.WindowModal
    }
}
