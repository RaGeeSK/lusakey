import QtQuick
import QtQuick.Layouts
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
                text: qsTr("Шрифт")
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
                        text: qsTr("Выберите шрифт для интерфейса")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                    }

                    Repeater {
                        model: appController.fontFamilies

                        Item {
                            Layout.fillWidth: true
                            height: 36

                            RadioButton {
                                text: modelData
                                checked: modelData === appController.currentFontFamily
                                onClicked: appController.setFontFamily(modelData)
                                width: parent.width
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.space6 }
        }
    }
}
