pragma Singleton
import QtQuick

// Design-token singleton for the whole app — Claude-inspired warm palette,
// see AGENTS.md for the source of these values and the reasoning behind
// them (font substitutes, contrast notes, etc). Every component/screen
// should reference Theme.* rather than hardcoding a color/size/font.
QtObject {
    id: theme

    // Toggle to switch the whole app's palette; SettingsScreen binds this
    // directly. main.cpp/Main.qml may also set it once at startup from
    // Qt.styleHints.colorScheme to follow the OS theme by default.
    property bool darkMode: false

    // ---- Light palette ----
    // Anchor colors verified against real claude.ai extractions (not
    // invented): background "Pampas" #F4F3EE, neutral grey "Cloudy"
    // #B1ADA1, accent "Crail" #C15F3C — see AGENTS.md for sources. Other
    // steps (borders, hover/pressed states, disabled) are this project's own
    // interpolation between those anchors, not independently verified.
    readonly property color _lightBgCanvas: "#F4F3EE"   // Pampas — verified
    readonly property color _lightBgBase: "#E7E4DA"     // sidebar — leans toward Cloudy for a visibly grey panel
    readonly property color _lightSurface: "#FDFCFA"
    readonly property color _lightSurfaceRaised: "#FFFFFF"
    readonly property color _lightBorderSubtle: "#E3E0D6"
    readonly property color _lightBorderDefault: "#D6D2C5"
    readonly property color _lightTextPrimary: "#2B2823"
    readonly property color _lightTextSecondary: "#847F72" // darkened Cloudy — Cloudy itself is too light for AA text contrast on Pampas
    readonly property color _lightTextDisabled: "#B3AEA1" // Cloudy — verified
    readonly property color _lightAccent: "#C15F3C"       // Crail — verified
    readonly property color _lightAccentHover: "#AA502F"
    readonly property color _lightAccentPressed: "#8F4026"
    readonly property color _lightAccentSubtle: "#1FC15F3C"
    readonly property color _lightSuccess: "#3F7B5C"
    readonly property color _lightWarning: "#C68A2E"
    readonly property color _lightDanger: "#C1392B"

    // ---- Dark palette ----
    // Not independently verified (search sources only described real
    // Claude's dark mode qualitatively as "deep charcoal with soft linen
    // text") — kept in the same warm-neutral family as the light palette,
    // with the accent lightened for legibility on a dark background.
    readonly property color _darkBgCanvas: "#1F1B16"
    readonly property color _darkBgBase: "#24201A"
    readonly property color _darkSurface: "#2C2720"
    readonly property color _darkSurfaceRaised: "#342E26"
    readonly property color _darkBorderSubtle: "#3D362C"
    readonly property color _darkBorderDefault: "#4A4235"
    readonly property color _darkTextPrimary: "#EDE9E0"
    readonly property color _darkTextSecondary: "#B0A99B"
    readonly property color _darkTextDisabled: "#766F60"
    readonly property color _darkAccent: "#D9744F"
    readonly property color _darkAccentHover: "#E28A67"
    readonly property color _darkAccentPressed: "#C05F3C"
    readonly property color _darkAccentSubtle: "#29D9744F"
    readonly property color _darkSuccess: "#6FAE85"
    readonly property color _darkWarning: "#E0A94E"
    readonly property color _darkDanger: "#E2685A"

    // ---- Resolved tokens (what components actually bind to) ----
    readonly property color bgCanvas: darkMode ? _darkBgCanvas : _lightBgCanvas
    readonly property color bgBase: darkMode ? _darkBgBase : _lightBgBase
    readonly property color surface: darkMode ? _darkSurface : _lightSurface
    readonly property color surfaceRaised: darkMode ? _darkSurfaceRaised : _lightSurfaceRaised
    readonly property color borderSubtle: darkMode ? _darkBorderSubtle : _lightBorderSubtle
    readonly property color borderDefault: darkMode ? _darkBorderDefault : _lightBorderDefault
    readonly property color textPrimary: darkMode ? _darkTextPrimary : _lightTextPrimary
    readonly property color textSecondary: darkMode ? _darkTextSecondary : _lightTextSecondary
    readonly property color textDisabled: darkMode ? _darkTextDisabled : _lightTextDisabled
    readonly property color accent: darkMode ? _darkAccent : _lightAccent
    readonly property color accentHover: darkMode ? _darkAccentHover : _lightAccentHover
    readonly property color accentPressed: darkMode ? _darkAccentPressed : _lightAccentPressed
    readonly property color accentSubtle: darkMode ? _darkAccentSubtle : _lightAccentSubtle
    readonly property color success: darkMode ? _darkSuccess : _lightSuccess
    readonly property color warning: darkMode ? _darkWarning : _lightWarning
    readonly property color danger: darkMode ? _darkDanger : _lightDanger
    readonly property color onAccentText: "#FFFFFF"

{"text": "    // ---- Typography ----\n    // \"Inter\" / \"JetBrains Mono\" are open-license stand-ins for Claude's\n    // actual (proprietary, non-bundleable) Styrene/Tiempos — see AGENTS.md.\n    // fontFamily is set from C++ via AppController::currentFontFamily()\n    // and updated through Main.qml's Connections block.\n    property string fontFamily: \"Inter\"\n    readonly property string monoFontFamily: \"JetBrains Mono\""}

    readonly property int fontSizeH1: 28
    readonly property int fontSizeH2: 20
    readonly property int fontSizeH3: 16
    readonly property int fontSizeBody: 14
    readonly property int fontSizeCaption: 12
    readonly property int fontSizeMono: 17
    readonly property int fontSizeMonoLarge: 32

    // ---- Spacing scale (4px base) ----
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 24
    readonly property int space6: 32
    readonly property int space7: 48
    readonly property int space8: 64

    // ---- Corner radius scale ----
    readonly property int radiusSm: 6
    readonly property int radiusMd: 10
    readonly property int radiusLg: 14
    readonly property int radiusXl: 20
}
