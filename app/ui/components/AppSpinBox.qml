import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Themed SpinBox — QQC2's default Basic-style SpinBox renders with its own
// white background/black text regardless of Theme, so every numeric
// stepper in the app (auto-lock minutes, clipboard-clear seconds, ...)
// should use this instead of a bare SpinBox.
SpinBox {
    id: control

    implicitHeight: 36

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBody

    contentItem: TextInput {
        text: control.displayText
        font: control.font
        color: Theme.textPrimary
        selectionColor: Theme.accentSubtle
        selectedTextColor: Theme.textPrimary
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        leftPadding: control.down.indicator.width
        rightPadding: control.up.indicator.width
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.indicator: Rectangle {
        x: control.width - width
        width: 32
        height: control.height
        color: control.up.pressed ? Theme.accentPressed : (control.up.hovered ? Theme.accentHover : "transparent")
        Behavior on color { ColorAnimation { duration: 100 } }

        Text {
            anchors.centerIn: parent
            text: "+"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            font.bold: true
            color: (control.up.pressed || control.up.hovered) ? Theme.onAccentText : Theme.textSecondary
        }
    }

    down.indicator: Rectangle {
        x: 0
        width: 32
        height: control.height
        color: control.down.pressed ? Theme.accentPressed : (control.down.hovered ? Theme.accentHover : "transparent")
        Behavior on color { ColorAnimation { duration: 100 } }

        Text {
            anchors.centerIn: parent
            text: "−"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            font.bold: true
            color: (control.down.pressed || control.down.hovered) ? Theme.onAccentText : Theme.textSecondary
        }
    }

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.borderDefault
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
