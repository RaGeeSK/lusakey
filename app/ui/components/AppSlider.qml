import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Themed Slider (password-generator length, etc.) — accent-colored fill and
// handle instead of QQC2 Basic's default grey groove.
Slider {
    id: control

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 160
        implicitHeight: 4
        width: control.availableWidth
        height: implicitHeight
        radius: height / 2
        color: Theme.borderSubtle

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color: Theme.accent
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 18
        implicitHeight: 18
        radius: 9
        color: Theme.surfaceRaised
        border.width: 2
        border.color: Theme.accent
    }
}
