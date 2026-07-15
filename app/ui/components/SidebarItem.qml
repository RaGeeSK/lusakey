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
    // Component producing one of the Canvas-based icons (KeyIcon,
    // ShieldIcon, GearIcon, ...) — null means no icon, just text.
    property Component icon: null
    signal clicked()

    Layout.fillWidth: true
    implicitHeight: 36
    radius: Theme.radiusMd
    color: root.selected ? Theme.accentSubtle : (mouseArea.containsMouse ? Theme.borderSubtle : "transparent")
    Behavior on color { ColorAnimation { duration: 120 } }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space3
        anchors.rightMargin: Theme.space3
        spacing: Theme.space2

        Loader {
            id: iconLoader
            active: root.icon !== null
            sourceComponent: root.icon
            Layout.alignment: Qt.AlignVCenter
        }
        Binding {
            target: iconLoader.item
            property: "strokeColor"
            value: root.selected ? Theme.accent : Theme.textSecondary
            when: iconLoader.item !== null
        }

        Text {
            Layout.fillWidth: true
            text: root.text
            color: root.selected ? Theme.accent : Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            font.weight: root.selected ? Font.DemiBold : Font.Normal
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
