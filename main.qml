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
    height: 200

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

            ComboBox  {
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
                onClicked: {

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
                onClicked: {

                    firmwareController.closePort();

                    disableComponents();
                }
            }
        }

        Button {
            id: choseFirmware
            text: qsTr("Chose Firmware")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked: {



            }
        }

        Button {
            id: flashFirmware
            implicitWidth: serialPortChose.width
            text: qsTr("Flash Firmware")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked: {

            }
        }

        Button {
            id: backUpFirmware
            implicitWidth: serialPortChose.width
            text: qsTr("Back Up Firmware")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked: {

            }
        }

        ProgressBar {
            id: firmwareProgressBar
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            value: 0
        }
    }

    function disableComponents()
    {
        serialPortChose.enabled = true;
        openPort.enabled = true;
        closePort.enabled = false;
        choseFirmware.enabled = false;
        flashFirmware.enabled = false;
        backUpFirmware.enabled = false;
    }


    function enableComponents()
    {
        serialPortChose.enabled = false;
        openPort.enabled = false;
        closePort.enabled = true;
        choseFirmware.enabled = true
        flashFirmware.enabled = true;
        backUpFirmware.enabled = true;
    }

    Component.onCompleted: {

        disableComponents();
    }


    FileDialog {
        id: openFileDialog
        title: "Please choose a folder"
        selectFolder: true
        folder: shortcuts.home
        onAccepted: {

            firmwareController.readFirmwareFile(openFile.fileUrls)
        }
        onRejected: {

            console.log("Canceled")
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


}
