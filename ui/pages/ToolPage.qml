import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"
ScrollView {
    id: view; clip: true; contentWidth: availableWidth
    property string toolId: hub.route.substring(5)
    property var info: { let revision=hub.revision; return hub.tool(toolId) }
    ColumnLayout {
        width: view.availableWidth; spacing: 24
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            Symbol { name: view.info.icon || "tools"; tint: Theme.accent; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
            Label { text: view.info.name || ""; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold; Layout.fillWidth: true; wrapMode: Text.Wrap }
            IconButton { symbol: "star"; hint: qsTr("Favorito"); selected: view.info.favorite || false; onClicked: hub.toggleFavorite(view.toolId) }
        }
        Label { text: view.info.description || ""; textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.bodyLarge; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: statusRow.implicitHeight+32; color: Theme.surface2; radius: Theme.radiusSmall
            RowLayout {
                id: statusRow; anchors.fill: parent; anchors.margins: 16; spacing: 12
                Symbol { name: view.info.availability==="ready" ? "check" : view.info.planned ? "clock" : "info"; tint: view.info.availability==="ready" ? Theme.accent : Theme.muted }
                Label { text: hub.statusText(view.info.availability || ""); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
                ActionButton { visible: view.info.availability==="ready"; variant: "primary"; symbol: "arrow"; text: qsTr("Abrir herramienta"); onClicked: hub.openTool(view.toolId) }
            }
        }
        ColumnLayout {
            visible: view.info.planned || false; Layout.fillWidth: true; spacing: 16
            Label { text: qsTr("Integración por etapas"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
            Label { text: view.toolId==="workbench.coordinates" ? qsTr("El módulo ampliará GD/GMS con CRS buscables, formatos regionales, conversión inversa y lotes. El repositorio original contiene documentación; su archivo Python está vacío. La implementación y sus pruebas geodésicas pertenecen a la próxima fase.") : qsTr("El catalogador existente seguirá funcionando por separado. Antes de integrarlo se elegirá la versión de referencia y se protegerán sus exportaciones, metadatos y recuperación de cambios."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Esta herramienta todavía no se ejecuta desde el Hub."); color: Theme.muted; font.pixelSize: Theme.bodySize }
        }
        ColumnLayout {
            visible: !view.info.planned; Layout.fillWidth: true; spacing: 16
            Label { text: qsTr("Instalación de la herramienta"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
            Label { text: qsTr("Selecciona la aplicación que ya utilizas. Sus modelos, configuración e historial permanecen en su instalación."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Ejecutable o script"); color: Theme.body; font.pixelSize: Theme.bodySize }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                AppField { id: program; Layout.fillWidth: true; text: view.info.program || view.info.previousPath || ""; Accessible.name: qsTr("Ruta del ejecutable o script"); font.family: Theme.mono; font.pixelSize: Theme.caption; placeholderText: qsTr("Selecciona el archivo de la herramienta") }
                ActionButton { text: qsTr("Elegir…"); onClicked: programDialog.open() }
            }
            Label { visible: view.info.runtime==="python" || view.info.runtime==="script"; text: qsTr("Intérprete del entorno"); color: Theme.body; font.pixelSize: Theme.bodySize }
            RowLayout {
                visible: view.info.runtime==="python" || view.info.runtime==="script"; Layout.fillWidth: true
                AppField { id: interpreter; Layout.fillWidth: true; text: view.info.interpreter || ""; Accessible.name: qsTr("Ruta del intérprete"); font.family: Theme.mono; font.pixelSize: Theme.caption }
                ActionButton { text: qsTr("Elegir…"); onClicked: interpreterDialog.open() }
            }
            ActionButton { text: qsTr("Revisar y guardar instalación"); enabled: program.text.length>0 && !volumes.refreshing; onClicked: hub.configureToolPaths(view.toolId,program.text,interpreter.text) }
            Label { text: qsTr("Guardar autoriza el ejecutable seleccionado. No se instalarán dependencias automáticamente."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
            Label { text: qsTr("Trabajo local, en su propia ventana"); color: Theme.ink; font.pixelSize: Theme.bodyLarge; font.weight: Font.DemiBold }
            Label { text: qsTr("El Hub registra la apertura. El progreso y los resultados del procesamiento se consultan dentro de la herramienta. Las descargas opcionales de modelos se gestionan allí; no se copian modelos dentro del Hub."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
        ActionButton { id: advanced; text: checked ? qsTr("Ocultar detalles") : qsTr("Detalles del módulo"); variant: "quiet"; checkable: true; symbol: "info" }
        ColumnLayout {
            visible: advanced.checked; Layout.fillWidth: true; spacing: 8
            Label { text: qsTr("Identificador: %1").arg(view.toolId); color: Theme.muted; font.family: Theme.mono; font.pixelSize: Theme.caption }
            Label { text: qsTr("Adaptador: %1 · Runtime: %2").arg(view.info.version || "").arg(view.info.runtime || ""); color: Theme.muted; font.pixelSize: Theme.caption }
            Label { text: view.info.manifest || ""; textFormat: Text.PlainText; color: Theme.muted; font.family: Theme.mono; font.pixelSize: Theme.caption; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
        }
        Item { height: 12 }
    }
    FileDialog { id: programDialog; title: qsTr("Seleccionar la herramienta"); fileMode: FileDialog.OpenFile; onAccepted: program.text=hub.localPath(selectedFile) }
    FileDialog { id: interpreterDialog; title: qsTr("Seleccionar el intérprete"); fileMode: FileDialog.OpenFile; onAccepted: interpreter.text=hub.localPath(selectedFile) }
}
