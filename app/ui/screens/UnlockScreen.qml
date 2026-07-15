import QtQuick
import QtQuick.Layouts
import Lusakey

// Unlock/create screen, plus a self-contained "forgot password" flow
// underneath the password field (only shown when unlocking an EXISTING
// vault): either answer configured secret questions, or wipe the vault
// entirely and start over. Reads/writes `appController` directly (it's a
// global QML context property, not just visible from Main.qml) for the
// recovery-specific actions, since nothing about screen navigation needs to
// change for them — Main.qml's existing Connections to
// appController.unlocked()/locked() already handle the outcome.
Rectangle {
    id: root
    color: Theme.bgCanvas

    property bool errorVisible: false
    property bool vaultExists: true
    property bool recoveryAvailable: appController.recoveryAvailable

    // Exposed so Main.qml can size the window's minimum bounds to this
    // screen's natural (unlock-mode) size on startup — read once, not bound,
    // since `card`'s height changes with `mode` (recovery/delete flows) and
    // we only want the primary unlock card's size as the floor.
    readonly property alias cardWidth: card.width
    readonly property alias cardHeight: card.height

    // "unlock" | "forgotMenu" | "recoveryAnswer" | "confirmDelete"
    property string mode: "unlock"
    property var answerFields: []
    property bool recoveryFailed: false

    // Re-grab keyboard focus for the password field when coming back to
    // this mode (e.g. clicking "Назад" out of the forgot-password flow).
    // The very first grab (on app launch) is driven by Main.qml instead —
    // see focusPasswordField() below — since forceActiveFocus() called this
    // early doesn't reliably stick until the platform window itself has
    // actually been activated by the OS.
    onModeChanged: if (mode === "unlock") passwordField.forceActiveFocus()

    function focusPasswordField() {
        if (mode === "unlock") {
            passwordField.forceActiveFocus();
        }
    }

    signal unlockRequested(string password)

    function shake() {
        shakeAnimation.start();
    }

    onErrorVisibleChanged: if (errorVisible) shake()

    function openForgotMenu() {
        mode = "forgotMenu";
    }

    function openRecoveryAnswerMode() {
        const questions = appController.recoveryQuestions();
        const fields = [];
        for (let i = 0; i < questions.length; i++) {
            fields.push({question: questions[i], answer: ""});
        }
        answerFields = fields;
        recoveryFailed = false;
        mode = "recoveryAnswer";
    }

    function submitRecoveryAnswers() {
        const answers = [];
        for (let i = 0; i < answerFields.length; i++) {
            answers.push(answerFields[i].answer);
        }
        appController.unlockWithRecoveryAnswers(answers);
    }

    Connections {
        target: appController
        function onLastRecoveryFailedChanged() {
            root.recoveryFailed = appController.lastRecoveryFailed;
            if (root.recoveryFailed) {
                shakeAnimation.start();
            }
        }
    }

    Card {
        id: card
        width: 380
        // Rectangle has no notion of "size to fit content" on its own, so
        // without this the ColumnLayout below (anchors.fill: parent) is
        // constrained to height 0 and all its children collapse/overlap
        // instead of stacking — bind height to the layout's own implicit
        // size (content + spacing) plus the margins on both sides. Since
        // hidden items don't count toward a Layout's implicitHeight, this
        // also makes the card resize automatically as `mode` changes.
        height: contentColumn.implicitHeight + Theme.space6 * 2
        anchors.centerIn: parent

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: Theme.space6
            spacing: Theme.space4

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 48
                height: 48
                radius: width / 2
                color: Theme.accentSubtle

                LockIcon {
                    anchors.centerIn: parent
                    visible: root.mode !== "confirmDelete"
                    implicitWidth: 22
                    implicitHeight: 22
                    strokeColor: Theme.accent
                }
                WarningIcon {
                    anchors.centerIn: parent
                    visible: root.mode === "confirmDelete"
                    implicitWidth: 22
                    implicitHeight: 22
                    strokeColor: Theme.danger
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: {
                    switch (root.mode) {
                    case "forgotMenu": return qsTr("Забыли пароль?");
                    case "recoveryAnswer": return qsTr("Ответьте на контрольные вопросы");
                    case "confirmDelete": return qsTr("Удалить всё?");
                    default: return root.vaultExists ? qsTr("Разблокировать lusakey") : qsTr("Создание хранилища");
                    }
                }
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeH2
                font.weight: Font.DemiBold
            }

            // ---- mode: unlock ----
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.mode === "unlock"
                spacing: Theme.space4

                AppTextField {
                    id: passwordField
                    Layout.fillWidth: true
                    revealable: true
                    placeholderText: root.vaultExists ? qsTr("Мастер-пароль") : qsTr("Придумайте мастер-пароль")
                    hasError: root.errorVisible
                    focus: root.mode === "unlock"
                    onAccepted: root.unlockRequested(text)
                }

                Text {
                    visible: root.errorVisible
                    text: qsTr("Неверный пароль")
                    color: Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }

                AppButton {
                    Layout.fillWidth: true
                    text: root.vaultExists ? qsTr("Разблокировать") : qsTr("Создать хранилище")
                    variant: "primary"
                    onClicked: root.unlockRequested(passwordField.text)
                }

                AppButton {
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.vaultExists
                    text: qsTr("Забыли пароль?")
                    variant: "ghost"
                    onClicked: root.openForgotMenu()
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Все данные хранятся только на этом устройстве — без аккаунта и облака.")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }
            }

            // ---- mode: forgotMenu ----
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.mode === "forgotMenu"
                spacing: Theme.space3

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Что вы хотите сделать?")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                }

                AppButton {
                    Layout.fillWidth: true
                    visible: root.recoveryAvailable
                    text: qsTr("Ответить на контрольные вопросы")
                    variant: "secondary"
                    onClicked: root.openRecoveryAnswerMode()
                }

                Text {
                    Layout.fillWidth: true
                    visible: !root.recoveryAvailable
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Для этого хранилища не настроены контрольные вопросы.")
                    color: Theme.textDisabled
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Удалить все данные и начать заново")
                    variant: "secondary"
                    onClicked: root.mode = "confirmDelete"
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Назад")
                    variant: "ghost"
                    onClicked: root.mode = "unlock"
                }
            }

            // ---- mode: recoveryAnswer ----
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.mode === "recoveryAnswer"
                spacing: Theme.space3

                Repeater {
                    model: root.answerFields

                    delegate: ColumnLayout {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: Theme.space1

                        Text {
                            Layout.fillWidth: true
                            text: modelData.question
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeCaption
                            wrapMode: Text.WordWrap
                        }
                        AppTextField {
                            Layout.fillWidth: true
                            hasError: root.recoveryFailed
                            onTextChanged: root.answerFields[index].answer = text
                        }
                    }
                }

                Text {
                    visible: root.recoveryFailed
                    text: qsTr("Один или несколько ответов неверны")
                    color: Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Восстановить")
                    variant: "primary"
                    onClicked: root.submitRecoveryAnswers()
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Назад")
                    variant: "ghost"
                    onClicked: root.mode = "forgotMenu"
                }
            }

            // ---- mode: confirmDelete ----
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.mode === "confirmDelete"
                spacing: Theme.space3

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Это безвозвратно удалит хранилище и все пароли/TOTP-секреты в нём. Действие нельзя отменить.")
                    color: Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                    font.weight: Font.Medium
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Да, удалить всё")
                    variant: "primary"
                    onClicked: appController.resetVault()
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Отмена")
                    variant: "ghost"
                    onClicked: root.mode = "forgotMenu"
                }
            }
        }
    }

    SequentialAnimation {
        id: shakeAnimation
        NumberAnimation { target: card; property: "x"; to: card.x - 8; duration: 40 }
        NumberAnimation { target: card; property: "x"; to: card.x + 8; duration: 40 }
        NumberAnimation { target: card; property: "x"; to: card.x - 6; duration: 40 }
        NumberAnimation { target: card; property: "x"; to: card.x; duration: 40 }
    }
}
