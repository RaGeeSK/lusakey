import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
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

    signal copyPasswordRequested(string password) // carries the field's *current* text, not the (stale) root.password
    signal copyTotpRequested()
    signal saveRequested(string title, string username, string password, string url, string notes)
    signal deleteRequested()
    signal closeRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.bgCanvas
    }

    // Wrapped in a ScrollView so a short window (many fields + notes + TOTP
    // card) can still reach the Save button instead of clipping/overlapping
    // content that no longer fits.
    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            x: Theme.space5
            y: Theme.space5
            width: scrollView.availableWidth - Theme.space5 * 2
            spacing: Theme.space4

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppTextField {
                    id: titleField
                    Layout.fillWidth: true
                    text: root.entryTitle
                    font.pixelSize: Theme.fontSizeH3
                    placeholderText: qsTr("Название")
                }
                AppButton { text: qsTr("Закрыть"); variant: "ghost"; onClicked: root.closeRequested() }
                AppButton {
                    text: qsTr("Удалить")
                    variant: "ghost"
                    visible: root.entryId !== null
                    onClicked: root.deleteRequested()
                }
            }

            AppTextField {
                id: usernameField
                Layout.fillWidth: true
                text: root.username
                placeholderText: qsTr("Логин")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppTextField {
                    id: passwordField
                    Layout.fillWidth: true
                    text: root.password
                    placeholderText: qsTr("Пароль")
                    monospace: true
                    revealable: true
                }
                AppButton {
                    text: qsTr("Копировать")
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

            AppTextArea {
                id: notesField
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                text: root.notes
                placeholderText: qsTr("Заметки")
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
                        text: qsTr("Копировать")
                        variant: "secondary"
                        onClicked: root.copyTotpRequested()
                    }
                }
            }

            AppButton {
                Layout.fillWidth: true
                text: qsTr("Сохранить")
                variant: "primary"
                onClicked: root.saveRequested(titleField.text, usernameField.text, passwordField.text, urlField.text,
                                              notesField.text)
            }

            // Bottom breathing room, included in the ColumnLayout's own
            // implicitHeight (what ScrollView measures) for symmetric
            // top/bottom padding when scrolled to the end.
            Item { Layout.preferredHeight: Theme.space5 }
        }
    }
}
