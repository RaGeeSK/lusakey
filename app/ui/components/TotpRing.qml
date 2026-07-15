import QtQuick
import QtQuick.Shapes
import Lusakey

// Countdown ring for a TOTP code. `progress` (1.0 = just refreshed, 0.0 =
// about to refresh) and `secondsRemaining` are expected to be recomputed by
// the caller on a ~1s QTimer tick (see AppController.currentTotpSecondsRemaining)
// — this component only draws, it doesn't run its own clock, to keep exactly
// one source of truth for "what time is it" (avoids drift after the app is
// backgrounded/suspended).
Item {
    id: ring

    property real progress: 1.0
    property int secondsRemaining: 30
    implicitWidth: 40
    implicitHeight: 40

    readonly property color _color: secondsRemaining <= 5 ? Theme.danger : Theme.accent

    Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: Theme.borderSubtle
            strokeWidth: 3
            fillColor: "transparent"
            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.width / 2 - 2
                radiusY: ring.height / 2 - 2
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath {
            strokeColor: ring._color
            strokeWidth: 3
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            PathAngleArc {
                id: progressArc
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.width / 2 - 2
                radiusY: ring.height / 2 - 2
                startAngle: -90
                sweepAngle: 360 * ring.progress
                Behavior on sweepAngle { NumberAnimation { duration: 200 } }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: ring.secondsRemaining
        color: Theme.textSecondary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeCaption
    }
}
