import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

// Links an existing entry to a TOTP secret pasted as an otpauth://totp/...
// URI — the format any 2FA issuer's QR code decodes to (see
// AppController::setEntryTotp; there's no image/QR import here, just text).
// Opened from the "Authenticator Codes" tab's "+ Добавить" button.
Dialog {
    id: root

    title: qsTr("Привязать код авторизации")
    modal: true
    standardButtons: Dialog.NoButton
    anchors.centerIn: parent

    property var selectedEntryId: null
    property string errorText: ""

    signal linkRequested(var entryId, string otpauthUri)

    // Named prepareForOpen() rather than reset() — "reset" collides with a
    // signal/method Dialog/Popup already defines (see RecoverySetupDialog.qml
    // for the same gotcha, confirmed at runtime in this project before).
    function prepareForOpen() {
        selectedEntryId = null;
        errorText = "";
        uriField.text = "";
    }

    onOpened: prepareForOpen()

    // Surfaces the real reason setEntryTotp() failed (malformed URI, entry
    // vanished, etc) — harmless if some unrelated action also emits this
    // signal while the dialog happens to be open, since it's just advisory
    // text reset on every open.
    Connections {
        target: appController
        function onErrorOccurred(message) {
            root.errorText = message;
        }
    }

    width: 420
    height: Math.min(520, (Overlay.overlay ? Overlay.overlay.height : 520) - Theme.space6 * 2)

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
                wrapMode: Text.WordWrap
                text: qsTr("Выберите запись и вставьте otpauth:// ссылку — такую же, как закодирована в QR-коде сервиса.")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }

            Text {
                text: qsTr("Запись (необязательно — если не выбрать, будет создана новая)")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                radius: Theme.radiusMd
                color: Theme.surface
                border.width: 1
                border.color: Theme.borderDefault

                ListView {
                    id: entryListView
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: vaultListModel

                    delegate: Rectangle {
                        width: entryListView.width
                        height: 44
                        color: model.entryId === root.selectedEntryId
                               ? Theme.accentSubtle
                               : (entryMouseArea.containsMouse ? Theme.borderSubtle : "transparent")
                        Behavior on color { ColorAnimation { duration: 100 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space3
                            anchors.rightMargin: Theme.space3
                            spacing: Theme.space2

                            Text {
                                Layout.fillWidth: true
                                text: model.title
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeBody
                                elide: Text.ElideRight
                            }
                            Chip {
                                visible: model.hasTotp
                                text: qsTr("2FA")
                            }
                        }

                        MouseArea {
                            id: entryMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectedEntryId = model.entryId
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: entryListView.count === 0
                        text: qsTr("Нет записей")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                    }
                }
            }

            AppTextField {
                id: uriField
                Layout.fillWidth: true
                placeholderText: qsTr("otpauth://totp/...")
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: root.errorText.length > 0
                text: root.errorText
                color: Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                Item { Layout.fillWidth: true }
                AppButton { text: qsTr("Отмена"); variant: "ghost"; onClicked: root.close() }
                AppButton {
                    text: root.selectedEntryId !== null ? qsTr("Привязать") : qsTr("Добавить запись")
                    variant: "primary"
                    enabled: uriField.text.length > 0
                    onClicked: root.linkRequested(root.selectedEntryId, uriField.text)
                }
            }
        }
    }
}
