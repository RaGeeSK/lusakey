import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Dialog {
    id: root

    title: qsTr("Generate Password")
    modal: true
    standardButtons: Dialog.NoButton
    anchors.centerIn: parent

    property string generatedPassword: ""
    property int strengthScore: 0
    property int length: 20
    property bool includeUppercase: true
    property bool includeLowercase: true
    property bool includeDigits: true
    property bool includeSymbols: true
    property bool excludeAmbiguous: false

    signal regenerateRequested()
    signal useRequested(string password)

    background: Rectangle {
        radius: Theme.radiusXl
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.borderSubtle
    }

    contentItem: ColumnLayout {
        implicitWidth: 360
        spacing: Theme.space4

        Text {
            Layout.fillWidth: true
            text: root.generatedPassword
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeMono
            wrapMode: Text.WrapAnywhere
        }

        PasswordStrengthMeter {
            Layout.fillWidth: true
            score: root.strengthScore
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space3

            Text {
                text: qsTr("Length: %1").arg(root.length)
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }
            Slider {
                Layout.fillWidth: true
                from: 8
                to: 64
                stepSize: 1
                value: root.length
                onValueChanged: root.length = Math.round(value)
            }
        }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            columnSpacing: Theme.space3

            CheckBox {
                text: qsTr("Uppercase")
                checked: root.includeUppercase
                onToggled: root.includeUppercase = checked
            }
            CheckBox {
                text: qsTr("Lowercase")
                checked: root.includeLowercase
                onToggled: root.includeLowercase = checked
            }
            CheckBox {
                text: qsTr("Digits")
                checked: root.includeDigits
                onToggled: root.includeDigits = checked
            }
            CheckBox {
                text: qsTr("Symbols")
                checked: root.includeSymbols
                onToggled: root.includeSymbols = checked
            }
        }

        CheckBox {
            text: qsTr("Exclude ambiguous characters (0/O, 1/l/I, ...)")
            checked: root.excludeAmbiguous
            onToggled: root.excludeAmbiguous = checked
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppButton { text: qsTr("Regenerate"); variant: "secondary"; onClicked: root.regenerateRequested() }
            Item { Layout.fillWidth: true }
            AppButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: root.close() }
            AppButton {
                text: qsTr("Use this password")
                variant: "primary"
                onClicked: root.useRequested(root.generatedPassword)
            }
        }
    }
}
