import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
ColumnLayout {
    id: state
    property string title: ""
    property string description: ""
    property string symbol: "folder"
    spacing: 12
    Symbol { name: state.symbol; tint: Theme.muted; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
    Label { text: state.title; color: Theme.ink; font.pixelSize: Theme.bodyLarge; font.weight: Font.DemiBold; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { text: state.description; color: Theme.muted; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
}
