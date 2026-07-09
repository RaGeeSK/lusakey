import QtQuick
import QtQuick.Layouts
import Lusakey

Item {
    id: root

    property var entryId: null
    property string entryTitle: ""
    property string username: ""
    property string password: ""
    property string url: ""
    property string notes: ""
    property bool hasTotp: false
    property string totpCode: ""
    property int totpSecondsRemaining: 30
    property bool passwordRevealed: false

    signal copyPasswordRequested(string password) // carries the field's *current* text, not the (stale) root.password
    signal copyTotpRequested()
    signal saveRequested(string title, string username, string password, string url, string notes)
    signal deleteRequested()
    signal closeRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.bgCanvas
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.space5
        spacing: Theme.space4

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppTextField {
                id: titleField
                Layout.fillWidth: true
                text: root.entryTitle
                font.pixelSize: Theme.fontSizeH3
                placeholderText: qsTr("Title")
            }
            AppButton { text: qsTr("Close"); variant: "ghost"; onClicked: root.closeRequested() }
            AppButton { text: qsTr("Delete"); variant: "ghost"; onClicked: root.deleteRequested() }
        }

        AppTextField {
            id: usernameField
            Layout.fillWidth: true
            text: root.username
            placeholderText: qsTr("Username")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppTextField {
                id: passwordField
                Layout.fillWidth: true
                text: root.password
                placeholderText: qsTr("Password")
                monospace: true
                echoMode: root.passwordRevealed ? TextInput.Normal : TextInput.Password
            }
            AppButton {
                text: root.passwordRevealed ? qsTr("Hide") : qsTr("Show")
                variant: "secondary"
                onClicked: root.passwordRevealed = !root.passwordRevealed
            }
            AppButton {
                text: qsTr("Copy")
                variant: "secondary"
                onClicked: root.copyPasswordRequested(passwordField.text)
            }
        }

        AppTextField {
            id: urlField
            Layout.fillWidth: true
            text: root.url
            placeholderText: qsTr("URL")
        }

        AppTextField {
            id: notesField
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            text: root.notes
            placeholderText: qsTr("Notes")
        }

        Card {
            visible: root.hasTotp
            Layout.fillWidth: true
            implicitHeight: 72
            color: Theme.accentSubtle

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.space3
                spacing: Theme.space3

                TotpRing {
                    progress: root.totpSecondsRemaining / 30.0
                    secondsRemaining: root.totpSecondsRemaining
                }

                Text {
                    Layout.fillWidth: true
                    text: root.totpCode
                    color: Theme.textPrimary
                    font.family: Theme.monoFontFamily
                    font.pixelSize: Theme.fontSizeMonoLarge
                    font.weight: Font.Medium
                }

                AppButton {
                    text: qsTr("Copy")
                    variant: "secondary"
                    onClicked: root.copyTotpRequested()
                }
            }
        }

        Item { Layout.fillHeight: true }

        AppButton {
            Layout.fillWidth: true
            text: qsTr("Save")
            variant: "primary"
            onClicked: root.saveRequested(titleField.text, usernameField.text, passwordField.text, urlField.text,
                                          notesField.text)
        }
    }
}
