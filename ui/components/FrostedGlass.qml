import QtQuick
import QtQuick.Effects
import "../themes"

Item {
    id: fx
    property Item surface: null
    readonly property int padding: 28
    readonly property real fieldWidth: width+2*padding
    readonly property real fieldHeight: height+2*padding
    function refresh(){capture.scheduleUpdate()}
    Connections {
        target: Theme
        function onDarkChanged(){Qt.callLater(fx.refresh)}
    }
    // La navegación toma sólo el fondo del material. Los paneles flotantes capturan el área de trabajo,
    // que excluye Overlay; ninguna ruta captura su propio efecto de forma recursiva.
    ShaderEffectSource {
        id: capture; visible: false; sourceItem: fx.surface ? fx.surface.backdrop : null
        sourceRect: fx.surface ? Qt.rect(fx.surface.sampleRect.x-fx.padding,fx.surface.sampleRect.y-fx.padding,fx.fieldWidth,fx.fieldHeight) : Qt.rect(0,0,1,1)
        textureSize: Qt.size(Math.max(1,Math.ceil(fx.fieldWidth*.5)),Math.max(1,Math.ceil(fx.fieldHeight*.5)))
        hideSource: false; recursive: false; live: fx.surface ? !fx.surface.popupSurface : false
        onSourceRectChanged: scheduleUpdate()
        Component.onCompleted: scheduleUpdate()
    }
    Item {
        id: shape; width: fx.fieldWidth; height: fx.fieldHeight
        Rectangle { x: fx.padding; y: fx.padding; width: fx.width; height: fx.height; radius: fx.surface ? fx.surface.radius : 0; color: "white"; antialiasing: true }
    }
    ShaderEffectSource { id: mask; sourceItem: shape; hideSource: true; visible: false; live: true; recursive: false }
    MultiEffect {
        x: -fx.padding; y: -fx.padding; width: fx.fieldWidth; height: fx.fieldHeight
        source: mask; autoPaddingEnabled: false
        shadowEnabled: true; shadowColor: Theme.glassShadow; shadowOpacity: 1
        shadowBlur: .65; blurMax: 24; shadowVerticalOffset: 7
    }
    MultiEffect {
        x: -fx.padding; y: -fx.padding; width: fx.fieldWidth; height: fx.fieldHeight
        source: capture; autoPaddingEnabled: false
        blurEnabled: true; blurMax: 32; blur: 1; blurMultiplier: 1
        maskEnabled: true; maskSource: mask
    }
}
