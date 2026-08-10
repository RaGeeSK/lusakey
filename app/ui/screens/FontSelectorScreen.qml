import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Item {
    id: root

    signal backRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.bgCanvas
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            id: contentColumn
            x: Math.max(0, (scrollView.availableWidth - width) / 2)
            y: Theme.space6
            width: Math.min(480, scrollView.availableWidth - Theme.space3)
            spacing: Theme.space5

            RowLayout {
                Layout.fillWidth: true
                AppButton { text: qsTr("← Назад"); variant: "ghost"; onClicked: root.backRequested() }
                Item { Layout.fillWidth: true }
            }

            Text {
                text: qsTr("Шрифт интерфейса")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeH2
                font.weight: Font.DemiBold
            }

            Card {
                Layout.fillWidth: true
                implicitHeight: fontListColumn.implicitHeight + Theme.space4 * 2

                ColumnLayout {
                    id: fontListColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space4
                    spacing: Theme.space2

                    Text {
                        text: qsTr("Выберите шрифт — он применится сразу ко всему интерфейсу.")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: appController.fontFamilies

                        RadioButton {
                            text: modelData
                            font.family: modelData
                            font.pixelSize: Theme.fontSizeBody
                            checked: modelData === appController.currentFontFamily
                            onClicked: appController.setFontFamily(modelData)
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.space1
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle; Layout.topMargin: Theme.space3 }

                    Text {
                        text: qsTr("Пароли и коды подтверждения всегда используют моноширинный шрифт.")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.space6 }
        }
    }
}
