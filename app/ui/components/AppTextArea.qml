import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// Multi-line, top-aligned text input for longer text (entry notes, etc).
// AppTextField wraps QQC2's single-line TextField, which vertically centers
// its one line inside a tall box — this wraps TextArea in a ScrollView
// instead: text starts at the top-left and the box scrolls internally once
// content overflows the visible height.
Rectangle {
    id: root

    property alias text: textArea.text
    property alias placeholderText: textArea.placeholderText

    radius: Theme.radiusMd
    color: Theme.surface
    border.width: 1
    border.color: textArea.activeFocus ? Theme.accent : Theme.borderDefault
    Behavior on border.color { ColorAnimation { duration: 120 } }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 1 // keep the outer Rectangle's border visible
        clip: true

        TextArea {
            id: textArea
            wrapMode: TextArea.Wrap
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            color: Theme.textPrimary
            placeholderTextColor: Theme.textDisabled
            selectionColor: Theme.accentSubtle
            selectedTextColor: Theme.textPrimary
            topPadding: Theme.space3
            bottomPadding: Theme.space3
            leftPadding: Theme.space3
            rightPadding: Theme.space3
            background: null // the outer Rectangle already draws the box
        }
    }
}
