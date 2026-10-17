import QtQuick
import QtQuick.Window
import "../themes"
Rectangle {
    id: glass; objectName: "glassSurface"
    readonly property bool isGlassSurface: true
    property bool popupSurface: false
    property Item backdrop: popupSurface ? Theme.sceneSource : Theme.materialSource
    readonly property bool wantsFrosting: !Theme.opaque && Theme.graphicsEffectsAvailable && backdrop!==null && visible && width>0 && height>0
    readonly property bool frosted: wantsFrosting && effect.status===Loader.Ready
    readonly property bool effectReady: effect.status===Loader.Ready
    readonly property bool opaqueFallback: !frosted
    property rect sampleRect: Qt.rect(0,0,width,height)
    function resample() {
        if(!backdrop){sampleRect=Qt.rect(0,0,width,height);return}
        const p=mapToItem(backdrop,0,0)
        sampleRect=Qt.rect(p.x,p.y,width,height)
    }
    // Centrar el panel puede mover un ancestro sin cambiar las coordenadas x/y de este elemento.
    // Espera a que termine la distribución antes de capturar la nueva región visible de la escena.
    onXChanged: Qt.callLater(resample)
    onYChanged: Qt.callLater(resample)
    onWidthChanged: Qt.callLater(resample)
    onHeightChanged: Qt.callLater(resample)
    onBackdropChanged: Qt.callLater(resample)
    onVisibleChanged: Qt.callLater(resample)
    Component.onCompleted: Qt.callLater(resample)
    Connections {
        target: glass.parent
        function onXChanged(){Qt.callLater(glass.resample)}
        function onYChanged(){Qt.callLater(glass.resample)}
    }
    Connections {
        target: glass.Window.window
        function onWidthChanged(){Qt.callLater(glass.resample)}
        function onHeightChanged(){Qt.callLater(glass.resample)}
    }
    color: "transparent"; radius: Theme.radiusGlass
    Loader {
        id: effect; anchors.fill: parent; active: glass.wantsFrosting
        source: "FrostedGlass.qml"
        onLoaded: item.surface=glass
    }
    Rectangle { anchors.fill: parent; radius: glass.radius; color: glass.frosted ? Theme.glassTint : Theme.surface1 }
    Rectangle {
        anchors.fill: parent; radius: glass.radius; visible: !Theme.opaque
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.glassReflection }
            GradientStop { position: .30; color: Theme.glassReflectionEnd }
            GradientStop { position: .72; color: "transparent" }
            GradientStop { position: 1; color: Theme.glassShade }
        }
    }
    Rectangle {
        anchors.fill: parent; radius: glass.radius; color: "transparent"
        border.color: Theme.opaque ? Theme.strongLine : Theme.glassRim; border.width: 1
    }
    Rectangle {
        anchors.fill: parent; anchors.margins: 1; radius: Math.max(0,glass.radius-1)
        color: "transparent"; border.color: Theme.glassInnerRim; border.width: 1
    }
}
