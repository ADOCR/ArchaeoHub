import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
Button {
    id: actionButton; objectName: "actionButton"
    property string variant: "secondary"
    property string symbol: ""
    property int textElide: Text.ElideNone
    readonly property color foreground: !enabled ? Theme.disabled : variant === "primary" ? Theme.accentText : Theme.body
    palette.buttonText: foreground
    palette.windowText: foreground
    palette.text: foreground
    implicitHeight: Theme.controlHeight
    implicitWidth: row.implicitWidth + 30
    leftPadding: 14; rightPadding: 14
    activeFocusOnTab: true
    font.pixelSize: Theme.bodySize
    contentItem: RowLayout {
        id: row; spacing: Theme.gap
        Symbol { visible: actionButton.symbol !== ""; name: actionButton.symbol; tint: actionButton.foreground; Layout.preferredWidth: 18; Layout.preferredHeight: 18 }
        Text { id: buttonCaption; objectName: "actionCaption"; text: actionButton.text; textFormat: Text.PlainText; font: actionButton.font; color: actionButton.foreground; horizontalAlignment: Text.AlignHCenter; elide: actionButton.textElide; Layout.fillWidth: true }
    }
    background: Rectangle {
        radius: Theme.radiusSmall
        color: !actionButton.enabled ? Theme.surface2 : actionButton.variant === "primary" ? (actionButton.hovered ? Theme.accentHover : Theme.accent) : actionButton.hovered || actionButton.down ? Theme.surface2 : actionButton.variant === "quiet" ? "transparent" : Theme.content
        border.width: actionButton.activeFocus ? 2 : actionButton.variant === "quiet" || actionButton.variant === "primary" ? 0 : 1
        border.color: actionButton.activeFocus ? Theme.accent : Theme.line
        Behavior on color { ColorAnimation { duration: Theme.motion } }
    }
    HoverHandler { cursorShape: actionButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
}
