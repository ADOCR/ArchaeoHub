import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

ColumnLayout {
    id: page; objectName: "documentSummaryWorkspace"; spacing: 12
    Component.onCompleted: documents.initialize()
    GlassSurface { Layout.fillWidth: true; implicitHeight: bar.implicitHeight+20
        Flow { id: bar; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: qsTr("Añadir documentos…"); symbol: "plus"; enabled: !documents.busy; onClicked: inputFiles.open() }
            ActionButton { text: qsTr("Actualizar modelos"); enabled: !documents.busy; onClicked: documents.initialize() }
            ActionButton { text: qsTr("Copiar resumen"); enabled: documents.summary.length>0; onClicked: hub.copyText(documents.summary) }
            ActionButton { text: qsTr("Exportar resumen…"); enabled: !documents.busy&&documents.summary.length>0; onClicked: exportFile.open() }
        }
    }
    Flow { Layout.fillWidth: true; spacing: 8
        Label { text: qsTr("Modelo local"); color: Theme.body; height: modelPicker.height; verticalAlignment: Text.AlignVCenter }
        ComboBox { id: modelPicker; width: Math.min(280,page.width-24); model: documents.models; currentIndex: Math.max(0,documents.models.indexOf(documents.model)); enabled: !documents.busy; Accessible.name: qsTr("Modelo local para resumir documentos"); onActivated: documents.model=currentText }
        CheckBox { id: detailed; text: qsTr("Resumen detallado"); enabled: !documents.busy; palette.windowText: Theme.body }
        ActionButton { text: qsTr("Generar resumen"); symbol: "arrow"; variant: "primary"; enabled: !documents.busy&&!documentLibrary.busy&&documents.files.length>0&&documents.models.length>0; onClicked: documents.generate(detailed.checked) }
        ActionButton { text: qsTr("Cancelar"); visible: documents.busy; onClicked: documents.cancel() }
    }
    Label { text: qsTr("Se utiliza Ollama en este equipo, sin enviar documentos a servicios externos. Admite .pdf con texto, .md y .docx; no ejecuta macros ni calcula fórmulas. Los PDF escaneados necesitan OCR."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { text: qsTr("Libros largos: se resumen por secciones y después se consolidan. Hasta 2 millones de caracteres por documento y 4 millones por lote; puede tardar varios minutos. Puedes cancelar sin perder el resumen anterior."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    ListView { Layout.fillWidth: true; Layout.preferredHeight: Math.min(150,Math.max(56,count*44)); clip: true; model: documents.files; ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 44
            contentItem: RowLayout { Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.body; elide: Text.ElideMiddle; Layout.fillWidth: true } IconButton { symbol: "arrow"; hint: qsTr("Abrir documento original"); onClicked: hub.openFile(hub.fileUrl(modelData.path)) } IconButton { symbol: "close"; hint: qsTr("Quitar de la lista"); enabled: !documents.busy; onClicked: documents.removeFile(index) } }
        }
        Label { visible: parent.count===0; anchors.centerIn: parent; text: qsTr("Añade o arrastra documentos para empezar."); color: Theme.muted }
        DropArea { anchors.fill: parent; enabled: !documents.busy; onDropped: drop=>{if(drop.hasUrls)documents.addFiles(drop.urls)} }
    }
    ProgressBar { visible: documents.busy; Layout.fillWidth: true; value: documents.progress; indeterminate: value===0; Layout.preferredHeight: 4 }
    Label { text: documents.message; textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { visible: documents.warnings.length>0; text: qsTr("Lectura parcial: %1").arg(documents.warnings.join(" · ")); textFormat: Text.PlainText; color: Theme.warning; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true; maximumLineCount: 3; elide: Text.ElideRight }
    ScrollView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        TextArea { text: documents.summary; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: Theme.body; placeholderTextColor: Theme.muted; font.pixelSize: Theme.bodySize; placeholderText: qsTr("El resumen aparecerá aquí. El contenido generado debe revisarse con los documentos de origen."); Accessible.name: qsTr("Resumen de documentos generado"); background: Rectangle { color: Theme.content; radius: Theme.radiusSmall; border.color: Theme.line } }
    }
    FileDialog { id: inputFiles; title: qsTr("Añadir documentos para resumir"); fileMode: FileDialog.OpenFiles; nameFilters: [qsTr("Documentos (*.pdf *.md *.markdown *.docx)")]; onAccepted: documents.addFiles(selectedFiles) }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+O":"Ctrl+O"; enabled: !documents.busy; onActivated: inputFiles.open() }
    FileDialog { id: exportFile; title: qsTr("Exportar resumen local"); fileMode: FileDialog.SaveFile; nameFilters: [qsTr("Markdown (*.md)"),qsTr("Texto (*.txt)")]; defaultSuffix: selectedNameFilter.index===1?"txt":"md"; onAccepted: documents.exportSummary(selectedFile) }
}
