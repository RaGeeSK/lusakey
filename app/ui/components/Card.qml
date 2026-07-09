import QtQuick
import Lusakey

// Base surface for grouped content — flat, hairline border, no drop shadow
// (Claude's style favors subtle borders over heavy elevation). Screens lay
// out their own content inside via anchors/margins.
Rectangle {
    radius: Theme.radiusLg
    color: Theme.surface
    border.width: 1
    border.color: Theme.borderSubtle
}
