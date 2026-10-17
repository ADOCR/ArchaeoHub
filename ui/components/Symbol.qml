import QtQuick
import "../themes"
Item {
    id: symbolRoot
    property string name: "tools"
    property color tint: Theme.body
    implicitWidth: 20; implicitHeight: 20
    width: implicitWidth; height: implicitHeight
    Accessible.ignored: true
    Image {
        anchors.fill: parent
        sourceSize: Qt.size(64,64)
        source: "image://symbols/" + symbolRoot.name + "/" + symbolRoot.tint.toString().replace("#", "")
        fillMode: Image.PreserveAspectFit
        smooth: true
        Accessible.ignored: true
    }
}
