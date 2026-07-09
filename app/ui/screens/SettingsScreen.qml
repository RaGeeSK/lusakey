import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Item {
    id: root

    property int autoLockMinutes: 5
    property int clipboardClearSeconds: 20

    signal exportRequested()
    signal importRequested()
    signal changeMasterPasswordRequested()
    signal backRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.bgCanvas
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.margins: Theme.space6
        width: 480
        spacing: Theme.space5

        RowLayout {
            Layout.fillWidth: true
            AppButton { text: qsTr("← Back"); variant: "ghost"; onClicked: root.backRequested() }
            Item { Layout.fillWidth: true }
        }

        Text {
            text: qsTr("Settings")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeH2
            font.weight: Font.DemiBold
        }

        Card {
            Layout.fillWidth: true
            implicitHeight: securityColumn.implicitHeight + Theme.space4 * 2

            ColumnLayout {
                id: securityColumn
                anchors.fill: parent
                anchors.margins: Theme.space4
                spacing: Theme.space3

                Text {
                    text: qsTr("Security")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                    font.weight: Font.DemiBold
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Auto-lock after inactivity (minutes)")
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                    }
                    SpinBox {
                        from: 1
                        to: 60
                        value: root.autoLockMinutes
                        onValueModified: root.autoLockMinutes = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Clear clipboard after (seconds)")
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                    }
                    SpinBox {
                        from: 5
                        to: 120
                        value: root.clipboardClearSeconds
                        onValueModified: root.clipboardClearSeconds = value
                    }
                }

                AppButton {
                    text: qsTr("Change master password")
                    variant: "secondary"
                    onClicked: root.changeMasterPasswordRequested()
                }
            }
        }

        Card {
            Layout.fillWidth: true
            implicitHeight: appearanceColumn.implicitHeight + Theme.space4 * 2

            ColumnLayout {
                id: appearanceColumn
                anchors.fill: parent
                anchors.margins: Theme.space4
                spacing: Theme.space3

                Text {
                    text: qsTr("Appearance")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                    font.weight: Font.DemiBold
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Dark mode")
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                    }
                    Switch {
                        checked: Theme.darkMode
                        onToggled: Theme.darkMode = checked
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            implicitHeight: backupColumn.implicitHeight + Theme.space4 * 2

            ColumnLayout {
                id: backupColumn
                anchors.fill: parent
                anchors.margins: Theme.space4
                spacing: Theme.space3

                Text {
                    text: qsTr("Backup")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                    font.weight: Font.DemiBold
                }

                RowLayout {
                    spacing: Theme.space2
                    AppButton { text: qsTr("Export vault…"); variant: "secondary"; onClicked: root.exportRequested() }
                    AppButton { text: qsTr("Import vault…"); variant: "secondary"; onClicked: root.importRequested() }
                }
            }
        }
    }
}
