import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
Button {
    id: control
    property string symbol: "tools"
    property bool selected: false
    property bool compact: false
    implicitHeight: 42
    leftPadding: 14; rightPadding: 12
    activeFocusOnTab: true
    Accessible.name: text
    ToolTip.text: text; ToolTip.visible: hovered && compact
    contentItem: RowLayout {
        spacing: 12
        Symbol { name: control.symbol; tint: control.selected ? Theme.accent : Theme.muted; Layout.preferredWidth: 20; Layout.preferredHeight: 20 }
        Label { visible: !control.compact; text: control.text; textFormat: Text.PlainText; color: control.selected ? Theme.ink : Theme.body; font.pixelSize: Theme.bodySize; font.weight: control.selected ? Font.DemiBold : Font.Normal; Layout.fillWidth: true; elide: Text.ElideRight }
    }
    background: Rectangle {
        radius: Theme.radiusSmall; color: control.selected ? Theme.accentSoft : control.hovered ? Theme.surface2 : "transparent"
        border.color: Theme.accent; border.width: control.activeFocus ? 2 : 0
        Rectangle { visible: control.selected; width: 3; height: 18; radius: 1.5; color: Theme.accent; anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter }
    }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
