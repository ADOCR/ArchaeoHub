import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

ColumnLayout {
    id: page; objectName: "documentLibraryWorkspace"; spacing: 8
    property string previewText: ""
    function showPreview(kind,index) { page.previewText=documentLibrary.preview(kind,index); previewDialog.open() }
    Component.onCompleted: { if(!documents.busy) documents.initialize() }
    GlassSurface { Layout.fillWidth: true; implicitHeight: bar.implicitHeight+20
        Flow { id: bar; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: qsTr("Añadir archivos…"); symbol: "plus"; enabled: !documentLibrary.busy; onClicked: inputFiles.open() }
            ActionButton { text: qsTr("Añadir carpeta…"); symbol: "folder"; enabled: !documentLibrary.busy; onClicked: inputFolder.open() }
            ActionButton { text: qsTr("Analizar lote"); enabled: !documentLibrary.busy&&documentLibrary.inputs.length>0; onClicked: documentLibrary.analyze() }
            ActionButton { text: qsTr("Abrir biblioteca"); enabled: documentLibrary.resultFolder.length>0; onClicked: hub.openFolder(hub.fileUrl(documentLibrary.resultFolder)) }
            ActionButton { text: qsTr("Cancelar"); visible: documentLibrary.busy; onClicked: documentLibrary.cancel() }
        }
    }
    Label { text: qsTr("Añade documentos, analiza el lote y prepara sus fichas. Revisa resúmenes y evidencias antes de exportar a Obsidian."); color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    ListView { Layout.fillWidth: true; Layout.preferredHeight: count?Math.min(70,count*44):42; clip: true; model: documentLibrary.inputs; ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 44
            contentItem: RowLayout { Symbol { name: modelData.directory?"folder":"document"; tint: Theme.muted } Label { text: modelData.path; textFormat: Text.PlainText; color: Theme.body; elide: Text.ElideMiddle; Layout.fillWidth: true; font.pixelSize: Theme.caption } IconButton { symbol: "close"; hint: qsTr("Quitar ubicación del lote"); enabled: !documentLibrary.busy; onClicked: documentLibrary.removeInput(index) } }
        }
        Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Añade o arrastra documentos y carpetas."); color: Theme.muted }
        DropArea { anchors.fill: parent; enabled: !documentLibrary.busy; onDropped: drop=>{if(drop.hasUrls)documentLibrary.addInputs(drop.urls)} }
    }
    Flow { Layout.fillWidth: true; spacing: 8
        Label { text: qsTr("Modelo local"); color: Theme.body; height: modelPicker.height; verticalAlignment: Text.AlignVCenter }
        ComboBox { id: modelPicker; width: Math.min(230,page.width-24); model: documents.models; currentIndex: Math.max(0,documents.models.indexOf(documents.model)); enabled: !documentLibrary.busy&&!documents.busy; Accessible.name: qsTr("Modelo para fichas y conceptos"); onActivated: documents.model=currentText }
        ActionButton { text: qsTr("Actualizar modelos"); enabled: !documentLibrary.busy&&!documents.busy; onClicked: documents.initialize() }
        ActionButton { text: qsTr("Crear fichas y conexiones"); variant: "primary"; enabled: documentLibrary.hasPlan&&!documentLibrary.busy&&!documents.busy&&documents.models.length>0; onClicked: documentLibrary.enrich(documents.model) }
    }
    Label { visible: documents.models.length===0; text: documents.message||qsTr("Inicia Ollama local y pulsa Actualizar modelos para preparar las fichas."); textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { text: qsTr("Carpeta de destino"); color: Theme.body; font.pixelSize: Theme.caption }
    RowLayout { Layout.fillWidth: true
        AppField { Layout.fillWidth: true; readOnly: true; text: documentLibrary.destination; placeholderText: qsTr("Elige un destino fuera de las carpetas de origen"); Accessible.name: qsTr("Destino de la biblioteca Markdown") }
        ActionButton { text: qsTr("Elegir…"); enabled: !documentLibrary.busy; onClicked: destination.open() }
        ActionButton { text: qsTr("Exportar biblioteca"); symbol: "arrow"; enabled: documentLibrary.hasAnalysis&&!documentLibrary.busy&&documentLibrary.destination.length>0; onClicked: documentLibrary.exportLibrary(hub.fileUrl(documentLibrary.destination),automaticLinks.checked) }
    }
    Flow { Layout.fillWidth: true; spacing: 8
        CheckBox { id: automaticLinks; checked: true; text: qsTr("Incluir conceptos y conexiones con evidencia"); enabled: !documentLibrary.busy&&documentLibrary.hasAnalysis; palette.windowText: Theme.body; font.pixelSize: Theme.caption }
    }
    Label { text: qsTr("Procesamiento local; los libros largos pueden tardar. Revisa las interpretaciones y sus citas antes de exportar. Los escaneos necesitan OCR."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    ProgressBar { Layout.fillWidth: true; visible: documentLibrary.busy; value: documentLibrary.progress; indeterminate: value===0; Layout.preferredHeight: 4 }
    Label { text: documentLibrary.message; textFormat: Text.PlainText; color: documentLibrary.error?Theme.error:Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    TabBar { id: results; Layout.fillWidth: true
        TabButton { text: qsTr("Documentos (%1)").arg(documentLibrary.rows.length) }
        TabButton { text: qsTr("Conceptos (%1)").arg(documentLibrary.hasAnalysis?documentLibrary.topics.length:0) }
        TabButton { text: qsTr("Conexiones (%1)").arg(documentLibrary.connections.length) }
    }
    StackLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 80; currentIndex: results.currentIndex
        ListView { clip: true; model: documentLibrary.rows; ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 72
                onClicked: { page.previewText=documentLibrary.preview("documents",index); previewDialog.open() }
                contentItem: ColumnLayout { spacing: 2
                    RowLayout { Layout.fillWidth: true; Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.ink; elide: Text.ElideMiddle; Layout.fillWidth: true } Label { text: modelData.status==="ready"?qsTr("Convertible · %1 caracteres").arg(modelData.characters):modelData.status==="needs-ocr"?qsTr("Necesita OCR"):qsTr("Pendiente"); color: modelData.status==="ready"?Theme.muted:Theme.warning; font.pixelSize: Theme.caption } }
                    Label { text: modelData.path; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true; elide: Text.ElideMiddle }
                    Label { text: modelData.message||qsTr("Se creará una nota con referencia al original."); textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true; elide: Text.ElideRight }
                }
            }
            Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Analiza el lote para revisar los archivos antes de exportar."); color: Theme.muted; width: parent.width-24; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter }
        }
        ListView { clip: true; model: documentLibrary.hasAnalysis?documentLibrary.topics:[]; ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 62
                onClicked: { page.previewText=documentLibrary.preview("concepts",index); previewDialog.open() }
                contentItem: ColumnLayout { Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.body; Layout.fillWidth: true } Label { text: modelData.definition; textFormat: Text.PlainText; elide: Text.ElideRight; Layout.fillWidth: true; color: Theme.muted; font.pixelSize: Theme.caption } }
            }
            Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Prepara las fichas para extraer conceptos definidos y respaldados por citas."); width: parent.width-24; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: Theme.muted }
        }
        ListView { clip: true; model: documentLibrary.connections; ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 66
                onClicked: { page.previewText=documentLibrary.preview("connections",index); previewDialog.open() }
                contentItem: ColumnLayout { Label { text: modelData.scope==="cross-document"?qsTr("Entre documentos · propuesta"):qsTr("Entre conceptos · propuesta"); color: Theme.accent; font.pixelSize: Theme.caption } Label { text: modelData.explanation; textFormat: Text.PlainText; elide: Text.ElideRight; Layout.fillWidth: true; color: Theme.body } }
            }
            Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Aquí aparecerán conexiones explicadas con sus evidencias. Si no hay respaldo suficiente, no se creará un enlace."); width: parent.width-24; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: Theme.muted }
        }
    }
    Dialog { id: previewDialog; title: qsTr("Revisar ficha y evidencias"); modal: true; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(850,parent.width-32); height: Math.min(650,parent.height-32); standardButtons: Dialog.Close
        contentItem: ScrollView { clip: true
            TextArea { text: page.previewText; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: Theme.body; font.pixelSize: Theme.bodySize; Accessible.name: qsTr("Contenido y citas del análisis"); background: Rectangle { color: Theme.content; radius: Theme.radiusSmall; border.color: Theme.line } }
        }
    }
    FileDialog { id: inputFiles; title: qsTr("Añadir documentos al lote"); fileMode: FileDialog.OpenFiles; nameFilters: [qsTr("Documentos (*.pdf *.md *.markdown *.docx)")]; onAccepted: documentLibrary.addInputs(selectedFiles) }
    FolderDialog { id: inputFolder; title: qsTr("Carpeta de documentos para analizar"); onAccepted: documentLibrary.addInputs([selectedFolder]) }
    FolderDialog { id: destination; title: qsTr("Destino de la biblioteca; cada lote creará una subcarpeta nueva"); onAccepted: documentLibrary.destination=hub.localPath(selectedFolder) }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+O":"Ctrl+O"; enabled: !documentLibrary.busy; onActivated: inputFiles.open() }
}
