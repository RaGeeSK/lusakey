import QtQuick
import QtQuick.Layouts
import Lusakey

// First-launch onboarding — a short, animated tour of what lusakey is and
// why it's built the way it is, shown once (AppController.showWelcome /
// dismissWelcome persist that via QSettings) before the unlock/create
// screen. Pushed on top of UnlockScreen by Main.qml, not a StackView
// replacement, so popping it just reveals the unlock screen underneath.
Rectangle {
    id: root
    color: Theme.bgCanvas
    opacity: 0

    signal finished()

    property int currentIndex: 0
    readonly property int pageCount: 4
    readonly property var pageIcons: [lockIconComponent, shieldIconComponent, keyIconComponent, gearIconComponent]
    readonly property var pageTitles: [
        qsTr("Добро пожаловать в lusakey"),
        qsTr("Никакого облака"),
        qsTr("Пароли и коды — вместе"),
        qsTr("Всё под вашим контролем")
    ]
    readonly property var pageTexts: [
        qsTr("Простой и безопасный менеджер паролей со встроенным генератором кодов двухфакторной аутентификации (TOTP)."),
        qsTr("Всё хранится только на этом устройстве, в одном зашифрованном файле. Ни серверов, ни аккаунтов, ни телеметрии."),
        qsTr("Коды авторизации живут рядом с паролями от тех же сервисов — как отдельное приложение-аутентификатор, только в одном месте."),
        qsTr("Генератор надёжных паролей, автоблокировка, автоочистка буфера обмена и восстановление по контрольным вопросам — настраивается под вас.")
    ]

    function goNext() {
        if (currentIndex < pageCount - 1) {
            currentIndex += 1;
        } else {
            root.finished();
        }
    }

    Component.onCompleted: entranceAnimation.start()

    NumberAnimation {
        id: entranceAnimation
        target: root
        property: "opacity"
        from: 0
        to: 1
        duration: 450
        easing.type: Easing.OutCubic
    }

    Item {
        id: viewport
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.space8
        anchors.bottom: dots.top
        anchors.bottomMargin: Theme.space5
        width: Math.min(520, parent.width - Theme.space8 * 2)
        clip: true

        Row {
            id: pagesRow
            height: viewport.height
            x: -root.currentIndex * viewport.width
            Behavior on x {
                NumberAnimation { duration: 380; easing.type: Easing.OutCubic }
            }

            Repeater {
                model: root.pageCount

                delegate: Item {
                    id: page
                    required property int index
                    width: viewport.width
                    height: viewport.height

                    ColumnLayout {
                        anchors.centerIn: parent
                        width: parent.width
                        spacing: Theme.space5

                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            width: 120
                            height: 120

                            Rectangle {
                                id: halo
                                anchors.centerIn: parent
                                width: 96
                                height: 96
                                radius: 48
                                color: Theme.accentSubtle

                                SequentialAnimation on scale {
                                    loops: Animation.Infinite
                                    running: root.currentIndex === page.index
                                    NumberAnimation { to: 1.08; duration: 1400; easing.type: Easing.InOutSine }
                                    NumberAnimation { to: 1.0; duration: 1400; easing.type: Easing.InOutSine }
                                }
                            }

                            Loader {
                                anchors.centerIn: parent
                                sourceComponent: root.pageIcons[page.index]
                                onLoaded: {
                                    if (item) {
                                        item.strokeColor = Theme.accent;
                                        item.width = 48;
                                        item.height = 48;
                                    }
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            text: root.pageTitles[page.index]
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeH1
                            font.weight: Font.DemiBold
                        }

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            text: root.pageTexts[page.index]
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeBody
                            lineHeight: 1.3
                        }
                    }
                }
            }
        }
    }

    Row {
        id: dots
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: buttonsRow.top
        anchors.bottomMargin: Theme.space6
        spacing: Theme.space2

        Repeater {
            model: root.pageCount

            delegate: Rectangle {
                required property int index
                width: index === root.currentIndex ? 20 : 8
                height: 8
                radius: 4
                color: index === root.currentIndex ? Theme.accent : Theme.borderDefault
                Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: 200 } }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentIndex = index
                }
            }
        }
    }

    RowLayout {
        id: buttonsRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.space7
        width: viewport.width
        spacing: Theme.space3

        AppButton {
            text: qsTr("Пропустить")
            variant: "ghost"
            visible: root.currentIndex < root.pageCount - 1
            onClicked: root.finished()
        }
        Item { Layout.fillWidth: true }
        AppButton {
            text: root.currentIndex < root.pageCount - 1 ? qsTr("Далее") : qsTr("Начать")
            variant: "primary"
            onClicked: root.goNext()
        }
    }

    Component { id: lockIconComponent; LockIcon {} }
    Component { id: shieldIconComponent; ShieldIcon {} }
    Component { id: keyIconComponent; KeyIcon {} }
    Component { id: gearIconComponent; GearIcon {} }
}
