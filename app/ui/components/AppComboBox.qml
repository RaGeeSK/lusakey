import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Themed ComboBox — QQC2's default Basic-style ComboBox renders its own
// white background/black text and a plain popup, regardless of Theme, same
// as every other bare QQC2 control in this app.
ComboBox {
    id: control

    implicitHeight: 36

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBody

    indicator: ChevronDownIcon {
        x: control.width - width - Theme.space3
        y: (control.height - height) / 2
        strokeColor: Theme.textSecondary
    }

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        leftPadding: Theme.space3
        rightPadding: control.indicator.width + Theme.space3
    }

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface
        border.width: 1
        border.color: control.activeFocus || control.popup.visible ? Theme.accent : Theme.borderDefault
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    popup: Popup {
        y: control.height + Theme.space1
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight, 240)
        padding: 1

        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.surfaceRaised
            border.width: 1
            border.color: Theme.borderSubtle
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollBar.vertical: ScrollBar {}
        }
    }

    delegate: ItemDelegate {
        width: control.width
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            text: control.textRole ? model[control.textRole] : modelData
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: parent.highlighted ? Theme.accentSubtle : (parent.hovered ? Theme.borderSubtle : "transparent")
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }
}
