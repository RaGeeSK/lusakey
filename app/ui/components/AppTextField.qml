import QtQuick
import QtQuick.Controls.Basic
import Lusakey

TextField {
    id: control

    property bool monospace: false
    property bool hasError: false
    // When true, the field behaves as a password field with a clickable
    // eye icon at the end that toggles visibility — used for the master
    // password field and entry passwords. Do not also set `echoMode`
    // externally when using this; it's managed internally.
    property bool revealable: false
    property bool revealed: false

    echoMode: control.revealable && !control.revealed ? TextInput.Password : TextInput.Normal

    font.family: monospace ? Theme.monoFontFamily : Theme.fontFamily
    font.pixelSize: monospace ? Theme.fontSizeMono : Theme.fontSizeBody
    color: Theme.textPrimary
    placeholderTextColor: Theme.textDisabled
    selectionColor: Theme.accentSubtle
    selectedTextColor: Theme.textPrimary
    topPadding: Theme.space3
    bottomPadding: Theme.space3
    leftPadding: Theme.space3
    rightPadding: control.revealable ? (revealIcon.width + Theme.space3 * 2) : Theme.space3
    selectByMouse: true

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface
        border.width: 1
        border.color: control.hasError ? Theme.danger : (control.activeFocus ? Theme.accent : Theme.borderDefault)
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    EyeIcon {
        id: revealIcon
        visible: control.revealable
        anchors.right: parent.right
        anchors.rightMargin: Theme.space3
        anchors.verticalCenter: parent.verticalCenter
        crossed: control.revealed
        strokeColor: revealMouseArea.containsMouse ? Theme.textPrimary : Theme.textSecondary

        MouseArea {
            id: revealMouseArea
            anchors.fill: parent
            anchors.margins: -6
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: control.revealed = !control.revealed
        }
    }
}
