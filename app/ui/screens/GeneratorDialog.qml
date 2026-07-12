import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Dialog {
    id: root

    title: qsTr("Генератор паролей")
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

    // Fixed width set directly on the Dialog, not via contentItem's
    // implicitWidth — see RecoverySetupDialog.qml for why (confirmed
    // binding-loop bug when a Layout-typed contentItem's implicitWidth is
    // assigned directly).
    width: 360
    // Capped height so this dialog can't grow taller than the window on a
    // small screen — the ScrollView below handles reaching the bottom
    // buttons when content overflows it (same pattern as LinkTotpDialog.qml).
    height: Math.min(480, (Overlay.overlay ? Overlay.overlay.height : 480) - Theme.space6 * 2)

    background: Rectangle {
        radius: Theme.radiusXl
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.borderSubtle
    }

    // QQC2's default Dialog header ignores Theme entirely (plain white
    // background, black text) — replace it so the title bar matches the
    // rest of the app instead of looking like a native/unstyled popup.
    header: Rectangle {
        color: Theme.surfaceRaised
        implicitHeight: headerLabel.implicitHeight + Theme.space4 * 2
        topLeftRadius: Theme.radiusXl
        topRightRadius: Theme.radiusXl

        Text {
            id: headerLabel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.space5
            anchors.rightMargin: Theme.space5
            text: root.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeH3
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
    }

    contentItem: ScrollView {
        id: dialogScroll
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
        x: Theme.space3
        width: dialogScroll.availableWidth - Theme.space3 * 2
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
                text: qsTr("Длина: %1").arg(root.length)
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }
            AppSlider {
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

            AppCheckBox {
                text: qsTr("Заглавные буквы")
                checked: root.includeUppercase
                onToggled: root.includeUppercase = checked
            }
            AppCheckBox {
                text: qsTr("Строчные буквы")
                checked: root.includeLowercase
                onToggled: root.includeLowercase = checked
            }
            AppCheckBox {
                text: qsTr("Цифры")
                checked: root.includeDigits
                onToggled: root.includeDigits = checked
            }
            AppCheckBox {
                text: qsTr("Символы")
                checked: root.includeSymbols
                onToggled: root.includeSymbols = checked
            }
        }

        AppCheckBox {
            text: qsTr("Исключить похожие символы (0/O, 1/l/I, …)")
            checked: root.excludeAmbiguous
            onToggled: root.excludeAmbiguous = checked
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppButton { text: qsTr("Обновить"); variant: "secondary"; onClicked: root.regenerateRequested() }
            Item { Layout.fillWidth: true }
            AppButton { text: qsTr("Отмена"); variant: "ghost"; onClicked: root.close() }
            AppButton {
                text: qsTr("Использовать этот пароль")
                variant: "primary"
                onClicked: root.useRequested(root.generatedPassword)
            }
        }
        }
    }
}
