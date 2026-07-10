import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

// Lets the user configure (or replace) the vault's recovery slot: an
// arbitrary number of secret question/answer pairs (the user adds/removes
// rows freely — there's no fixed count). Answers are never retrievable once
// saved (they're only usable to re-derive the wrapping key, not stored in
// plaintext or recoverable form), so re-opening this dialog on an
// already-configured vault shows the existing QUESTIONS but blank answer
// fields — the user must re-enter every answer to save changes.
Dialog {
    id: root

    title: qsTr("Контрольные вопросы")
    modal: true
    standardButtons: Dialog.NoButton
    anchors.centerIn: parent

    property var rows: []

    // Named prepareRows() rather than reset() — "reset" collides with a
    // signal/method Dialog/Popup already defines (QML rejected it at
    // startup with "invalid override of property change signal or
    // superclass signal").
    function prepareRows() {
        const existing = appController.recoveryQuestions();
        if (existing.length > 0) {
            const newRows = [];
            for (let i = 0; i < existing.length; i++) {
                newRows.push({question: existing[i], answer: ""});
            }
            rows = newRows;
        } else {
            rows = [{question: "", answer: ""}, {question: "", answer: ""}];
        }
    }

    function addRow() {
        rows = rows.concat([{question: "", answer: ""}]);
    }

    function removeRow(index) {
        const copy = rows.slice();
        copy.splice(index, 1);
        rows = copy;
    }

    function save() {
        const questions = [];
        const answers = [];
        for (let i = 0; i < rows.length; i++) {
            questions.push(rows[i].question);
            answers.push(rows[i].answer);
        }
        appController.setupRecovery(questions, answers);
        root.close();
    }

    onOpened: prepareRows()

    // Fixed width set directly on the Dialog (not via contentItem's
    // implicitWidth) — assigning implicitWidth on a Layout-typed contentItem
    // fights Popup's own implicit-size negotiation and creates a binding
    // loop (confirmed at runtime: "Binding loop detected for property
    // implicitWidth").
    width: 420
    // Capped height (the user can add unlimited question rows) so this
    // never grows taller than the window — the ScrollView below handles
    // reaching the Save button when the row list overflows it.
    height: Math.min(560, (Overlay.overlay ? Overlay.overlay.height : 560) - Theme.space6 * 2)

    background: Rectangle {
        radius: Theme.radiusXl
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.borderSubtle
    }

    contentItem: ScrollView {
        id: dialogScroll
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: dialogScroll.availableWidth
            spacing: Theme.space4

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Если вы забудете мастер-пароль, вместо него можно будет ответить на эти вопросы. Выбирайте вопросы, ответ на которые знаете только вы — при сохранении нужно заново ввести все ответы, даже для неизменённых вопросов.")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space3

            Repeater {
                model: root.rows

                delegate: ColumnLayout {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    spacing: Theme.space1

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space2

                        AppTextField {
                            Layout.fillWidth: true
                            text: modelData.question
                            placeholderText: qsTr("Вопрос %1").arg(index + 1)
                            onTextChanged: root.rows[index].question = text
                        }
                        Rectangle {
                            visible: root.rows.length > 1
                            implicitWidth: 36
                            implicitHeight: 36
                            radius: Theme.radiusMd
                            color: removeMouseArea.containsMouse ? Theme.borderSubtle : "transparent"
                            Behavior on color { ColorAnimation { duration: 100 } }

                            CloseIcon {
                                anchors.centerIn: parent
                                strokeColor: Theme.textSecondary
                            }

                            MouseArea {
                                id: removeMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.removeRow(index)
                            }
                        }
                    }
                    AppTextField {
                        Layout.fillWidth: true
                        text: modelData.answer
                        placeholderText: qsTr("Ответ")
                        onTextChanged: root.rows[index].answer = text
                    }
                }
            }
        }

        AppButton {
            Layout.fillWidth: true
            text: qsTr("+ Добавить вопрос")
            variant: "secondary"
            onClicked: root.addRow()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppButton {
                visible: appController.recoveryEnabled
                text: qsTr("Отключить восстановление")
                variant: "ghost"
                onClicked: {
                    appController.disableRecovery();
                    root.close();
                }
            }
            Item { Layout.fillWidth: true }
            AppButton { text: qsTr("Отмена"); variant: "ghost"; onClicked: root.close() }
            AppButton { text: qsTr("Сохранить"); variant: "primary"; onClicked: root.save() }
        }
        }
    }
}
