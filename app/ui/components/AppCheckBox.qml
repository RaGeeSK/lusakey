import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Themed CheckBox — used for the password generator's character-class
// toggles instead of QQC2 Basic's default white checkbox.
CheckBox {
    id: control

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBody

    indicator: Rectangle {
        implicitWidth: 20
        implicitHeight: 20
        radius: Theme.radiusSm
        color: control.checked ? Theme.accent : Theme.surface
        border.width: 1
        border.color: control.checked ? Theme.accent : Theme.borderDefault
        Behavior on color { ColorAnimation { duration: 120 } }

        CheckIcon {
            anchors.centerIn: parent
            visible: control.checked
            implicitWidth: 12
            implicitHeight: 12
            strokeColor: Theme.onAccentText
        }
    }

    contentItem: Text {
        text: control.text
        color: Theme.textPrimary
        font: control.font
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
    }
}
