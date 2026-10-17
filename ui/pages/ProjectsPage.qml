import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
import "../components"
ColumnLayout {
    id: page
    signal addRequested()
    signal createRequested()
    spacing: 24
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Proyectos"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold; Layout.fillWidth: true }
        ActionButton { text: qsTr("Vincular carpeta"); symbol: "folder"; onClicked: page.addRequested() }
        ActionButton { text: qsTr("Nuevo proyecto"); symbol: "plus"; variant: "primary"; enabled: !hub.creatingProject; onClicked: page.createRequested() }
    }
    Label { text: qsTr("Referencias a tus carpetas de trabajo. Los archivos permanecen donde están."); color: Theme.muted; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { visible: hub.creatingProject; text: qsTr("Creando la estructura del proyecto…"); color: Theme.accent; font.pixelSize: Theme.bodySize }
    ListView {
        id: list; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 1; model: hub.projects
        delegate: ItemDelegate {
            required property var modelData
            required property int index
            width: list.width; height: 60
            background: Rectangle { color: parent.hovered ? Theme.surface2 : Theme.content }
            contentItem: RowLayout {
                spacing: 16
                Symbol { name: "folder"; tint: Theme.accent }
                Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.fillWidth: true; elide: Text.ElideRight }
                Symbol { name: "arrow"; tint: Theme.muted }
            }
            onClicked: hub.openProject(index)
        }
        EmptyState { anchors.centerIn: parent; width: Math.min(380,parent.width-48); visible: list.count===0; title: qsTr("Comienza tu próximo proyecto"); description: qsTr("Crea su estructura de carpetas o vincula un proyecto que ya tengas.") }
        ScrollBar.vertical: ScrollBar {}
    }
}
