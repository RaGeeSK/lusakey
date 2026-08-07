import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Item {
    id: root

    property var entryId: null
    property string entryTitle: ""
    property string username: ""
    property string password: ""
    property string url: ""
    property string notes: ""
    property bool hasTotp: false
    property string totpCode: ""
    property int totpSecondsRemaining: 30
    property string totpLinkError: ""
    // 0 = no folder (matches AppController::getEntry()'s "0 sentinel" and
    // AppController::setEntryFolder()'s "0 clears it" convention).
    property var folderId: 0
    property var folderChoices: [{folderId: 0, name: qsTr("Без папки")}]

    Component.onCompleted: {
        folderChoices = [{folderId: 0, name: qsTr("Без папки")}].concat(appController.folderOptions());
    }

    signal copyPasswordRequested(string password) // carries the field's *current* text, not the (stale) root.password
    signal copyTotpRequested()
    signal saveRequested(string title, string username, string password, string url, string notes)
    signal deleteRequested()
    signal closeRequested()
    signal linkTotpRequested(string otpauthUri)
    signal unlinkTotpRequested()

    // Surfaces the reason setEntryTotp() failed (malformed URI, etc) right
    // next to the field that caused it — harmless if some unrelated action
    // also emits this signal while the panel happens to be open, since it's
    // just advisory text that's overwritten the next time this fires.
    Connections {
        target: appController
        function onErrorOccurred(message) {
            root.totpLinkError = message;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bgCanvas
    }

    // Wrapped in a ScrollView so a short window (many fields + notes + TOTP
    // card) can still reach the Save button instead of clipping/overlapping
    // content that no longer fits.
    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            x: Theme.space5
            y: Theme.space5
            width: scrollView.availableWidth - Theme.space5 * 2
            spacing: Theme.space4

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppTextField {
                    id: titleField
                    Layout.fillWidth: true
                    text: root.entryTitle
                    font.pixelSize: Theme.fontSizeH3
                    placeholderText: qsTr("Название")
                }
                AppButton { text: qsTr("Закрыть"); variant: "ghost"; onClicked: root.closeRequested() }
                AppButton {
                    text: qsTr("Удалить")
                    variant: "ghost"
                    visible: root.entryId !== null
                    onClicked: root.deleteRequested()
                }
            }

            AppTextField {
                id: usernameField
                Layout.fillWidth: true
                text: root.username
                placeholderText: qsTr("Логин")
            }

            // Folder assignment — only offered once the entry exists
            // (entryId !== null), same gate as the TOTP-link section below:
            // a brand-new unsaved entry has no id yet to assign a folder to.
            ColumnLayout {
                visible: root.entryId !== null
                Layout.fillWidth: true
                spacing: Theme.space1

                Text {
                    text: qsTr("Папка")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }

                AppComboBox {
                    id: folderCombo
                    Layout.fillWidth: true
                    model: root.folderChoices
                    textRole: "name"
                    valueRole: "folderId"
                    currentIndex: {
                        for (let i = 0; i < root.folderChoices.length; i++) {
                            if (root.folderChoices[i].folderId === root.folderId) {
                                return i;
                            }
                        }
                        return 0;
                    }
                    onActivated: appController.setEntryFolder(root.entryId, currentValue)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppTextField {
                    id: passwordField
                    Layout.fillWidth: true
                    text: root.password
                    placeholderText: qsTr("Пароль")
                    monospace: true
                    revealable: true
                }
                AppButton {
                    text: qsTr("Копировать")
                    variant: "secondary"
                    onClicked: root.copyPasswordRequested(passwordField.text)
                }
                AppButton {
                    text: qsTr("Сгенерировать")
                    variant: "secondary"
                    onClicked: generatorDialog.open()
                }
            }

            AppTextField {
                id: urlField
                Layout.fillWidth: true
                text: root.url
                placeholderText: qsTr("URL")
            }

            AppTextArea {
                id: notesField
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                text: root.notes
                placeholderText: qsTr("Заметки")
            }

            Card {
                visible: root.hasTotp
                Layout.fillWidth: true
                implicitHeight: 72
                color: Theme.accentSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.space3
                    spacing: Theme.space3

                    TotpRing {
                        progress: root.totpSecondsRemaining / 30.0
                        secondsRemaining: root.totpSecondsRemaining
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.totpCode
                        color: Theme.textPrimary
                        font.family: Theme.monoFontFamily
                        font.pixelSize: Theme.fontSizeMonoLarge
                        font.weight: Font.Medium
                    }

                    AppButton {
                        text: qsTr("Копировать")
                        variant: "secondary"
                        onClicked: root.copyTotpRequested()
                    }
                    AppButton {
                        text: qsTr("Отвязать")
                        variant: "ghost"
                        onClicked: root.unlinkTotpRequested()
                    }
                }
            }

            // ---- Link a TOTP secret to this (already-saved) entry ----
            // Only offered once the entry exists (entryId !== null) — a
            // brand-new unsaved entry has no id yet to attach a secret to.
            ColumnLayout {
                visible: !root.hasTotp && root.entryId !== null
                Layout.fillWidth: true
                spacing: Theme.space2

                Text {
                    text: qsTr("Код авторизации (TOTP)")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space2

                    AppTextField {
                        id: totpUriField
                        Layout.fillWidth: true
                        placeholderText: qsTr("otpauth://totp/...")
                    }
                    AppButton {
                        text: qsTr("Привязать")
                        variant: "secondary"
                        enabled: totpUriField.text.length > 0
                        onClicked: root.linkTotpRequested(totpUriField.text)
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: root.totpLinkError.length > 0
                    text: root.totpLinkError
                    color: Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }
            }

            AppButton {
                Layout.fillWidth: true
                text: qsTr("Сохранить")
                variant: "primary"
                onClicked: root.saveRequested(titleField.text, usernameField.text, passwordField.text, urlField.text,
                                              notesField.text)
            }

            // Bottom breathing room, included in the ColumnLayout's own
            // implicitHeight (what ScrollView measures) for symmetric
            // top/bottom padding when scrolled to the end.
            Item { Layout.preferredHeight: Theme.space5 }
        }
    }

    GeneratorDialog {
        id: generatorDialog

        function regenerate() {
            generatedPassword = appController.generatePassword(
                length, includeUppercase, includeLowercase, includeDigits, includeSymbols, excludeAmbiguous);
            strengthScore = appController.estimatePasswordStrength(generatedPassword);
        }

        onOpened: regenerate()
        onRegenerateRequested: regenerate()
        onLengthChanged: regenerate()
        onIncludeUppercaseChanged: regenerate()
        onIncludeLowercaseChanged: regenerate()
        onIncludeDigitsChanged: regenerate()
        onIncludeSymbolsChanged: regenerate()
        onExcludeAmbiguousChanged: regenerate()
        onUseRequested: function (generated) {
            passwordField.text = generated;
            close();
        }
    }
}
