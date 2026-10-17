import QtQuick
import QtQuick.Controls
import "../themes"
TextField {
    id: control
    implicitHeight: Theme.controlHeight + 2
    font.pixelSize: Theme.bodySize
    color: Theme.ink
    placeholderTextColor: Theme.muted
    selectionColor: Theme.accentSoft
    selectedTextColor: Theme.ink
    leftPadding: 12; rightPadding: 12
    selectByMouse: true
    background: Rectangle { color: Theme.content; radius: Theme.radiusSmall; border.color: control.activeFocus ? Theme.accent : Theme.strongLine; border.width: control.activeFocus ? 2 : 1 }
}
