import QtQuick
import QtQuick.Controls.Basic
import Lusakey

ComboBox {
    id: control

    property string labelRole: "text"
    property string valueField: ""

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBody
    topPadding: Theme.space3
    bottomPadding: Theme.space3
    leftPadding: Theme.space3
    rightPadding: Theme.space3
    flat: false

    delegate: ItemDelegate {
        width: control.width
        contentItem: Text {
            text: model[control.labelRole] ?? modelData ?? ""
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: highlighted ? Theme.accentSubtle : "transparent"
        }
    }

    contentItem: Text {
        text: control.currentText
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeBody
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Canvas {
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8

        onPaint: {
            var ctx = getContext("2d");
            ctx.fillStyle = Theme.textSecondary;
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.lineTo(width / 2, height);
            ctx.lineTo(width, 0);
            ctx.closePath();
            ctx.fill();
        }
    }

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.borderDefault
    }

    popup: Popup {
        y: control.height
        width: control.width
        padding: 0

        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.surface
            border.color: Theme.borderDefault
        }

        contentItem: ListView {
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            clip: true
        }
    }
}
