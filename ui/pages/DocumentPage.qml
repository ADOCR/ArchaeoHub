import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
import "../components"
ColumnLayout {
    id: page; objectName: "documentWorkspace"; spacing: 12
    property alias currentTab: modes.currentIndex
    RowLayout { Layout.fillWidth: true; spacing: 12
        Symbol { name: "document"; tint: Theme.accent; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
        ColumnLayout { Layout.fillWidth: true; spacing: 4
            Label { text: qsTr("Gestión documental"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
            Label { text: qsTr("Resumen local · biblioteca Markdown · conexiones para Obsidian"); color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true; wrapMode: Text.Wrap }
        }
        IconButton { symbol: "star"; hint: qsTr("Favorito"); onClicked: hub.toggleFavorite("workbench.document-summary") }
    }
    TabBar { id: modes; Layout.fillWidth: true
        TabButton { text: qsTr("Resumir documentos") }
        TabButton { text: qsTr("Biblioteca y Obsidian") }
        TabButton { text: qsTr("Convertir a Markdown") }
    }
    Loader { Layout.fillWidth: true; Layout.fillHeight: true; source: modes.currentIndex===2?"DocumentConversionPage.qml":modes.currentIndex===1?"DocumentLibraryPage.qml":"DocumentSummaryPage.qml" }
}
