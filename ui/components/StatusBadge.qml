import QtQuick
import QtQuick.Controls
import "../themes"
Rectangle {
    id: badge
    property string text: ""
    property bool positive: false
    implicitWidth: label.implicitWidth + 18
    implicitHeight: label.implicitHeight + 8
    radius: height/2
    color: positive ? Theme.accentSoft : Theme.surface2
    Label { id: label; anchors.centerIn: parent; text: badge.text; color: badge.positive ? Theme.accent : Theme.muted; font.pixelSize: Theme.caption }
}
