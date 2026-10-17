import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
import "../components"
ColumnLayout {
    id: page
    spacing: 24
    property string mode: hub.route
    Label { text: page.mode==="favorites" ? qsTr("Favoritos") : page.mode==="recent" ? qsTr("Herramientas recientes") : qsTr("Herramientas"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
    Label { text: qsTr("Un espacio común para herramientas independientes."); color: Theme.muted; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
    AppField { id: query; placeholderText: qsTr("Buscar por nombre, descripción o categoría…"); Accessible.name: qsTr("Buscar herramientas"); Layout.fillWidth: true }
    Binding { target: toolFilter; property: "query"; value: query.text; restoreMode: Binding.RestoreNone }
    Binding { target: toolFilter; property: "mode"; value: page.mode; restoreMode: Binding.RestoreNone }
    ListView {
        id: list; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 12; cacheBuffer: 0
        model: toolFilter
        delegate: ToolCard { required property string name; width: list.width; toolName: name }
        EmptyState { anchors.centerIn: parent; width: Math.min(400,parent.width-48); visible: list.count===0; symbol: page.mode==="favorites" ? "star" : "tools"; title: qsTr("Todavía no hay herramientas aquí"); description: qsTr("Marca una herramienta como favorita, ábrela o ajusta la búsqueda.") }
        ScrollBar.vertical: ScrollBar {}
    }
}
