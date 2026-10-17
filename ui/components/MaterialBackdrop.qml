import QtQuick
import "../themes"

Rectangle {
    color: Theme.canvas
    // Iluminación estática del material, cubierta por el área de trabajo opaca.
    Rectangle {
        anchors.fill: parent; visible: !Theme.opaque
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.materialTop }
            GradientStop { position: .46; color: Theme.materialMiddle }
            GradientStop { position: 1; color: Theme.materialBottom }
        }
    }
    Rectangle {
        anchors.fill: parent; visible: !Theme.opaque
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: .48; color: Theme.materialHorizon }
            GradientStop { position: 1; color: Theme.canvas }
        }
    }
}
