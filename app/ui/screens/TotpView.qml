import QtQuick
import QtQuick.Layouts
import Lusakey

// Dedicated "Authenticator Codes" grid (Authy/Bitwarden-style), separate
// from the inline TOTP block on EntryDetailPanel. `model` is expected to
// expose roles: entryId, title, code, secondsRemaining — see the
// known-gap note in AGENTS.md: a dedicated TotpListModel (ticking every
// second) still needs to be written; this screen is the target UI shape
// for it.
Item {
    id: root

    property alias model: gridView.model
    signal codeCopyRequested(var entryId)

    GridView {
        id: gridView
        anchors.fill: parent
        anchors.margins: Theme.space5
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
                        text: qsTr("Copy")
                        variant: "secondary"
                        onClicked: root.codeCopyRequested(model.entryId)
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: gridView.count === 0
            text: qsTr("No authenticator codes yet — add a TOTP secret to an entry.")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }
}
