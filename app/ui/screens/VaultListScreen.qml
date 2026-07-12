import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Lusakey

Item {
    id: root

    signal entrySelected(var entryId)
    signal addEntryRequested()
    signal settingsRequested()
    signal searchTextChanged(string text)

    // "entries" | "totp" — switches the main pane while keeping the same
    // sidebar visible, instead of navigating to a separate full-screen
    // StackView page (that's still how Settings works, since it isn't one
    // of the sidebar's own "views").
    property string currentView: "entries"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Sidebar ----
        Rectangle {
            Layout.preferredWidth: 240
            Layout.fillHeight: true
            color: Theme.bgBase

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.space4
                spacing: Theme.space2

                Text {
                    text: qsTr("lusakey")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeH3
                    font.weight: Font.DemiBold
                }

                Item { Layout.preferredHeight: Theme.space3 }

                SidebarItem {
                    text: qsTr("Все записи")
                    icon: keyIconComponent
                    selected: root.currentView === "entries"
                    onClicked: root.currentView = "entries"
                }
                SidebarItem {
                    text: qsTr("Коды авторизации")
                    icon: shieldIconComponent
                    selected: root.currentView === "totp"
                    onClicked: root.currentView = "totp"
                }

                Item { Layout.fillHeight: true }

                SidebarItem {
                    text: qsTr("Настройки")
                    icon: gearIconComponent
                    onClicked: root.settingsRequested()
                }
            }
        }

        // ---- Main pane: entries list ----
        Rectangle {
            visible: root.currentView === "entries"
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgCanvas

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.space5
                spacing: Theme.space4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space3

                    AppTextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Поиск")
                        onTextChanged: root.searchTextChanged(text)
                    }

                    AppButton {
                        text: qsTr("+ Добавить")
                        variant: "primary"
                        onClicked: root.addEntryRequested()
                    }
                }

                ListView {
                    id: listView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: vaultListModel
                    spacing: Theme.space1

                    // Native ListView already scrolls (it's a Flickable) once
                    // entries overflow the pane; this bar is only a visible
                    // affordance so it's obvious there's more to scroll. Explicit
                    // `visible` because `policy: AsNeeded` alone still renders a
                    // full-height bar when the list is empty (contentHeight 0
                    // makes the size/position calculation degenerate instead of
                    // hiding it).
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        visible: listView.contentHeight > listView.height
                        background: null
                        contentItem: Rectangle {
                            implicitWidth: 6
                            radius: 3
                            color: Theme.borderDefault
                        }
                    }

                    // Narrower than listView.width (not just the ListView
                    // itself margined) — the scrollbar overlays listView's
                    // own right edge, so the row content needs its OWN
                    // reserved gutter to actually look separated from it.
                    delegate: Rectangle {
                        width: listView.width - Theme.space3
                        height: 56
                        radius: Theme.radiusMd
                        color: mouseArea.containsMouse ? Theme.accentSubtle : "transparent"
                        Behavior on color { ColorAnimation { duration: 100 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space3
                            anchors.rightMargin: Theme.space3
                            spacing: Theme.space3

                            Rectangle {
                                width: 32
                                height: 32
                                radius: 16
                                color: Theme.borderSubtle

                                Text {
                                    anchors.centerIn: parent
                                    text: model.title.length > 0 ? model.title.charAt(0).toUpperCase() : "?"
                                    color: Theme.textSecondary
                                    font.family: Theme.fontFamily
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: model.title
                                    color: Theme.textPrimary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSizeBody
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: model.username
                                    color: Theme.textSecondary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSizeCaption
                                    elide: Text.ElideRight
                                }
                            }

                            Chip {
                                visible: model.hasTotp
                                text: qsTr("2FA")
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.entrySelected(model.entryId)
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: listView.count === 0
                        text: qsTr("Пока нет записей — добавьте первый пароль.")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeBody
                    }
                }
            }
        }

        // ---- Main pane: authenticator codes ----
        TotpView {
            visible: root.currentView === "totp"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: totpListModel
            onCodeCopyRequested: function (entryId) {
                appController.copyToClipboard(appController.currentTotpCode(entryId));
            }
            onAddRequested: linkTotpDialog.open()
        }
    }

    LinkTotpDialog {
        id: linkTotpDialog
        onLinkRequested: function (entryId, otpauthUri) {
            const ok = entryId !== null
                ? appController.setEntryTotp(entryId, otpauthUri)
                : appController.addTotpEntry(otpauthUri);
            if (ok) {
                close();
            }
        }
    }

    Component { id: keyIconComponent; KeyIcon {} }
    Component { id: shieldIconComponent; ShieldIcon {} }
    Component { id: gearIconComponent; GearIcon {} }
}
