import QtQuick
import QtQuick.Layouts
import Lusakey

Rectangle {
    id: root
    color: Theme.bgCanvas

    property bool errorVisible: false
    property bool vaultExists: true

    signal unlockRequested(string password)

    function shake() {
        shakeAnimation.start();
    }

    onErrorVisibleChanged: if (errorVisible) shake()

    Card {
        id: card
        width: 380
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.space6
            spacing: Theme.space4

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 48
                height: 48
                radius: width / 2
                color: Theme.accentSubtle

                Text {
                    anchors.centerIn: parent
                    text: "🔒"
                    font.pixelSize: 22
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.vaultExists ? qsTr("Unlock lusakey") : qsTr("Create your vault")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeH2
                font.weight: Font.DemiBold
            }

            AppTextField {
                id: passwordField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: root.vaultExists ? qsTr("Master password") : qsTr("Choose a master password")
                hasError: root.errorVisible
                focus: true
                onAccepted: root.unlockRequested(text)
            }

            Text {
                visible: root.errorVisible
                text: qsTr("Incorrect password")
                color: Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }

            AppButton {
                Layout.fillWidth: true
                text: root.vaultExists ? qsTr("Unlock") : qsTr("Create vault")
                variant: "primary"
                onClicked: root.unlockRequested(passwordField.text)
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Everything stays on this device — no account, no cloud.")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }
        }
    }

    SequentialAnimation {
        id: shakeAnimation
        NumberAnimation { target: card; property: "x"; to: card.x - 8; duration: 40 }
        NumberAnimation { target: card; property: "x"; to: card.x + 8; duration: 40 }
        NumberAnimation { target: card; property: "x"; to: card.x - 6; duration: 40 }
        NumberAnimation { target: card; property: "x"; to: card.x; duration: 40 }
    }
}
