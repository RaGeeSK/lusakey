import QtQuick
import QtQuick.Controls.Basic
import Lusakey

TextField {
    id: control

    property bool monospace: false
    property bool hasError: false

    font.family: monospace ? Theme.monoFontFamily : Theme.fontFamily
    font.pixelSize: monospace ? Theme.fontSizeMono : Theme.fontSizeBody
    color: Theme.textPrimary
    placeholderTextColor: Theme.textDisabled
    selectionColor: Theme.accentSubtle
    selectedTextColor: Theme.textPrimary
    padding: Theme.space3
    selectByMouse: true

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface
        border.width: 1
        border.color: control.hasError ? Theme.danger : (control.activeFocus ? Theme.accent : Theme.borderDefault)
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
