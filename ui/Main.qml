import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "themes"
import "components"
import "pages"
ApplicationWindow {
    id: window
    width: 1280; height: 820; minimumWidth: 960; minimumHeight: 640
    visible: true; title: hub.productName
    color: Theme.canvas
    font.pixelSize: Theme.bodySize
    palette.window: Theme.canvas; palette.windowText: Theme.ink; palette.text: Theme.body
    palette.base: Theme.content; palette.button: Theme.surface2; palette.buttonText: Theme.body
    palette.highlight: Theme.accent; palette.highlightedText: Theme.accentText
    property bool compact: width<1120
    property bool toolsExpanded: true
    property bool closingAfterTasks: false
    function openCommandPalette(){commands.open()}
    function openNewProject(){newProject.open()}
    onClosing: function(close) { if(tasks.activeCount>0 || hub.creatingProject){close.accepted=false;closingAfterTasks=true;tasks.cancelAll()} }
    Connections { target: tasks; function onChanged(){if(window.closingAfterTasks && tasks.activeCount===0 && !hub.creatingProject)Qt.quit()} }
    Connections { target: hub; function onChanged(){if(window.closingAfterTasks && tasks.activeCount===0 && !hub.creatingProject)Qt.quit()} }
    Shortcut { sequence: Qt.platform.os==="osx" ? "Meta+K" : "Ctrl+K"; onActivated: commands.open() }
    Shortcut { sequence: Qt.platform.os==="osx" ? "Meta+," : "Ctrl+,"; onActivated: hub.navigate("settings") }
    Shortcut { sequence: Qt.platform.os==="osx" ? "Meta+O" : "Ctrl+O"; enabled: !["tool/workbench.photo-catalog","tool/workbench.transcription","tool/workbench.coordinates","tool/workbench.document-summary","tool/workbench.inventory"].includes(hub.route); onActivated: fileDialog.open() }
    Shortcut { sequence: "Alt+Left"; enabled: hub.canGoBack; onActivated: hub.goBack() }
    Binding { target: Theme; property: "materialSource"; value: materialBackdrop }
    Binding { target: Theme; property: "sceneSource"; value: workbenchScene }
    Binding { target: Theme; property: "graphicsEffectsAvailable"; value: [GraphicsInfo.OpenGL,GraphicsInfo.Direct3D11,GraphicsInfo.Direct3D12,GraphicsInfo.Vulkan,GraphicsInfo.Metal].includes(workbenchScene.GraphicsInfo.api) }
    Item {
        id: workbenchScene; anchors.fill: parent
        MaterialBackdrop { id: materialBackdrop; anchors.fill: parent }
    RowLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 0
        GlassSurface {
            Layout.preferredWidth: window.compact ? 72 : 248; Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 4
                RowLayout {
                    Layout.fillWidth: true; Layout.topMargin: 12; Layout.bottomMargin: 24; Layout.leftMargin: 10; spacing: 12
                    Symbol { name: "coordinates"; tint: Theme.accent; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
                    ColumnLayout {
                        visible: !window.compact; spacing: 3
                        Label { text: hub.productName; color: Theme.ink; font.pixelSize: Theme.bodyLarge; font.weight: Font.DemiBold }
                        Label { text: qsTr("Espacio de trabajo"); color: Theme.muted; font.pixelSize: Theme.caption }
                    }
                }
                ScrollView {
                    id: navigationView; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
                    ColumnLayout { width: navigationView.availableWidth; spacing: 4
                NavItem { Layout.fillWidth: true; text: qsTr("Inicio"); symbol: "home"; compact: window.compact; selected: hub.route==="home"; onClicked: hub.navigate("home") }
                NavItem { Layout.fillWidth: true; text: qsTr("Herramientas"); symbol: "tools"; compact: window.compact; selected: hub.route==="tools"; onClicked: { window.toolsExpanded=true; hub.navigate("tools") } }
                Repeater {
                    model: toolModel
                    delegate: NavItem {
                        required property string toolId
                        required property string name
                        required property string iconName
                        Layout.fillWidth: true; Layout.leftMargin: window.compact ? 0 : 12
                        visible: window.toolsExpanded; text: name; symbol: iconName; compact: window.compact
                        selected: hub.route==="tool/"+toolId; onClicked: hub.navigate("tool/"+toolId)
                    }
                }
                Item { height: 16 }
                NavItem { Layout.fillWidth: true; text: qsTr("Proyectos"); symbol: "folder"; compact: window.compact; selected: hub.route==="projects"; onClicked: hub.navigate("projects") }
                NavItem { Layout.fillWidth: true; text: qsTr("Favoritos"); symbol: "star"; compact: window.compact; selected: hub.route==="favorites"; onClicked: hub.navigate("favorites") }
                NavItem { Layout.fillWidth: true; text: qsTr("Recientes"); symbol: "clock"; compact: window.compact; selected: hub.route==="recent"; onClicked: hub.navigate("recent") }
                NavItem { Layout.fillWidth: true; text: tasks.activeCount>0 ? qsTr("Actividad · %1").arg(tasks.activeCount) : qsTr("Actividad"); symbol: "activity"; compact: window.compact; selected: hub.route==="activity"; onClicked: hub.navigate("activity") }
                    }
                }
                NavItem { Layout.fillWidth: true; text: qsTr("Configuración"); symbol: "settings"; compact: window.compact; selected: hub.route==="settings"; onClicked: hub.navigate("settings") }
                NavItem { Layout.fillWidth: true; text: qsTr("Ayuda y acerca de"); symbol: "info"; compact: window.compact; selected: hub.route==="about"; onClicked: hub.navigate("about") }
                RowLayout {
                    Layout.fillWidth: true; Layout.topMargin: 12
                    IconButton { symbol: "command"; hint: qsTr("Buscar acciones · Ctrl / ⌘ + K"); onClicked: commands.open() }
                    Label { visible: !window.compact; text: qsTr("Procesamiento local"); color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; color: Theme.canvas
        ColumnLayout {
            anchors.fill: parent; anchors.leftMargin: window.compact ? 24 : 32; anchors.rightMargin: 20; anchors.topMargin: 20; anchors.bottomMargin: 12
            spacing: 16
            Rectangle {
                visible: hub.notice!==""; Layout.fillWidth: true; implicitHeight: noticeRow.implicitHeight+24
                color: Theme.surface2; radius: Theme.radiusSmall; border.color: Theme.line
                RowLayout {
                    id: noticeRow; anchors.fill: parent; anchors.margins: 12; spacing: 12
                    Symbol { name: hub.noticeError ? "warning" : "check"; tint: hub.noticeError ? Theme.error : Theme.accent }
                    Label { text: hub.notice; textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    IconButton { symbol: "close"; hint: qsTr("Cerrar aviso"); onClicked: hub.dismissNotice() }
                }
            }
            Loader {
                id: page; Layout.fillWidth: true; Layout.fillHeight: true
                sourceComponent: hub.route==="home" ? home : hub.route.startsWith("tool/") ? detail : hub.route==="projects" ? projects : hub.route==="activity" ? activity : hub.route==="settings" ? settings : hub.route==="about" ? about : toolsPage
            }
        }
    }
    }
    }
    Component { id: home; HomePage { onAddProjectRequested: folderDialog.open(); onCreateProjectRequested: newProject.open(); onCommandRequested: commands.open() } }
    Component { id: detail; Loader {
        property string workspace: {let revision=hub.revision;return hub.tool(hub.route.substring(5)).workspace || ""}
        source: workspace==="catalog"?"pages/CatalogPage.qml":workspace==="coordinates"?"pages/CoordinatePage.qml":workspace==="transcription"?"pages/TranscriptionPage.qml":workspace==="inventory"?"pages/InventoryPage.qml":workspace==="documents"?"pages/DocumentPage.qml":"pages/ToolPage.qml"
    } }
    Component { id: toolsPage; ColumnLayout {
        ToolsPage { Layout.fillWidth: true; Layout.fillHeight: true }
        Label { visible: hub.route==="recent" && hub.recentFiles.length>0; text: qsTr("Archivos recientes"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
        ListView {
            visible: hub.route==="recent" && hub.recentFiles.length>0; Layout.fillWidth: true; Layout.preferredHeight: Math.min(160,hub.recentFiles.length*42); clip: true
            model: hub.recentFiles
            delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; height: 42; text: modelData.name; onClicked: hub.openRecentFile(index); palette.text: Theme.body }
        }
    } }
    Component { id: projects; ProjectsPage { onAddRequested: folderDialog.open(); onCreateRequested: newProject.open() } }
    Component { id: activity; ActivityPage { onHashRequested: hashDialog.open() } }
    Component { id: settings; SettingsPage { onImportRequested: manifestDialog.open() } }
    Component { id: about; ScrollView {
        id: aboutView
        contentWidth: availableWidth; clip: true
        ColumnLayout {
            width: aboutView.availableWidth; spacing: 24
            Label { text: hub.productName; color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
            Label { text: qsTr("Versión %1 · Herramientas arqueológicas independientes").arg(hub.version); color: Theme.muted; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Para comenzar"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
            Label { text: qsTr("1. Abre Transcripción, Catálogo o Coordenadas desde la barra lateral.\n2. Carga un catálogo existente, importa una tabla o añade tus audios.\n3. Vincula proyectos y marca tus herramientas favoritas."); color: Theme.body; font.pixelSize: Theme.bodyLarge; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Las herramientas se usan dentro del Hub. Sus motores se cargan sólo al utilizarlos. Los modelos de transcripción se conservan fuera de la instalación; los resúmenes de documentos usan Ollama local. Las descargas son opcionales."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
            ActionButton { text: qsTr("Licencias y componentes de terceros"); onClicked: hub.openLicenses() }
            ActionButton { text: qsTr("Abrir archivo…"); symbol: "document"; onClicked: fileDialog.open() }
            Label { text: qsTr("Ctrl / ⌘ + O · Abrir archivo\nCtrl / ⌘ + , · Configuración\nAlt + ← · Volver\nEscape · Cerrar un panel flotante"); color: Theme.muted; font.family: Theme.mono; font.pixelSize: Theme.caption }
        }
    } }
    CommandPalette { id: commands; parent: Overlay.overlay }
    NewProjectDialog { id: newProject }
    FolderDialog { id: folderDialog; title: qsTr("Vincular carpeta de proyecto"); onAccepted: hub.addProject(selectedFolder) }
    FileDialog { id: fileDialog; title: qsTr("Abrir archivo con su aplicación asociada"); fileMode: FileDialog.OpenFile; onAccepted: hub.openFile(selectedFile) }
    FileDialog { id: hashDialog; title: qsTr("Verificar integridad de un archivo"); fileMode: FileDialog.OpenFile; onAccepted: hub.hashFile(selectedFile) }
    FileDialog { id: manifestDialog; title: qsTr("Revisar manifiesto de herramienta"); nameFilters: [qsTr("Manifiesto JSON (*.json)")]; fileMode: FileDialog.OpenFile; onAccepted: hub.reviewManifest(selectedFile) }
    Dialog {
        id: review; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(560,parent.width-64); modal: true
        title: qsTr("Revisar herramienta")
        visible: Object.keys(hub.pendingManifest).length>0
        closePolicy: Popup.NoAutoClose
        background: Rectangle { color: Theme.content; radius: Theme.radiusPanel; border.color: Theme.strongLine }
        contentItem: ColumnLayout {
            spacing: 16
            Label { text: hub.pendingManifest.name || ""; textFormat: Text.PlainText; font.pixelSize: Theme.h2; color: Theme.ink; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: hub.pendingManifest.description || ""; textFormat: Text.PlainText; color: Theme.body; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Runtime: %1").arg(hub.pendingManifest.runtime || ""); color: Theme.muted }
            Label { text: qsTr("Registrar guarda los metadatos. Aún deberás elegir y autorizar el ejecutable; no se ejecutará código al agregarlo."); color: Theme.body; wrapMode: Text.Wrap; Layout.fillWidth: true }
            RowLayout { Layout.alignment: Qt.AlignRight; ActionButton { text: qsTr("Cancelar"); onClicked: hub.dismissManifest() } ActionButton { text: qsTr("Registrar"); variant: "primary"; onClicked: hub.approveManifest() } }
        }
    }
}
