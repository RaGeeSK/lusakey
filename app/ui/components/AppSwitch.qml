import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Themed Switch (dark mode toggle, etc.) — the accent color fills the track
// when on, instead of QQC2 Basic's default platform-grey/white look.
Switch {
    id: control

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBody

    indicator: Rectangle {
        implicitWidth: 44
        implicitHeight: 24
        radius: height / 2
        color: control.checked ? Theme.accent : Theme.borderDefault
        Behavior on color { ColorAnimation { duration: 120 } }

        Rectangle {
            width: 18
            height: 18
            radius: 9
            color: Theme.surfaceRaised
            anchors.verticalCenter: parent.verticalCenter
            x: control.checked ? parent.width - width - 3 : 3
            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.InOutQuad } }
        }
    }

    contentItem: Text {
        text: control.text
        visible: text.length > 0
        color: Theme.textPrimary
        font: control.font
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
