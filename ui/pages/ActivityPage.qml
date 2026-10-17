import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
import "../components"
ColumnLayout {
    id: page
    signal hashRequested()
    property var taskItems: []
    Component.onCompleted: taskItems=tasks.items
    Connections { target: tasks; function onChanged(){if(!refresh.running)refresh.start()} }
    Timer { id: refresh; interval: 100; onTriggered: page.taskItems=tasks.items }
    spacing: 24
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Actividad"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold; Layout.fillWidth: true }
        ActionButton { text: qsTr("Verificar archivo"); symbol: "document"; onClicked: page.hashRequested() }
    }
    Label { text: qsTr("Aperturas de herramientas y tareas del Hub. La verificación calcula SHA-256 sin modificar el archivo."); color: Theme.muted; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
    ListView {
        id: list; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 1; model: page.taskItems; cacheBuffer: 0
        delegate: Rectangle {
            required property var modelData
            width: list.width; height: item.implicitHeight+32; color: Theme.content
            ColumnLayout {
                id: item; anchors.fill: parent; anchors.margins: 16; spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Symbol { name: modelData.state==="failed" ? "warning" : modelData.state==="succeeded" ? "check" : "activity"; tint: modelData.state==="failed" ? Theme.error : Theme.muted }
                    Label { text: modelData.kind==="tool.launch" ? qsTr("Abrir %1").arg(modelData.name) : modelData.kind==="file.hash" ? qsTr("Verificar %1").arg(modelData.name) : modelData.name; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodySize; elide: Text.ElideRight; Layout.fillWidth: true }
                    StatusBadge { text: hub.taskStateText(modelData.state); positive: modelData.state==="succeeded" }
                    ActionButton { visible: ["running","queued","cancelling"].indexOf(modelData.state)>=0; enabled: modelData.state!=="cancelling"; text: qsTr("Cancelar"); variant: "quiet"; onClicked: tasks.cancel(modelData.id) }
                }
                ProgressBar { visible: ["running","queued","cancelling"].indexOf(modelData.state)>=0; value: modelData.progress || 0; Layout.fillWidth: true; palette.highlight: Theme.accent }
                Label { visible: !!modelData.result; text: modelData.result || ""; textFormat: Text.PlainText; font.family: Theme.mono; font.pixelSize: Theme.caption; color: Theme.body; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                ActionButton { visible: !!modelData.result; text: modelData.kind==="file.hash" ? qsTr("Copiar huella") : qsTr("Copiar resultado"); variant: "quiet"; onClicked: hub.copyText(modelData.result) }
                Label { visible: !!modelData.error; textFormat: Text.PlainText; text: modelData.error==="file.unreadable" ? qsTr("No se pudo leer el archivo. Revisa su unidad y permisos.") : modelData.error==="file.read_failed" ? qsTr("Se interrumpió la lectura. El archivo se conserva.") : modelData.kind==="tool.launch" ? hub.statusText(modelData.error) : modelData.error || ""; color: Theme.error; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
            }
        }
        EmptyState { anchors.centerIn: parent; width: Math.min(400,parent.width-48); visible: list.count===0; symbol: "activity"; title: qsTr("Todo listo para trabajar"); description: qsTr("Aquí aparecerán las tareas del Hub. El procesamiento de una herramienta externa se consulta en su ventana.") }
        ScrollBar.vertical: ScrollBar {}
    }
}
