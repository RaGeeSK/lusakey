import QtQuick
import QtQuick.Layouts
import Lusakey

Item {
    id: root

    signal entrySelected(var entryId)
    signal addEntryRequested()
    signal settingsRequested()
    signal authenticatorRequested()
    signal searchTextChanged(string text)

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

                SidebarItem { text: qsTr("All Items"); selected: true }
                SidebarItem { text: qsTr("Authenticator Codes"); onClicked: root.authenticatorRequested() }

                Item { Layout.fillHeight: true }

                SidebarItem { text: qsTr("Settings"); onClicked: root.settingsRequested() }
            }
        }

        // ---- Main list ----
        Rectangle {
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
                        placeholderText: qsTr("Search")
                        onTextChanged: root.searchTextChanged(text)
                    }

                    AppButton {
                        text: qsTr("+ Add")
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

                    delegate: Rectangle {
                        width: listView.width
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
                        text: qsTr("No items yet — add your first password.")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeBody
                    }
                }
            }
        }
    }
}
