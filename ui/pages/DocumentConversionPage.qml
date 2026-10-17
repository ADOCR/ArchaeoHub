import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

ColumnLayout {
    id: page; objectName: "documentConversionWorkspace"; spacing: 12
    GlassSurface { Layout.fillWidth: true; implicitHeight: bar.implicitHeight+20
        Flow { id: bar; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: qsTr("Añadir PDF o Word…"); symbol: "plus"; enabled: !documentConversion.busy; onClicked: files.open() }
            ActionButton { text: qsTr("Añadir carpeta…"); symbol: "folder"; enabled: !documentConversion.busy; onClicked: folder.open() }
            ActionButton { text: qsTr("Revisar lista"); enabled: !documentConversion.busy&&documentConversion.inputs.length>0; onClicked: documentConversion.analyze() }
            ActionButton { text: qsTr("Abrir resultado"); enabled: documentConversion.resultFolder.length>0; onClicked: hub.openFolder(hub.fileUrl(documentConversion.resultFolder)) }
            ActionButton { text: qsTr("Cancelar"); visible: documentConversion.busy; onClicked: documentConversion.cancel() }
        }
    }
    Label { text: qsTr("Convierte PDF con texto y Word (.docx) a archivos .md, individualmente o por lotes. Esta sección no requiere modelos de IA ni genera resúmenes o conexiones."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
    ListView { Layout.fillWidth: true; Layout.preferredHeight: Math.max(60,Math.min(130,count*44)); model: documentConversion.inputs; clip: true; ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 44
            contentItem: RowLayout { Label { text: modelData.path; textFormat: Text.PlainText; elide: Text.ElideMiddle; color: Theme.body; Layout.fillWidth: true } IconButton { symbol: "close"; hint: qsTr("Quitar de la lista"); enabled: !documentConversion.busy; onClicked: documentConversion.removeInput(index) } }
        }
        Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Añade o arrastra archivos PDF, Word o carpetas."); color: Theme.muted }
        DropArea { anchors.fill: parent; enabled: !documentConversion.busy; onDropped: drop=>{ if(drop.hasUrls)documentConversion.addInputs(drop.urls) } }
    }
    Label { text: qsTr("Carpeta de destino"); color: Theme.body; font.pixelSize: Theme.caption }
    RowLayout { Layout.fillWidth: true
        AppField { Layout.fillWidth: true; text: documentConversion.destination; readOnly: true; placeholderText: qsTr("Elige dónde guardar los Markdown"); Accessible.name: qsTr("Destino de la conversión a Markdown") }
        ActionButton { text: qsTr("Elegir…"); enabled: !documentConversion.busy; onClicked: destination.open() }
    }
    Flow { Layout.fillWidth: true; spacing: 8
        ActionButton { text: qsTr("Convertir a Markdown"); variant: "primary"; enabled: documentConversion.hasPlan&&!documentConversion.busy&&documentConversion.destination.length>0; onClicked: documentConversion.exportLibrary(hub.fileUrl(documentConversion.destination),false) }
        Label { text: qsTr("Primero revisa la lista. Cada conversión crea una carpeta nueva, sin reemplazar archivos."); color: Theme.muted; font.pixelSize: Theme.caption; width: Math.min(540,page.width); wrapMode: Text.Wrap }
    }
    ProgressBar { Layout.fillWidth: true; visible: documentConversion.busy; value: documentConversion.progress; indeterminate: value===0 }
    Label { text: documentConversion.message; textFormat: Text.PlainText; color: documentConversion.error?Theme.error:Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    ListView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 80; model: documentConversion.rows; clip: true; ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate { required property var modelData; width: ListView.view.width; height: 64
            contentItem: ColumnLayout { Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.ink; elide: Text.ElideMiddle; Layout.fillWidth: true } Label { text: modelData.status==="ready"?qsTr("Listo · %1 caracteres").arg(modelData.characters):modelData.message; textFormat: Text.PlainText; color: modelData.status==="ready"?Theme.muted:Theme.warning; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: Theme.caption } }
        }
    }
    Label { text: qsTr("Conserva texto extraíble, localizadores y referencia al original. No reproduce exactamente el diseño de Word ni tablas complejas. Los PDF escaneados necesitan OCR; los Word antiguos .doc deben convertirse antes a .docx."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    FileDialog { id: files; title: qsTr("PDF y Word para convertir"); fileMode: FileDialog.OpenFiles; nameFilters: [qsTr("PDF y Word (*.pdf *.docx)")]; onAccepted: documentConversion.addInputs(selectedFiles) }
    FolderDialog { id: folder; title: qsTr("Carpeta con PDF y Word"); onAccepted: documentConversion.addInputs([selectedFolder]) }
    FolderDialog { id: destination; title: qsTr("Destino de los Markdown"); onAccepted: documentConversion.destination=hub.localPath(selectedFolder) }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+O":"Ctrl+O"; enabled: !documentConversion.busy; onActivated: files.open() }
}
