import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
Popup {
    id: popup
    width: Math.min(640, parent.width-64)
    height: Math.min(480, parent.height*.65)
    x: (parent.width-width)/2; y: Math.max(40,parent.height*.14)
    modal: true; focus: true
    padding: 12
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: GlassSurface { popupSurface: true }
    Overlay.modal: Rectangle { color: Theme.scrim }
    onOpened: { search.text=""; search.forceActiveFocus(); results.currentIndex=0 }
    function activate() {
        if (results.currentIndex>=0 && results.currentIndex<results.model.length) {
            hub.navigate(results.model[results.currentIndex].route); popup.close()
        }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Symbol { name: "search"; tint: Theme.muted; Layout.leftMargin: 8 }
            AppField {
                id: search; Layout.fillWidth: true; placeholderText: qsTr("Buscar herramienta o acción…")
                Accessible.name: qsTr("Buscar comandos")
                onTextChanged: results.currentIndex=0
                Keys.onDownPressed: results.currentIndex=Math.min(results.count-1,results.currentIndex+1)
                Keys.onUpPressed: results.currentIndex=Math.max(0,results.currentIndex-1)
                Keys.onReturnPressed: popup.activate()
            }
            Label { text: "Esc"; color: Theme.muted; font.pixelSize: Theme.caption; Layout.rightMargin: 8 }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
        ListView {
            id: results; Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            model: Array.from(hub.commands).filter(function(c){return c.name.toLocaleLowerCase().indexOf(search.text.toLocaleLowerCase())>=0})
            spacing: 4
            delegate: ItemDelegate {
                required property var modelData
                required property int index
                width: results.width; height: 44
                background: Rectangle { radius: Theme.radiusSmall; color: results.currentIndex===index ? Theme.accentSoft : parent.hovered ? Theme.surface2 : "transparent" }
                contentItem: RowLayout {
                    spacing: 12
                    Symbol { name: modelData.icon; tint: results.currentIndex===index ? Theme.accent : Theme.muted }
                    Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.fillWidth: true; elide: Text.ElideRight }
                    Label { visible: results.currentIndex===index; text: "↵"; color: Theme.muted }
                }
                onClicked: { results.currentIndex=index; popup.activate() }
            }
            Label { anchors.centerIn: parent; visible: results.count===0; text: qsTr("No hay acciones que coincidan."); color: Theme.muted }
            ScrollBar.vertical: ScrollBar {}
        }
        Label { text: qsTr("↑ ↓ para elegir · Enter para abrir"); color: Theme.muted; font.pixelSize: Theme.caption; Layout.leftMargin: 8 }
    }
}
