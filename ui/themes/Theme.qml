pragma Singleton
import QtQuick
QtObject {
    property Item materialSource: null
    property Item sceneSource: null
    property bool graphicsEffectsAvailable: false
    readonly property bool dark: preferences.theme === "dark" || (preferences.theme === "auto" && preferences.systemDark)
    readonly property bool opaque: preferences.reduceTransparency || preferences.highContrast
    readonly property real scale: preferences.textScale
    readonly property color canvas: dark ? "#0B0C0E" : "#F5F6F7"
    readonly property color content: dark ? "#111317" : "#FFFFFF"
    readonly property color surface1: dark ? "#15181C" : "#FAFAFB"
    readonly property color surface2: dark ? "#1B1F24" : "#F0F2F4"
    readonly property color surface3: dark ? "#23282E" : "#E8EBEE"
    readonly property color ink: dark ? "#F4F6F8" : "#15171A"
    readonly property color body: dark ? "#CDD2D8" : "#3F444B"
    readonly property color muted: preferences.highContrast ? body : (dark ? "#949BA4" : "#747B84")
    readonly property color disabled: dark ? "#626A73" : "#A3A8AE"
    readonly property color accent: dark ? "#4ED8C1" : "#0B7F6F"
    readonly property color accentHover: dark ? "#69E2CE" : "#096E61"
    readonly property color accentSoft: dark ? Qt.rgba(.306,.847,.757,.14) : Qt.rgba(.043,.498,.435,.12)
    readonly property color accentText: dark ? "#0B0C0E" : "#FFFFFF"
    readonly property color line: dark ? Qt.rgba(1,1,1,.09) : Qt.rgba(.078,.094,.11,.10)
    readonly property color strongLine: dark ? Qt.rgba(1,1,1,.16) : Qt.rgba(.078,.094,.11,.18)
    readonly property color glass: opaque ? surface1 : (dark ? Qt.rgba(.102,.114,.129,.84) : Qt.rgba(.973,.98,.988,.84))
    readonly property color glassBorder: dark ? Qt.rgba(1,1,1,.12) : Qt.rgba(1,1,1,.52)
    readonly property color glassTint: dark ? Qt.rgba(.102,.125,.141,.68) : Qt.rgba(.973,.984,.988,.72)
    readonly property color glassRim: dark ? Qt.rgba(1,1,1,.32) : Qt.rgba(1,1,1,.88)
    readonly property color glassInnerRim: dark ? Qt.rgba(1,1,1,.07) : Qt.rgba(.35,.42,.45,.10)
    readonly property color glassReflection: dark ? Qt.rgba(1,1,1,.12) : Qt.rgba(1,1,1,.48)
    readonly property color glassReflectionEnd: dark ? Qt.rgba(1,1,1,.015) : Qt.rgba(1,1,1,.035)
    readonly property color glassShade: Qt.rgba(0,0,0,dark?.08:.025)
    readonly property color glassShadow: Qt.rgba(0,0,0,dark?.45:.22)
    readonly property color materialTop: dark ? "#29433F" : "#CBDFD9"
    readonly property color materialMiddle: dark ? "#25323E" : "#DFE5EC"
    readonly property color materialBottom: dark ? "#28282D" : "#E8E1D9"
    readonly property color materialHorizon: dark ? "#18262C" : "#E2EAE9"
    readonly property color scrim: Qt.rgba(0,0,0,dark ? .48 : .18)
    readonly property color success: "#2E9B66"
    readonly property color warning: "#B97812"
    readonly property color error: "#C94A4A"
    readonly property int caption: Math.round(12*scale)
    readonly property int bodySize: Math.round(14*scale)
    readonly property int bodyLarge: Math.round(16*scale)
    readonly property int h2: Math.round(20*scale)
    readonly property int h1: Math.round(26*scale)
    readonly property int display: Math.round(32*scale)
    readonly property int controlHeight: Math.round(36*scale)
    readonly property int radiusSmall: 8
    readonly property int radius: 12
    readonly property int radiusPanel: 16
    readonly property int radiusGlass: 22
    readonly property int gap: 8
    readonly property int padding: 24
    readonly property int motion: preferences.reduceMotion ? 0 : 120
    readonly property string mono: Qt.platform.os === "windows" ? "Cascadia Mono" : Qt.platform.os === "osx" ? "Menlo" : "monospace"
}
