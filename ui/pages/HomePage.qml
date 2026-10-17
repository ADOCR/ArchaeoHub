import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
import "../components"
ScrollView {
    id: view
    signal addProjectRequested()
    signal createProjectRequested()
    signal commandRequested()
    clip: true; contentWidth: availableWidth
    ColumnLayout {
        width: view.availableWidth; spacing: 32
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8; Layout.fillWidth: true
                Label { text: hub.productName; color: Theme.ink; font.pixelSize: Theme.display; font.weight: Font.DemiBold }
                Label { text: qsTr("Tu espacio de trabajo arqueológico"); color: Theme.muted; font.pixelSize: Theme.bodyLarge }
            }
            ActionButton { text: qsTr("Buscar"); symbol: "search"; onClicked: view.commandRequested(); ToolTip.text: "Ctrl / ⌘ + K"; ToolTip.visible: hovered }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 16
            Label { text: qsTr("Continuar trabajando"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
            Rectangle {
                visible: hub.projects.length===0; Layout.fillWidth: true; implicitHeight: projectEmpty.implicitHeight+40
                color: Theme.content; border.color: Theme.line; radius: Theme.radius
                RowLayout {
                    id: projectEmpty; anchors.fill: parent; anchors.margins: 20; spacing: 24
                    EmptyState { Layout.fillWidth: true; title: qsTr("Vincula tu primer proyecto"); description: qsTr("Accede a su carpeta desde aquí, sin mover ni duplicar tus archivos.") }
                    ColumnLayout {
                        ActionButton { text: qsTr("Nuevo proyecto"); variant: "primary"; symbol: "plus"; enabled: !hub.creatingProject; onClicked: view.createProjectRequested() }
                        ActionButton { text: qsTr("Vincular carpeta"); variant: "quiet"; symbol: "folder"; onClicked: view.addProjectRequested() }
                    }
                }
            }
            Repeater {
                model: hub.projects.slice(0,3)
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true; implicitHeight: 64
                    background: Rectangle { color: parent.hovered ? Theme.surface2 : Theme.content; radius: Theme.radiusSmall; border.color: Theme.line }
                    contentItem: RowLayout {
                        Symbol { name: "folder"; tint: Theme.accent; Layout.leftMargin: 12 }
                        Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.fillWidth: true; elide: Text.ElideRight }
                        Symbol { name: "arrow"; tint: Theme.muted; Layout.rightMargin: 12 }
                    }
                    onClicked: hub.openProject(index)
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 16
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Tus herramientas"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold; Layout.fillWidth: true }
                ActionButton { text: qsTr("Ver todas"); variant: "quiet"; onClicked: hub.navigate("tools") }
            }
            GridLayout {
                Layout.fillWidth: true; columns: view.availableWidth>850 ? 3 : 2; columnSpacing: 16; rowSpacing: 16
                Repeater {
                    model: toolModel
                    delegate: ToolCard {
                        required property string name
                        Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 220
                        toolName: name
                    }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 16
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Almacenamiento"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold; Layout.fillWidth: true }
                Label { text: volumes.refreshing ? qsTr("Actualizando…") : qsTr("Unidades de este equipo"); color: Theme.muted; font.pixelSize: Theme.caption }
            }
            Flow {
                Layout.fillWidth: true; spacing: 24
                Repeater {
                    model: volumes.items
                    delegate: RowLayout {
                        required property var modelData
                        width: 220; height: 48; spacing: 12
                        Symbol { name: "disk"; tint: modelData.online ? Theme.muted : Theme.warning }
                        ColumnLayout {
                            spacing: 4; Layout.fillWidth: true
                            Label { text: modelData.label; textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.bodySize; elide: Text.ElideRight; Layout.fillWidth: true }
                            Label { text: modelData.online ? qsTr("%1 GB disponibles").arg((modelData.available/1e9).toFixed(1)) : qsTr("Unidad no disponible"); color: Theme.muted; font.pixelSize: Theme.caption }
                        }
                    }
                }
            }
        }
        Item { height: 8 }
    }
}
