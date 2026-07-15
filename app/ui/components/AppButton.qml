import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Custom-styled button built on QQC2's Button (Basic style) so focus,
// hover, and keyboard handling come for free; only background/contentItem
// are replaced with Theme-driven painting.
Button {
    id: control

    property string variant: "primary" // "primary" | "secondary" | "ghost"

    readonly property color _bg: {
        if (variant === "primary") {
            return control.pressed ? Theme.accentPressed : (control.hovered ? Theme.accentHover : Theme.accent);
        }
        if (variant === "secondary") {
            return control.pressed ? Theme.borderDefault : (control.hovered ? Theme.borderSubtle : "transparent");
        }
        return control.hovered ? Theme.borderSubtle : "transparent"; // ghost
    }
    readonly property color _fg: variant === "primary" ? Theme.onAccentText : Theme.textPrimary
    readonly property color _borderColor: variant === "secondary" ? Theme.borderDefault : "transparent"

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBody
    font.weight: Font.DemiBold
    topPadding: Theme.space3
    bottomPadding: Theme.space3
    leftPadding: Theme.space4
    rightPadding: Theme.space4

    background: Rectangle {
        radius: Theme.radiusMd
        color: control._bg
        border.width: control.variant === "secondary" ? 1 : 0
        border.color: control._borderColor
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control._fg
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
