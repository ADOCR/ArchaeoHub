import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

ColumnLayout {
    id: page; objectName: "transcriptionWorkspace"; spacing: 12
    readonly property var state: transcription.state
    readonly property bool available: transcription.ready&&!transcription.busy
    readonly property var models: state.models||[]
    readonly property var selectedModel: models[state.modelIndex||0]||{}
    Component.onCompleted: transcription.initialize()
    RowLayout {
        Layout.fillWidth: true; spacing: 12
        Symbol { name: "transcription"; tint: Theme.accent; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
        ColumnLayout { Layout.fillWidth: true; spacing: 4
            Label { text: qsTr("Transcripción"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
            Label { text: qsTr("Audio y video · modelos locales · texto, subtítulos y minutas"); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
        IconButton { symbol: "star"; hint: qsTr("Favorito"); onClicked: hub.toggleFavorite("workbench.transcription") }
    }
    GlassSurface {
        Layout.fillWidth: true; implicitHeight: toolbar.implicitHeight+20
        Flow { id: toolbar; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: qsTr("Añadir archivos…"); symbol: "plus"; enabled: page.available; onClicked: files.open() }
            ActionButton { text: qsTr("Añadir carpeta…"); symbol: "folder"; enabled: page.available; onClicked: inputFolder.open() }
            ActionButton { text: qsTr("Ver resultados"); enabled: !!page.state.resultFolder; onClicked: hub.openFile(hub.fileUrl(page.state.resultFolder)) }
            ActionButton { text: qsTr("Copiar texto"); enabled: !!page.state.transcript; onClicked: hub.copyText(tabs.currentIndex===2?(page.state.minutes||""):(page.state.transcript||"")) }
            ActionButton { text: qsTr("Resumir documentos…"); symbol: "document"; onClicked: hub.navigate("tool/workbench.document-summary") }
        }
    }
    ScrollView {
        id: settingsView; Layout.fillWidth: true; Layout.preferredHeight: Math.min(settingsContent.implicitHeight,Math.max(180,page.height*0.40)); contentWidth: availableWidth; contentHeight: settingsContent.implicitHeight; clip: true
        ColumnLayout { id: settingsContent; width: settingsView.availableWidth; spacing: 8
            RowLayout { Layout.fillWidth: true; spacing: 12
                ColumnLayout { Layout.fillWidth: true; Label { text: qsTr("Modelo"); color: Theme.body } ComboBox { Layout.fillWidth: true; model: page.models; textRole: "name"; currentIndex: page.state.modelIndex||0; enabled: page.available; Accessible.name: qsTr("Modelo de transcripción"); onActivated: transcription.settings({modelIndex:currentIndex}) } }
                ColumnLayout { Layout.preferredWidth: 145; Label { text: qsTr("Idioma"); color: Theme.body } ComboBox { Layout.fillWidth: true; model: [qsTr("Español"),qsTr("Automático"),qsTr("Inglés"),qsTr("Portugués"),qsTr("Francés")]; property var codes: ["es","auto","en","pt","fr"]; currentIndex: Math.max(0,codes.indexOf(page.state.language||"es")); enabled: page.available; Accessible.name: qsTr("Idioma del audio"); onActivated: transcription.settings({language:codes[currentIndex]}) } }
                ColumnLayout { Layout.preferredWidth: 190; Label { text: qsTr("Procesador"); color: Theme.body } ComboBox { Layout.fillWidth: true; model: [qsTr("Automático / GPU"),qsTr("CPU")]; currentIndex: page.state.device==="cpu"?1:0; enabled: page.available; Accessible.name: qsTr("Dispositivo de transcripción"); onActivated: transcription.settings({device:currentIndex===1?"cpu":"auto"}) } }
            }
            RowLayout { Layout.fillWidth: true
                Label { Layout.fillWidth: true; text: (page.selectedModel.ready?qsTr("Modelo disponible sin conexión. "):qsTr("Modelo pendiente de descarga. "))+(page.selectedModel.detail||""); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap }
                ActionButton { text: qsTr("Descargar modelo…"); visible: !page.selectedModel.ready; enabled: page.available; onClicked: downloadConsent.open() }
            }
            RowLayout { Layout.fillWidth: true
                CheckBox { text: qsTr("Generar minuta"); checked: !!page.state.createMinutes; enabled: page.available; palette.windowText: Theme.body; onClicked: transcription.settings({createMinutes:checked}) }
                AppField { Layout.fillWidth: true; text: page.state.ollamaModel||"qwen3:8b"; enabled: page.available&&!!page.state.createMinutes; Accessible.name: qsTr("Modelo de Ollama local para minutas"); onEditingFinished: transcription.settings({ollamaModel:text}) }
                ActionButton { text: qsTr("Carpeta de resultados…"); enabled: page.available; onClicked: outputFolder.open() }
            }
            Label { text: qsTr("Minutas: requieren Ollama en este equipo. Los archivos de audio y video se procesan localmente."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
            RowLayout { Layout.fillWidth: true
                Label { text: qsTr("Modelos: %1").arg(transcription.modelsFolder); textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; elide: Text.ElideMiddle; Layout.fillWidth: true }
                ActionButton { text: qsTr("Usar modelos existentes…"); enabled: page.available; onClicked: modelsFolder.open() }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { text: transcription.ready?(page.state.status||qsTr("Listo")):qsTr("Iniciando motor local…"); textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
        ActionButton { text: qsTr("Cancelar"); visible: transcription.busy; onClicked: transcription.cancel() }
        ActionButton { text: qsTr("Transcribir"); variant: "primary"; symbol: "arrow"; enabled: page.available&&!!page.selectedModel.ready&&(page.state.files||[]).length>0; onClicked: {tabs.currentIndex=1;transcription.start()} }
    }
    ProgressBar { visible: transcription.busy; Layout.fillWidth: true; value: page.state.downloading?(page.state.downloadProgress||0):(page.state.progress||0); indeterminate: value===0; Layout.preferredHeight: 4 }
    Label { visible: transcription.message.length>0; text: transcription.message; textFormat: Text.PlainText; color: Theme.error; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    TabBar { id: tabs; Layout.fillWidth: true
        TabButton { text: qsTr("Archivos (%1)").arg((page.state.files||[]).length) }
        TabButton { text: qsTr("Transcripción") }
        TabButton { text: qsTr("Minuta") }
        TabButton { text: qsTr("Historial") }
    }
    StackLayout {
        currentIndex: tabs.currentIndex; Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 100
        ListView { clip: true; model: page.state.files||[]; ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 54
                contentItem: RowLayout { Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.body; elide: Text.ElideMiddle; Layout.fillWidth: true } Label { text: modelData.size; color: Theme.muted; font.pixelSize: Theme.caption } IconButton { symbol: "close"; hint: qsTr("Quitar de la lista"); enabled: page.available; onClicked: transcription.removeFile(index) } }
            }
            Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Añade o arrastra audios y videos para empezar."); color: Theme.muted; wrapMode: Text.Wrap; width: Math.min(420,parent.width-24); horizontalAlignment: Text.AlignHCenter }
            DropArea { anchors.fill: parent; enabled: page.available; onDropped: drop=>{if(drop.hasUrls)transcription.addFiles(drop.urls)} }
        }
        ScrollView { clip: true; TextArea { text: page.state.transcript||""; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; color: Theme.body; font.pixelSize: Theme.bodySize; Accessible.name: qsTr("Transcripción generada"); background: Rectangle { color: Theme.content } } }
        ScrollView { clip: true; TextArea { text: page.state.minutes||qsTr("La minuta aparecerá después de transcribir con esa opción activada."); readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; color: Theme.body; font.pixelSize: Theme.bodySize; background: Rectangle { color: Theme.content } } }
        ListView { clip: true; model: page.state.history||[]; ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 64; enabled: page.available
                contentItem: ColumnLayout { Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.ink; elide: Text.ElideMiddle; Layout.fillWidth: true } Label { text: modelData.date+" · "+modelData.model; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption } }
                onClicked: {transcription.openHistory(index);tabs.currentIndex=1}
            }
        }
    }
    FileDialog { id: files; title: qsTr("Añadir audios y videos"); fileMode: FileDialog.OpenFiles; nameFilters: [qsTr("Audio y video (*.mp3 *.wav *.m4a *.ogg *.opus *.flac *.aac *.mp4 *.mkv *.mov *.webm *.wma)"),qsTr("Todos los archivos (*)")]; onAccepted: transcription.addFiles(selectedFiles) }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+O":"Ctrl+O"; enabled: page.available; onActivated: files.open() }
    FolderDialog { id: inputFolder; title: qsTr("Carpeta de audios y videos"); onAccepted: transcription.addFolder(selectedFolder) }
    FolderDialog { id: outputFolder; title: qsTr("Carpeta para resultados"); onAccepted: transcription.settings({outputFolder:hub.localPath(selectedFolder)}) }
    FolderDialog { id: modelsFolder; title: qsTr("Carpeta que contiene los modelos descargados"); onAccepted: transcription.useModelsFolder(selectedFolder) }
    Dialog { id: downloadConsent; parent: Overlay.overlay; anchors.centerIn: parent; implicitWidth: 560; implicitHeight: 260; width: Math.min(implicitWidth,parent.width-64); height: implicitHeight; modal: true; title: qsTr("Descargar modelo de terceros"); standardButtons: Dialog.Ok|Dialog.Cancel
        contentItem: Label { text: qsTr("Se descargará %1 desde su proveedor. Puede ocupar varios GB. Consulta las licencias incluidas en Ayuda. La descarga requiere Internet; tu audio y video permanecen en este equipo.").arg(page.selectedModel.name||""); color: Theme.body; wrapMode: Text.Wrap }
        onAccepted: transcription.downloadModel()
    }
}
