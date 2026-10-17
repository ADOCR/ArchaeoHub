import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

Item {
ColumnLayout {
    id: page; objectName: "coordinateWorkspace"; spacing: 8
    anchors.fill: parent
    property bool batch: false
    property bool editing: true
    property string picking: "source"
    property string exportFormat: "csv"
    readonly property bool sourceAngular: coordinates.sourceInfo.geographic === true
    readonly property bool targetAngular: coordinates.targetInfo.geographic === true
    readonly property var angleFormats: [qsTr("Grados decimales (DD)"),qsTr("Grados y minutos (DM)"),qsTr("Grados, minutos, segundos (DMS)")]
    function unitName(unit){return unit==="metre"?qsTr("Metros (m)"):unit==="degree"?qsTr("Grados (°)"):(unit || qsTr("Selecciona un sistema"))}
    function unitSymbol(unit){return unit==="metre"?"m":unit==="degree"?"°":(unit || "")}
    function axisLabel(info,axis){const unit=unitSymbol(info[axis+"Unit"]);return (info[axis+"Label"] || axis.toUpperCase())+(unit?" ("+unit+")":"")}
    function options(){return {inputFormat: sourceAngular?["DD","DM","DMS"][inputFormat.currentIndex]:"DD",outputFormat: targetAngular?["DD","DM","DMS"][outputFormat.currentIndex]:"DD",decimal: decimal.currentIndex===0?".":",",precision: precision.value,delimiter: delimiter.currentIndex===0?"\t":delimiter.currentIndex===1?";":",",header: header.checked,xColumn: xColumn.value-1,yColumn: yColumn.value-1,idColumn: idColumn.value-1}}
    property string submittedCrsQuery: ""
    property string submittedCrsGroup: ""
    function searchCrs(offset){
        const group=crsGroup.currentIndex===0&&crsQuery.text.trim().length>0?"all":crsGroup.currentValue
        if(crsQuery.text!==submittedCrsQuery||group!==submittedCrsGroup)offset=0
        submittedCrsQuery=crsQuery.text;submittedCrsGroup=group
        coordinates.search(crsQuery.text,group,offset)
    }
    function pick(which){picking=which;picker.open();searchCrs(0)}
    Component.onCompleted: coordinates.initialize()
    Connections { target: coordinates; function onConversionCompleted(){page.editing=false} function onSpreadsheetSheetsAvailable(){sheetPicker.open()} function onSpreadsheetImported(){page.batch=true;page.editing=true;delimiter.currentIndex=0;decimal.currentIndex=0} }
    RowLayout {
        Layout.fillWidth: true
        Symbol { name: "coordinates"; tint: Theme.accent; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
        ColumnLayout { Layout.fillWidth: true; spacing: 4
            Label { text: qsTr("Gestor de coordenadas"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
            Label { text: qsTr("CRS verificables · Conversión local · Resultados con procedencia"); color: Theme.muted; font.pixelSize: Theme.caption }
        }
        IconButton { symbol: "star"; hint: qsTr("Favorito"); onClicked: hub.toggleFavorite("workbench.coordinates") }
    }
    GlassSurface {
        Layout.fillWidth: true; implicitHeight: tools.implicitHeight+20
        Flow { id: tools; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: page.batch?qsTr("Un punto"):qsTr("Pegar tabla"); enabled: !coordinates.busy; onClicked: {page.batch=!page.batch;page.editing=true} }
            ActionButton { text: qsTr("Importar Excel / CSV / TXT…"); symbol: "folder"; enabled: !coordinates.busy; onClicked: inputDialog.open() }
            ActionButton { text: qsTr("Datos de ejemplo"); enabled: !coordinates.busy; onClicked: {coordinates.example();page.batch=true;page.editing=true} }
            ActionButton { text: qsTr("Copiar CSV"); enabled: !coordinates.busy&&Object.keys(coordinates.summary).length>0; onClicked: hub.copyText(coordinates.exportText("csv")) }
            ActionButton { text: qsTr("Exportar…"); enabled: !coordinates.busy&&Object.keys(coordinates.summary).length>0; onClicked: exportDialog.open() }
        }
    }
    ScrollView {
        id: setupView; objectName: "coordinateSetupView"
        Layout.fillWidth: true; Layout.fillHeight: page.editing; Layout.minimumHeight: 0
        Layout.preferredHeight: page.editing?Math.min(450,page.height*0.65):crsRow.implicitHeight
        clip: true; contentWidth: availableWidth
        ScrollBar.vertical.active: true
        ColumnLayout {
            width: setupView.availableWidth; spacing: 8
    RowLayout {
        id: crsRow; Layout.fillWidth: true; spacing: 12
        Repeater {
            model: ["source","target"]
            delegate: ColumnLayout {
                required property string modelData
                readonly property var info: modelData==="source"?coordinates.sourceInfo:coordinates.targetInfo
                Layout.fillWidth: true; Layout.preferredWidth: 1; spacing: 5
                Label { text: modelData==="source"?qsTr("Sistema de origen"):qsTr("Sistema de destino"); color: Theme.body; font.pixelSize: Theme.caption }
                ActionButton { Layout.fillWidth: true; text: (info.code || (modelData==="source"?coordinates.sourceCode:coordinates.targetCode))+" · "+(info.name || qsTr("Seleccionar CRS")); textElide: Text.ElideRight; ToolTip.visible: hovered; ToolTip.text: text; enabled: !coordinates.busy; onClicked: page.pick(modelData) }
                Label { text: info.datum || ""; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; elide: Text.ElideRight; Layout.fillWidth: true }
                Label { text: page.axisLabel(info,"x")+" / "+page.axisLabel(info,"y"); color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
            }
        }
        IconButton { symbol: "arrow"; hint: qsTr("Intercambiar CRS"); enabled: !coordinates.busy; onClicked: coordinates.describe(coordinates.targetCode,coordinates.sourceCode) }
    }
    Label { objectName: "coordinateInputHint"; visible: page.editing; text: Object.keys(coordinates.sourceInfo).length===0?qsTr("Selecciona el sistema en que están tus coordenadas."):page.sourceAngular?qsTr("Entrada: X = Longitud; Y = Latitud. Elige el formato de los ángulos."):qsTr("Entrada: X = Este; Y = Norte. Usa las unidades del CRS de origen."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
        ColumnLayout {
            visible: page.editing; Layout.fillWidth: true; spacing: 8
            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 12; rowSpacing: 8
                ColumnLayout {
                    Layout.fillWidth: true; Layout.preferredWidth: 1; spacing: 4
                    Label { text: qsTr("Formato de entrada"); color: Theme.body; font.pixelSize: Theme.caption }
                    ComboBox { id: inputFormat; objectName: "coordinateInputFormat"; Layout.fillWidth: true; visible: page.sourceAngular; model: page.angleFormats; enabled: !coordinates.busy; Accessible.name: qsTr("Formato angular de entrada") }
                    Label { objectName: "coordinateInputUnit"; visible: !page.sourceAngular; text: page.unitName(coordinates.sourceInfo.xUnit); color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.minimumHeight: Theme.controlHeight; verticalAlignment: Text.AlignVCenter; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.preferredWidth: 1; spacing: 4
                    Label { text: qsTr("Formato de salida"); color: Theme.body; font.pixelSize: Theme.caption }
                    ComboBox { id: outputFormat; objectName: "coordinateOutputFormat"; Layout.fillWidth: true; visible: page.targetAngular; model: page.angleFormats; enabled: !coordinates.busy; Accessible.name: qsTr("Formato angular de salida") }
                    Label { objectName: "coordinateOutputUnit"; visible: !page.targetAngular; text: page.unitName(coordinates.targetInfo.xUnit); color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.minimumHeight: Theme.controlHeight; verticalAlignment: Text.AlignVCenter; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 4
                    Label { text: qsTr("Separador decimal de entrada"); color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    ComboBox { id: decimal; objectName: "coordinateDecimal"; Layout.fillWidth: true; model: [qsTr("Punto (1234.56)"),qsTr("Coma (1234,56)")]; enabled: !coordinates.busy; Accessible.name: qsTr("Separador decimal de entrada") }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 4
                    Label { text: qsTr("Decimales de salida"); color: Theme.body; font.pixelSize: Theme.caption }
                    SpinBox { id: precision; objectName: "coordinatePrecision"; from: 0; to: 9; value: 3; enabled: !coordinates.busy; Accessible.name: qsTr("Decimales de salida") }
                }
            }
            RowLayout {
                visible: !page.batch; Layout.fillWidth: true
                ColumnLayout { Layout.preferredWidth: 115; Label { text: qsTr("Identificador"); color: Theme.body } AppField { id: pointId; Layout.fillWidth: true; Accessible.name: qsTr("Identificador del punto"); enabled: !coordinates.busy } }
                ColumnLayout { Layout.fillWidth: true; Label { text: page.axisLabel(coordinates.sourceInfo,"x"); color: Theme.body } AppField { id: singleX; objectName: "coordinateSingleX"; Layout.fillWidth: true; font.family: Theme.mono; enabled: !coordinates.busy; Accessible.name: page.axisLabel(coordinates.sourceInfo,"x") } }
                ColumnLayout { Layout.fillWidth: true; Label { text: page.axisLabel(coordinates.sourceInfo,"y"); color: Theme.body } AppField { id: singleY; objectName: "coordinateSingleY"; Layout.fillWidth: true; font.family: Theme.mono; enabled: !coordinates.busy; Accessible.name: page.axisLabel(coordinates.sourceInfo,"y") } }
            }
            ColumnLayout {
                visible: page.batch; Layout.fillWidth: true; spacing: 8
                Label { visible: coordinates.inputSheet.length>0; text: qsTr("Hoja Excel: %1").arg(coordinates.inputSheet); color: Theme.muted; font.pixelSize: Theme.caption }
                Label { text: qsTr("Tabla de entrada · hasta 10 000 puntos · no se modifica el archivo original"); color: Theme.body; font.pixelSize: Theme.caption }
                TextArea { id: table; Layout.fillWidth: true; Layout.preferredHeight: 100; text: coordinates.tableText; onTextChanged: {if(activeFocus)coordinates.tableText=text} enabled: !coordinates.busy; color: Theme.body; font.family: Theme.mono; font.pixelSize: Theme.caption; selectByMouse: true; wrapMode: TextEdit.NoWrap; Accessible.name: qsTr("Tabla de coordenadas"); background: Rectangle { color: Theme.content; radius: Theme.radiusSmall; border.color: table.activeFocus?Theme.accent:Theme.strongLine } }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox { id: delimiter; model: [qsTr("Tabulador"),qsTr("Punto y coma"),qsTr("Coma")]; enabled: !coordinates.busy; Accessible.name: qsTr("Separador de columnas") }
                    CheckBox { id: header; text: qsTr("Encabezados"); checked: true; enabled: !coordinates.busy; palette.windowText: Theme.body }
                    Label { text: "ID"; color: Theme.body; font.pixelSize: Theme.caption }
                    SpinBox { id: idColumn; from: 0; to: 128; value: 1; enabled: !coordinates.busy; Accessible.name: qsTr("Columna ID: cero para automática") }
                    Label { text: "X"; color: Theme.body }
                    SpinBox { id: xColumn; from: 1; to: 128; value: 2; enabled: !coordinates.busy; Accessible.name: qsTr("Columna X") }
                    Label { text: "Y"; color: Theme.body }
                    SpinBox { id: yColumn; from: 1; to: 128; value: 3; enabled: !coordinates.busy; Accessible.name: qsTr("Columna Y") }
                }
            }
        }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { Layout.fillWidth: true; text: coordinates.message; textFormat: Text.PlainText; color: coordinates.error?Theme.error:Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap }
        ActionButton { visible: coordinates.busy; text: qsTr("Cancelar"); onClicked: coordinates.cancel() }
        ActionButton { objectName: "coordinateConvert"; text: page.editing?qsTr("Convertir"):qsTr("Editar entrada"); symbol: "arrow"; variant: page.editing?"primary":"secondary"; enabled: !coordinates.busy&&Object.keys(coordinates.sourceInfo).length>0; onClicked: {if(!page.editing)page.editing=true;else if(page.batch)coordinates.convertBatch(page.options());else coordinates.convertSingle(pointId.text,singleX.text,singleY.text,page.options())} }
    }
    ProgressBar { visible: coordinates.busy; value: coordinates.progress; indeterminate: coordinates.progress===0; Layout.fillWidth: true; Layout.preferredHeight: 4 }
    Label { visible: Object.keys(coordinates.summary).length>0; text: qsTr("Resultados · %1 → %2").arg((coordinates.summary.source || {}).code || "").arg((coordinates.summary.target || {}).code || ""); color: Theme.ink; font.pixelSize: Theme.bodyLarge; font.weight: Font.DemiBold }
    Rectangle {
        visible: Object.keys(coordinates.summary).length>0
        Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 90; color: Theme.content; border.color: Theme.line; radius: Theme.radiusSmall
        Flickable {
            id: resultViewport
            anchors.fill: parent; anchors.margins: 1; clip: true; contentWidth: Math.max(width,1110); contentHeight: height
            ColumnLayout {
                width: resultViewport.contentWidth; height: resultViewport.height; spacing: 0
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: 36; color: Theme.surface2
                    Row { anchors.fill: parent; anchors.leftMargin: 8
                        Repeater { model: [qsTr("ID"),qsTr("Entrada X (%1)").arg(page.unitSymbol((coordinates.summary.source || {}).xUnit)),qsTr("Entrada Y (%1)").arg(page.unitSymbol((coordinates.summary.source || {}).yUnit)),qsTr("Salida X (%1)").arg(page.unitSymbol((coordinates.summary.target || {}).xUnit)),qsTr("Salida Y (%1)").arg(page.unitSymbol((coordinates.summary.target || {}).yUnit)),qsTr("Estado / error")]; delegate: Label { required property string modelData; width: modelData===qsTr("Estado / error")?360:148; height: 36; verticalAlignment: Text.AlignVCenter; text: modelData; elide: Text.ElideRight; ToolTip.visible: headerHover.hovered; ToolTip.text: text; HoverHandler { id: headerHover } color: Theme.body; font.pixelSize: Theme.caption; font.weight: Font.DemiBold } }
                    }
                }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: coordinates.rows; reuseItems: true; ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        required property string pointId; required property string inputX; required property string inputY; required property string outputX; required property string outputY; required property bool valid; required property string errorText
                        width: ListView.view.width; height: 42; color: Theme.content
                        Row { anchors.fill: parent; anchors.leftMargin: 8
                            Repeater { model: [pointId,inputX,inputY,outputX,outputY,valid?qsTr("Correcto"):errorText]; delegate: Label { required property string modelData; required property int index; width: index===5?360:148; height: 42; verticalAlignment: Text.AlignVCenter; text: modelData; textFormat: Text.PlainText; elide: Text.ElideRight; color: index===5&&!valid?Theme.error:Theme.body; font.family: index===5?Application.font.family:Theme.mono; font.pixelSize: Theme.caption; ToolTip.visible: hovered; property bool hovered: hover.hovered; HoverHandler { id: hover } ToolTip.text: modelData } }
                        }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
                    }
                    Label { anchors.centerIn: parent; visible: parent.count===0; text: qsTr("Los resultados aparecerán aquí."); color: Theme.muted; font.pixelSize: Theme.caption }
                }
            }
            ScrollBar.horizontal: ScrollBar {}
        }
    }
    Item { visible: !page.editing&&Object.keys(coordinates.summary).length===0; Layout.fillWidth: true; Layout.fillHeight: true }
    ActionButton { id: advanced; text: qsTr("CRS, operación y precisión"); variant: "quiet"; checkable: true }
    ScrollView {
        id: advancedView
        visible: advanced.checked; Layout.fillWidth: true; Layout.preferredHeight: 150; contentWidth: availableWidth; clip: true
        TextArea { width: advancedView.availableWidth; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; color: Theme.body; font.family: Theme.mono; font.pixelSize: Theme.caption; text: qsTr("Los decimales son de presentación; no representan exactitud del levantamiento. No se realizan transformaciones verticales ni dinámicas sin época. CRTM90 requiere una definición verificada y no se asume equivalente a CRTM05.\n\n")+JSON.stringify({source:coordinates.sourceInfo,target:coordinates.targetInfo,engine:coordinates.engineInfo,operations:coordinates.summary.operations || [],error:coordinates.technicalCode},null,2); background: Rectangle { color: Theme.surface1 } }
    }
    Popup {
        id: picker; objectName: "coordinateCrsPicker"; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(700,parent.width-64); height: Math.min(570,parent.height-64); modal: true; focus: true
        background: GlassSurface { popupSurface: true }
        contentItem: ColumnLayout {
            spacing: 12
            Label { text: page.picking==="source"?qsTr("Elegir CRS de origen"):qsTr("Elegir CRS de destino"); color: Theme.ink; font.pixelSize: Theme.h2 }
            ComboBox {
                id: crsGroup; objectName: "coordinateCrsGroup"; Layout.fillWidth: true; enabled: !coordinates.busy
                textRole: "label"; valueRole: "key"; Accessible.name: qsTr("Grupo de sistemas de coordenadas")
                model: [
                    {key:"recommended",label:qsTr("Recomendados")},
                    {key:"costa-rica",label:qsTr("Costa Rica")},
                    {key:"central-america",label:qsTr("Centroamérica e históricos regionales")},
                    {key:"south-america",label:qsTr("Sudamérica · SIRGAS, MAGNA, POSGAR y otros")},
                    {key:"geographic",label:qsTr("Geográficos · longitud / latitud")},
                    {key:"utm-north",label:qsTr("WGS84 / UTM · zonas 1–60 Norte")},
                    {key:"utm-south",label:qsTr("WGS84 / UTM · zonas 1–60 Sur")},
                    {key:"world",label:qsTr("Cartografía mundial y otros países")},
                    {key:"all",label:qsTr("Toda la base EPSG · buscar o explorar")}
                ]
                onActivated: page.searchCrs(0)
            }
            RowLayout { Layout.fillWidth: true; AppField { id: crsQuery; objectName: "coordinateCrsQuery"; Layout.fillWidth: true; enabled: !coordinates.busy; placeholderText: qsTr("EPSG, nombre, Costa Rica, UTM…"); Accessible.name: qsTr("Buscar sistema de referencia"); onAccepted: page.searchCrs(0) } ActionButton { text: qsTr("Buscar"); enabled: !coordinates.busy; onClicked: page.searchCrs(0) } }
            Label { text: qsTr("Definiciones EPSG locales. Cada selección verifica ejes, unidades y datum; una conversión puede requerir rejillas no disponibles. No se adivina la zona UTM."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: coordinates.searchResults; ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    required property var modelData; width: ListView.view.width; height: 68; enabled: !coordinates.busy
                    contentItem: ColumnLayout { spacing: 4; Label { text: modelData.code+" · "+modelData.name; textFormat: Text.PlainText; color: Theme.ink; font.pixelSize: Theme.bodySize; elide: Text.ElideRight; Layout.fillWidth: true } Label { text: modelData.area || ""; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; elide: Text.ElideRight; Layout.fillWidth: true } }
                    onClicked: {picker.close();coordinates.describe(page.picking==="source"?modelData.code:coordinates.sourceCode,page.picking==="target"?modelData.code:coordinates.targetCode)}
                }
            }
            RowLayout {
                Layout.fillWidth: true
                ActionButton { text: qsTr("Anterior"); enabled: !coordinates.busy&&coordinates.searchOffset>0; onClicked: page.searchCrs(Math.max(0,coordinates.searchOffset-100)) }
                Label { text: qsTr("%1–%2 de %3").arg(coordinates.searchResults.length?coordinates.searchOffset+1:0).arg(coordinates.searchOffset+coordinates.searchResults.length).arg(coordinates.searchTotal); color: Theme.muted; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                ActionButton { text: qsTr("Siguiente"); enabled: !coordinates.busy&&coordinates.searchOffset+coordinates.searchResults.length<coordinates.searchTotal; onClicked: page.searchCrs(coordinates.searchOffset+100) }
                ActionButton { text: qsTr("Cerrar"); onClicked: picker.close() }
            }
        }
    }
    FileDialog { id: inputDialog; title: qsTr("Importar tabla de coordenadas"); fileMode: FileDialog.OpenFile; nameFilters: [qsTr("Excel y tablas (*.xlsx *.csv *.txt *.tsv)"),qsTr("Excel (*.xlsx)")]; onAccepted: {coordinates.importText(selectedFile);page.batch=true;page.editing=true} }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+O":"Ctrl+O"; enabled: !coordinates.busy; onActivated: inputDialog.open() }
    Dialog { id: sheetPicker; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(480,parent.width-64); modal: true; title: qsTr("Elegir hoja de Excel"); standardButtons: Dialog.Ok|Dialog.Cancel; contentItem: ComboBox { id: sheetChoice; model: coordinates.sheets; Accessible.name: qsTr("Hoja de Excel") } onAccepted: coordinates.selectSheet(sheetChoice.currentText) }
    FileDialog { id: exportDialog; title: qsTr("Guardar resultados con procedencia"); fileMode: FileDialog.SaveFile; nameFilters: [qsTr("Excel (*.xlsx)"),qsTr("CSV (*.csv)"),qsTr("JSON completo (*.json)")]; defaultSuffix: ["xlsx","csv","json"][selectedNameFilter.index]; onAccepted: coordinates.exportFile(selectedFile,["xlsx","csv","json"][selectedNameFilter.index]) }
}
}
