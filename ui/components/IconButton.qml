import QtQuick
import QtQuick.Controls
import "../themes"
Button {
    id: control
    property string symbol: "info"
    property string hint: ""
    property bool selected: false
    implicitWidth: 36; implicitHeight: 36
    activeFocusOnTab: true
    Accessible.name: hint
    ToolTip.visible: hovered && hint !== ""
    ToolTip.text: hint
    contentItem: Symbol { name: control.symbol; tint: control.selected ? Theme.accent : Theme.muted }
    background: Rectangle { radius: Theme.radiusSmall; color: control.hovered || control.down ? Theme.surface2 : "transparent"; border.width: control.activeFocus ? 2 : 0; border.color: Theme.accent }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
