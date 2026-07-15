import QtQuick
import Lusakey

// Segmented strength indicator, fed by
// AppController.estimatePasswordStrength() (0..4). A heuristic traffic
// light, not a security guarantee — see password_generator.h's docs.
Item {
    id: meter

    property int score: 0 // 0..4
    implicitHeight: 6

    readonly property color _color: {
        if (score <= 1) return Theme.danger;
        if (score <= 2) return Theme.warning;
        return Theme.success;
    }

    Row {
        anchors.fill: parent
        spacing: Theme.space1

        Repeater {
            model: 4
            delegate: Rectangle {
                width: (meter.width - Theme.space1 * 3) / 4
                height: meter.height
                radius: Theme.radiusSm / 2
                color: index < meter.score ? meter._color : Theme.borderSubtle
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }
    }
}
