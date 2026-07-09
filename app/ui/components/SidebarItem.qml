import QtQuick
import QtQuick.Layouts
import Lusakey

// A single row in VaultListScreen's sidebar (Layout.fillWidth relies on
// being placed directly inside a ColumnLayout, hence importing
// QtQuick.Layouts here too).
Rectangle {
    id: root

    property string text: ""
    property bool selected: false
    signal clicked()

    Layout.fillWidth: true
    implicitHeight: 36
    radius: Theme.radiusMd
    color: root.selected ? Theme.accentSubtle : (mouseArea.containsMouse ? Theme.borderSubtle : "transparent")
    Behavior on color { ColorAnimation { duration: 120 } }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Theme.space3
        text: root.text
        color: root.selected ? Theme.accent : Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeBody
        font.weight: root.selected ? Font.DemiBold : Font.Normal
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
