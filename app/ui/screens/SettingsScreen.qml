import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Item {
    id: root

    signal exportRequested()
    signal importRequested()
    signal changeMasterPasswordRequested()
    signal backRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.bgCanvas
    }

    // Wrapped in a ScrollView so a short/small window can still reach the
    // bottom of the settings content (previously just a fixed ColumnLayout
    // with no way to scroll once it overflowed the visible height).
    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            id: contentColumn
            // Manual centering (x, not anchors.horizontalCenter) — avoids
            // relying on how ScrollView's internal Flickable contentItem
            // resolves "parent" for an anchor.
            x: Math.max(0, (scrollView.availableWidth - width) / 2)
            y: Theme.space6
            width: Math.min(480, scrollView.availableWidth)
            spacing: Theme.space5

            RowLayout {
                Layout.fillWidth: true
                AppButton { text: qsTr("← Назад"); variant: "ghost"; onClicked: root.backRequested() }
                Item { Layout.fillWidth: true }
            }

            Text {
                text: qsTr("Настройки")
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
                        text: qsTr("Безопасность")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Автоблокировка при бездействии")
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                        }
                        AppSwitch {
                            checked: appController.autoLockEnabled
                            onToggled: appController.autoLockEnabled = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: appController.autoLockEnabled
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Блокировать через (мин.)")
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeCaption
                        }
                        AppSpinBox {
                            from: 1
                            to: 60
                            value: appController.autoLockMinutes
                            onValueModified: appController.autoLockMinutes = value
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Автоочистка буфера обмена")
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                        }
                        AppSwitch {
                            checked: appController.clipboardClearEnabled
                            onToggled: appController.clipboardClearEnabled = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: appController.clipboardClearEnabled
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Очищать через (сек.)")
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeCaption
                        }
                        AppSpinBox {
                            from: 5
                            to: 120
                            value: appController.clipboardClearSeconds
                            onValueModified: appController.clipboardClearSeconds = value
                        }
                    }

                    AppButton {
                        text: qsTr("Сменить мастер-пароль")
                        variant: "secondary"
                        onClicked: root.changeMasterPasswordRequested()
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: appController.recoveryEnabled
                                  ? qsTr("Контрольные вопросы: настроены (%1)").arg(appController.recoveryQuestions().length)
                                  : qsTr("Контрольные вопросы: не настроены")
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            wrapMode: Text.WordWrap
                        }
                        AppButton {
                            text: appController.recoveryEnabled ? qsTr("Изменить") : qsTr("Настроить")
                            variant: "secondary"
                            onClicked: recoveryDialog.open()
                        }
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
                        text: qsTr("Внешний вид")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Тёмная тема")
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                        }
                        AppSwitch {
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
                        text: qsTr("Резервное копирование")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        spacing: Theme.space2
                        AppButton { text: qsTr("Экспорт хранилища…"); variant: "secondary"; onClicked: root.exportRequested() }
                        AppButton { text: qsTr("Импорт хранилища…"); variant: "secondary"; onClicked: root.importRequested() }
                    }
                }
            }

            // Bottom breathing room — included in contentColumn's own
            // implicitHeight (what ScrollView measures), unlike the top `y`
            // offset, so scrolling to the end shows equal top/bottom margins.
            Item { Layout.preferredHeight: Theme.space6 }
        }
    }

    RecoverySetupDialog {
        id: recoveryDialog
    }
}
