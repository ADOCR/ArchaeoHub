import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
Rectangle {
    id: card
    required property string toolId
    required property string toolName
    required property string description
    required property string iconName
    required property string availability
    required property bool favorite
    required property bool planned
    implicitHeight: content.implicitHeight + 40
    color: Theme.content; radius: Theme.radius; border.color: Theme.line
    ColumnLayout {
        id: content; anchors.fill: parent; anchors.margins: 20; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Rectangle { width: 38; height: 38; radius: Theme.radiusSmall; color: Theme.surface2; Symbol { name: card.iconName; tint: card.planned ? Theme.muted : Theme.accent; anchors.centerIn: parent; width: 22; height: 22 } }
            Item { Layout.fillWidth: true }
            IconButton { symbol: "star"; selected: card.favorite; hint: card.favorite ? qsTr("Quitar de favoritos") : qsTr("Agregar a favoritos"); onClicked: hub.toggleFavorite(card.toolId) }
        }
        Label { text: card.toolName; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodyLarge; font.weight: Font.DemiBold; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Label { text: card.description; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; maximumLineCount: 3; elide: Text.ElideRight; Layout.fillWidth: true; Layout.fillHeight: true }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            StatusBadge { text: card.planned ? qsTr("Planificado") : card.availability === "ready" ? qsTr("Disponible") : qsTr("Configurar"); positive: card.availability === "ready" }
            Item { Layout.fillWidth: true }
            ActionButton { variant: "quiet"; text: card.planned ? qsTr("Ver detalles") : card.availability === "ready" ? qsTr("Abrir") : qsTr("Configurar"); symbol: "arrow"; onClicked: card.availability === "ready" ? hub.openTool(card.toolId) : hub.navigate("tool/"+card.toolId) }
        }
    }
}
