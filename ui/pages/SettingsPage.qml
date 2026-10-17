import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../themes"
import "../components"
ScrollView {
    id: view
    signal importRequested()
    clip: true; contentWidth: availableWidth
    ColumnLayout {
        width: view.availableWidth; spacing: 24
        Label { text: qsTr("Configuración"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
        Label { text: qsTr("Apariencia"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
        RowLayout {
            Layout.fillWidth: true; spacing: 24
            Label { text: qsTr("Tema"); color: Theme.body; font.pixelSize: Theme.bodySize; Layout.fillWidth: true }
            ComboBox {
                implicitWidth: 190; implicitHeight: Theme.controlHeight
                model: [qsTr("Según el sistema"),qsTr("Claro"),qsTr("Oscuro")]
                currentIndex: preferences.theme==="dark" ? 2 : preferences.theme==="light" ? 1 : 0
                onActivated: preferences.theme=["auto","light","dark"][currentIndex]
                Accessible.name: qsTr("Tema de la aplicación")
            }
        }
        CheckBox { text: qsTr("Reducir transparencia"); checked: preferences.reduceTransparency; onToggled: preferences.reduceTransparency=checked; font.pixelSize: Theme.bodySize }
        Label {
            text: Theme.opaque ? qsTr("Las superficies opacas están activadas por tus ajustes de accesibilidad.") : Theme.graphicsEffectsAvailable ? qsTr("Vidrio esmerilado: difusión del fondo, reflejos suaves y profundidad en la navegación y los paneles flotantes.") : qsTr("Se usan superficies opacas para mantener la legibilidad en este sistema.")
            color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true
        }
        CheckBox { text: qsTr("Reducir movimiento"); checked: preferences.reduceMotion; onToggled: preferences.reduceMotion=checked; font.pixelSize: Theme.bodySize }
        CheckBox { text: qsTr("Reforzar contraste y superficies opacas"); checked: preferences.highContrast; onToggled: preferences.highContrast=checked; font.pixelSize: Theme.bodySize }
        RowLayout {
            Layout.fillWidth: true; spacing: 24
            Label { text: qsTr("Tamaño del texto"); color: Theme.body; font.pixelSize: Theme.bodySize; Layout.fillWidth: true }
            Slider { from: .9; to: 1.4; stepSize: .05; value: preferences.textScale; onMoved: preferences.textScale=value; implicitWidth: 200; Accessible.name: qsTr("Escala de texto") }
            Label { text: Math.round(preferences.textScale*100)+" %"; color: Theme.body; font.pixelSize: Theme.bodySize; Layout.preferredWidth: 48 }
        }
        Rectangle { height: 1; color: Theme.line; Layout.fillWidth: true }
        Label { text: qsTr("Herramientas y dependencias"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
        Label { text: qsTr("Cada herramienta conserva su entorno. El Hub comprueba las rutas antes de abrirla y no instala componentes automáticamente."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
        RowLayout {
            spacing: 12
            ActionButton { text: qsTr("Configurar herramientas"); onClicked: hub.navigate("tools") }
            ActionButton { text: qsTr("Agregar manifiesto…"); symbol: "plus"; onClicked: view.importRequested() }
        }
        Label { text: qsTr("Agregar un manifiesto registra sus datos para revisión. La ejecución requiere seleccionar y autorizar una instalación."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Rectangle { height: 1; color: Theme.line; Layout.fillWidth: true }
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Unidades y ubicaciones"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold; Layout.fillWidth: true }
            ActionButton { text: qsTr("Actualizar"); enabled: !volumes.refreshing; onClicked: volumes.refresh() }
        }
        Repeater {
            model: volumes.items
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true; spacing: 8
                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    Symbol { name: "disk"; tint: modelData.online ? Theme.muted : Theme.warning }
                    Label { text: modelData.label; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.fillWidth: true; elide: Text.ElideRight }
                    Label { text: modelData.online ? qsTr("%1 / %2 GB libres").arg((modelData.available/1e9).toFixed(1)).arg((modelData.total/1e9).toFixed(1)) : qsTr("Unidad no disponible"); color: Theme.muted; font.pixelSize: Theme.caption }
                }
                Label { text: modelData.mount || ""; textFormat: Text.PlainText; color: Theme.muted; font.family: Theme.mono; font.pixelSize: Theme.caption; Layout.leftMargin: 32 }
                Label { visible: modelData.online && !modelData.strongIdentity; text: qsTr("Identidad persistente no verificada; la relocalización requiere revisión."); color: Theme.warning; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true; Layout.leftMargin: 32 }
            }
        }
        Rectangle { height: 1; color: Theme.line; Layout.fillWidth: true }
        Label { text: qsTr("Diagnóstico"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
        Label { text: qsTr("SQLite y configuración locales. Los registros técnicos tienen rotación y no incluyen el contenido de tus archivos por defecto."); color: Theme.body; font.pixelSize: Theme.bodySize; wrapMode: Text.Wrap; Layout.fillWidth: true }
        RowLayout {
            spacing: 12
            ActionButton { text: qsTr("Ver registros"); symbol: "document"; onClicked: { logText.text=hub.diagnosticLog(); logs.open() } }
            ActionButton { text: qsTr("Carpeta de datos"); symbol: "folder"; onClicked: hub.openDataFolder() }
        }
        Item { height: 16 }
    }
    Dialog {
        id: logs; parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(900,parent.width-64); height: Math.min(620,parent.height-64); modal: true
        title: qsTr("Registros de diagnóstico"); standardButtons: Dialog.Close
        background: Rectangle { color: Theme.content; radius: Theme.radiusPanel; border.color: Theme.strongLine }
        contentItem: ScrollView { clip: true; TextArea { id: logText; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; color: Theme.body; font.family: Theme.mono; font.pixelSize: Theme.caption; wrapMode: TextEdit.NoWrap; background: Rectangle { color: Theme.content } } }
    }
}
