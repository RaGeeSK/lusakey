import QtQuick
import Lusakey

Rectangle {
    id: chip

    property string text: ""

    radius: Theme.radiusSm
    color: Theme.accentSubtle
    implicitHeight: label.implicitHeight + Theme.space2
    implicitWidth: label.implicitWidth + Theme.space3 * 2

    Text {
        id: label
        anchors.centerIn: parent
        text: chip.text
        color: Theme.accent
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeCaption
        font.weight: Font.DemiBold
    }
}
