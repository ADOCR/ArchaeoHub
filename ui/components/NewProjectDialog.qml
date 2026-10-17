import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
Dialog {
    id: dialog
    parent: Overlay.overlay; anchors.centerIn: parent
    width: Math.min(700,parent.width-48); height: Math.min(720,parent.height-48)
    modal: true; title: qsTr("Nuevo proyecto")
    property url parentFolder: ""
    onOpened: projectName.forceActiveFocus()
    background: GlassSurface { popupSurface: true }
    header: Label {
        text: dialog.title; textFormat: Text.PlainText; color: Theme.ink
        font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
        padding: 12; elide: Text.ElideRight
    }
    contentItem: ColumnLayout {
        spacing: 16
        Label { text: qsTr("Nombre del proyecto"); color: Theme.ink; font.pixelSize: Theme.bodySize }
        AppField { id: projectName; Layout.fillWidth: true; placeholderText: qsTr("Codigo del sitio_tipo de trabajo en sitio"); maximumLength: 120; Accessible.name: qsTr("Nombre del proyecto") }
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            ColumnLayout {
                Layout.fillWidth: true; spacing: 4
                Label { text: qsTr("Crear dentro de"); color: Theme.ink; font.pixelSize: Theme.bodySize }
                Label { text: dialog.parentFolder.toString()==="" ? qsTr("Selecciona la carpeta de destino") : hub.localPath(dialog.parentFolder); textFormat: Text.PlainText; color: Theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true; font.pixelSize: Theme.caption }
            }
            ActionButton { text: qsTr("Elegir carpeta…"); symbol: "folder"; onClicked: destination.open() }
        }
        Label { text: qsTr("Estructura arqueológica · %1 carpetas vacías").arg(hub.projectFolders.length); color: Theme.ink; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: Theme.surface1; radius: Theme.radiusSmall; border.color: Theme.line
            ListView {
                id: tree; anchors.fill: parent; anchors.margins: 12; clip: true; model: hub.projectFolders
                delegate: RowLayout {
                    required property string modelData
                    readonly property var segments: modelData.split("/")
                    width: tree.width; height: Math.round(29*Theme.scale); spacing: 8
                    Item { Layout.preferredWidth: (parent.segments.length-1)*20 }
                    Symbol { name: "folder"; tint: Theme.muted; Layout.preferredWidth: 16; Layout.preferredHeight: 16 }
                    Label { text: segments[segments.length-1]; textFormat: Text.PlainText; color: Theme.body; font.family: Theme.mono; font.pixelSize: Theme.caption; elide: Text.ElideRight; Layout.fillWidth: true }
                }
                ScrollBar.vertical: ScrollBar {}
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight; spacing: 12
            ActionButton { text: qsTr("Cancelar"); onClicked: dialog.close() }
            ActionButton { text: qsTr("Crear proyecto"); variant: "primary"; symbol: "plus"; enabled: projectName.text.trim()!=="" && dialog.parentFolder.toString()!=="" && !hub.creatingProject; onClicked: {if(hub.createProject(dialog.parentFolder,projectName.text))dialog.close()} }
        }
    }
    FolderDialog { id: destination; title: qsTr("Elegir ubicación del nuevo proyecto"); onAccepted: dialog.parentFolder=selectedFolder }
}
