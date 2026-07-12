import QtQuick
import QtQuick.Layouts
import Lusakey

// Dedicated "Authenticator Codes" grid (Authy/Bitwarden-style), separate
// from the inline TOTP block on EntryDetailPanel. `model` is expected to
// expose roles: entryId, title, code, secondsRemaining — see
// TotpListModel/AppController::totpListModel().
Item {
    id: root

    property alias model: gridView.model
    signal codeCopyRequested(var entryId)
    signal addRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.space5
        spacing: Theme.space4

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space3

            Text {
                Layout.fillWidth: true
                text: qsTr("Коды авторизации")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeH3
                font.weight: Font.DemiBold
            }

            AppButton {
                text: qsTr("+ Добавить")
                variant: "primary"
                onClicked: root.addRequested()
            }
        }

        GridView {
            id: gridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: 232
            cellHeight: 132

            delegate: Item {
                width: gridView.cellWidth
                height: gridView.cellHeight

                Card {
                    anchors.fill: parent
                    anchors.margins: Theme.space2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.space3
                        spacing: Theme.space2

                        Text {
                            Layout.fillWidth: true
                            text: model.title
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeBody
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space2

                            TotpRing {
                                progress: model.secondsRemaining / 30.0
                                secondsRemaining: model.secondsRemaining
                            }

                            Text {
                                Layout.fillWidth: true
                                text: model.code
                                color: Theme.textPrimary
                                font.family: Theme.monoFontFamily
                                font.pixelSize: Theme.fontSizeMono
                                font.weight: Font.Medium
                            }
                        }

                        AppButton {
                            Layout.fillWidth: true
                            text: qsTr("Копировать")
                            variant: "secondary"
                            onClicked: root.codeCopyRequested(model.entryId)
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: gridView.count === 0
                text: qsTr("Пока нет кодов авторизации.")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
            }
        }
    }
}
