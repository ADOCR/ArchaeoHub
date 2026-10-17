import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

ColumnLayout {
    id: page; objectName: "catalogWorkspace"
    spacing: 16
    property bool configuring: !catalog.opened
    property bool correcting: false
    property bool dirty: false
    property bool browseOnly: false
    property bool creating: false
    property url pendingCatalog: ""
    property int pendingStep: 1
    property real zoom: 1
    property int turn: 0
    function fitView(){preview.cancelFlick();zoom=1;preview.contentX=0;preview.contentY=0}
    function zoomKeysAllowed(){let focus=page.Window.window?page.Window.window.activeFocusItem:null;return hasPhoto&&!browseOnly&&!(focus instanceof TextInput)&&!(focus instanceof TextEdit)}
    property string pendingSelection: ""
    property string removal: ""
    readonly property var photo: catalog.selection
    readonly property bool hasPhoto: !!photo.id
    readonly property bool editable: hasPhoto && (!photo.registered || correcting) && !catalog.busy && !catalog.state.recoveryRequired
    Component.onCompleted: {
        if(page.hasPhoto)page.loadSelection()
        let draft=catalog.draft
        if(draft.id===page.photo.id&&draft.form){let f=draft.form;operation.text=f.operation;unitType.currentIndex=Math.max(0,unitType.model.indexOf(f.unitType));unit.text=f.unit;subunitType.currentIndex=Math.max(0,subunitType.model.indexOf(f.subunitType));subunit.text=f.subunit;level.text=f.level;detail.text=f.view;notes.text=f.notes;page.dirty=!!draft.dirty;page.correcting=!!draft.correcting}
        if(draft.id===page.photo.id&&draft.gps){gpsLatitude.text=draft.gps[0];gpsLongitude.text=draft.gps[1];gpsAltitude.text=draft.gps[2];gpsAccuracy.text=draft.gps[3]}
    }
    Component.onDestruction: catalog.draft={id:page.photo.id || "",form:page.form(),dirty:page.dirty,correcting:page.correcting,gps:[gpsLatitude.text,gpsLongitude.text,gpsAltitude.text,gpsAccuracy.text]}
    function form() { return {operation: operation.text, unitType: unitType.currentText, unit: unit.text, level: level.text, subunitType: subunitType.currentText, subunit: subunit.text, view: detail.text, notes: notes.text} }
    function openCatalog(confirm) { if(creating)catalog.createCatalog(source.text,output.text,site.text,legacyOperation.text);else catalog.loadCatalog(hub.fileUrl(output.text),source.text,catalog.project.needsSite?site.text:"",catalog.project.needsOperation?legacyOperation.text:"",confirm) }
    function loadCatalog(file) { if(dirty){pendingCatalog=file;pendingSelection="__load";leaveDialog.open()}else{creating=false;catalog.loadCatalog(file,source.text)} }
    function savePhoto() { if(correcting)catalog.correct(form());else catalog.saveAndNext(form(),[gpsLatitude.text,gpsLongitude.text,gpsAltitude.text,gpsAccuracy.text].some(v=>v.trim().length>0)?[gpsLatitude.text,gpsLongitude.text,gpsAltitude.text,gpsAccuracy.text]:[]) }
    function choose(id) { if(catalog.busy)return; if(dirty){pendingSelection=id;leaveDialog.open()}else catalog.select(id) }
    function step(delta) {if(catalog.busy||!hasPhoto)return;if(dirty){pendingStep=delta;pendingSelection="__step";leaveDialog.open()}else catalog.navigate(delta)}
    function loadSelection() {
        correcting=false;dirty=false;turn=0;fitView()
        let row=photo.record || {}
        if(photo.registered){
            operation.text=row.Operacion || "";unitType.currentIndex=Math.max(0,unitType.model.indexOf(row.Tipo_unidad));unit.text=row.Unidad || ""
            subunitType.currentIndex=Math.max(0,subunitType.model.indexOf(row.Tipo_subunidad || ""));subunit.text=row.Subunidad || "";level.text=row.Nivel || ""
        } else if(!keepContext.checked){operation.text="";unitType.currentIndex=0;unit.text="";subunitType.currentIndex=0;subunit.text="";level.text=""}
        detail.text=row.Vista_o_detalle || "";notes.text=row.Observaciones || ""
        gpsLatitude.text="";gpsLongitude.text="";gpsAltitude.text="";gpsAccuracy.text=""
    }
    Connections {
        target: catalog
        function onSessionReset(){operation.text=catalog.state.initialOperation||"";unitType.currentIndex=0;unit.text="";subunitType.currentIndex=0;subunit.text="";level.text="";detail.text="";notes.text="";gpsLatitude.text="";gpsLongitude.text="";gpsAltitude.text="";gpsAccuracy.text="";page.dirty=false;page.correcting=false}
        function onSelected(){page.loadSelection()}
        function onSaved(){page.loadSelection();filter.currentIndex=0}
        function onProjectDiscovered(){let p=catalog.project;output.text=p.output;source.text=p.source||p.suggestedSource||source.text;site.text=p.site||"";legacyOperation.text=p.operation||"";if(catalog.awaitingInput)page.configuring=true}
        function onChanged(){let index=filter.model.indexOf(catalog.activeFilter);if(index>=0)filter.currentIndex=index}
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 12
        Symbol { name: "catalog"; tint: Theme.accent; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 4
            Label { text: qsTr("Catálogo fotográfico"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold }
            Label { text: catalog.opened ? qsTr("%1 · %2 fotografías · %3 registradas").arg(catalog.state.site).arg(catalog.state.total).arg(catalog.state.registered) : qsTr("Fotografías, contexto y entrega al Museo"); color: Theme.muted; font.pixelSize: Theme.caption }
        }
        IconButton { symbol: "settings"; hint: qsTr("Carpetas del catálogo"); selected: page.configuring; enabled: !catalog.busy; onClicked: page.configuring=!page.configuring }
        IconButton { symbol: "star"; hint: qsTr("Favorito"); onClicked: hub.toggleFavorite("workbench.photo-catalog") }
    }
    // El vidrio se usa en navegación y acciones; fotografías y metadatos mantienen superficies sólidas.
    GlassSurface {
        visible: catalog.opened; Layout.fillWidth: true; implicitHeight: bar.implicitHeight+20
        Flow {
            id: bar; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: page.browseOnly ? qsTr("Ver detalle") : qsTr("Ver fotografías"); symbol: "catalog"; onClicked: page.browseOnly=!page.browseOnly }
            ActionButton { text: qsTr("Abrir original"); symbol: "arrow"; enabled: page.hasPhoto&&!catalog.busy; onClicked: hub.openFile(hub.fileUrl(page.photo.path)) }
            ActionButton { text: qsTr("Entrega Museo"); symbol: "folder"; enabled: !catalog.busy; onClicked: hub.openFile(hub.fileUrl(catalog.state.xlsx)) }
            ActionButton { text: qsTr("Reporte GPS…"); enabled: !catalog.busy; onClicked: gpsExport.open() }
        }
    }
    ScrollView {
        id: configurationView
        visible: page.configuring; Layout.fillWidth: true; Layout.fillHeight: !catalog.opened
        Layout.preferredHeight: catalog.opened ? Math.min(280,page.height*0.5) : -1
        clip: true; contentWidth: availableWidth
        ColumnLayout {
            width: configurationView.availableWidth; spacing: 12
            Label { text: page.creating ? qsTr("Crear un catálogo nuevo") : qsTr("Continuar un catálogo existente"); color: Theme.ink; font.pixelSize: Theme.h2; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; spacing: 8
                ActionButton { objectName: "loadCatalogButton"; text: qsTr("Cargar catálogo…"); symbol: "folder"; variant: "primary"; enabled: !catalog.busy; onClicked: catalogPicker.open() }
                ActionButton { text: qsTr("Nuevo catálogo"); enabled: !catalog.busy; onClicked: {page.creating=true;page.configuring=true;output.text="";site.text="";legacyOperation.text=""} }
            }
            Label { text: page.creating ? qsTr("Elige los originales y una carpeta de salida vacía.") : qsTr("Selecciona el Excel del catálogo o su archivo estado_catalogador.json. El sitio y las operaciones se leen del catálogo; no tienes que escribirlos otra vez."); color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Origen de las fotografías"); color: Theme.body }
            RowLayout { Layout.fillWidth: true; AppField { id: source; objectName: "catalogSourceField"; Layout.fillWidth: true; text: catalog.remembered.source || ""; enabled: !catalog.busy; Accessible.name: qsTr("Carpeta de fotografías originales") } ActionButton { text: qsTr("Elegir…"); enabled: !catalog.busy; onClicked: {folderPicker.choosingOutput=false;folderPicker.open()} } }
            Label { text: qsTr("Carpeta de trabajo y entrega"); color: Theme.body }
            RowLayout { Layout.fillWidth: true; AppField { id: output; Layout.fillWidth: true; text: catalog.remembered.output || ""; enabled: !catalog.busy; Accessible.name: qsTr("Carpeta de salida del catálogo") } ActionButton { text: qsTr("Elegir…"); enabled: !catalog.busy; onClicked: {folderPicker.choosingOutput=true;folderPicker.open()} } }
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                ColumnLayout { visible: page.creating||!!catalog.project.needsSite; Layout.fillWidth: true; Label { text: qsTr("Código del sitio"); color: Theme.body } AppField { id: site; Layout.fillWidth: true; text: catalog.remembered.site || ""; enabled: !catalog.busy; Accessible.name: qsTr("Código del sitio") } }
                ColumnLayout { visible: page.creating||!!catalog.project.needsOperation; Layout.preferredWidth: 150; Label { text: qsTr("Operación inicial"); color: Theme.body } AppField { id: legacyOperation; Layout.fillWidth: true; text: catalog.remembered.operation || ""; enabled: !catalog.busy; Accessible.name: qsTr("Operación inicial del catálogo") } }
                ActionButton { objectName: "openCatalogFolderButton"; text: page.creating ? qsTr("Crear catálogo") : catalog.awaitingInput ? qsTr("Continuar con estos originales") : qsTr("Abrir carpeta indicada"); enabled: !catalog.busy&&output.text.trim().length>0&&(!page.creating||(source.text.trim().length>0&&site.text.trim().length>0&&legacyOperation.text.trim().length>0)); onClicked: {if(page.dirty){page.pendingSelection="__open";leaveDialog.open()}else page.openCatalog(false)} }
            }
            Label { visible: !page.creating&&!!catalog.project.site; text: qsTr("Sitio: %1 · %2 registros · Operaciones: %3").arg(catalog.project.site||"").arg(catalog.project.rows||0).arg((catalog.project.operations||[]).join(", ")||qsTr("sin datos")); color: Theme.body; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Al guardar se crean copias de entrega y el Excel del Museo. Los originales se conservan. “Resguardar original” lo mueve sólo tras una confirmación, con registro para recuperarlo. La operación se indica en el formulario de contexto."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
    }
    RowLayout {
        visible: catalog.opened; Layout.fillWidth: true
        ComboBox { id: filter; model: [qsTr("Todas"),qsTr("Por catalogar"),qsTr("Pendientes"),qsTr("Omitidas"),qsTr("Registradas"),qsTr("GPS por revisar")]; enabled: !catalog.busy; onActivated: catalog.page(currentText,search.text,0); Accessible.name: qsTr("Filtro de fotografías") }
        AppField { id: search; Layout.fillWidth: true; placeholderText: qsTr("Buscar nombre o contexto…"); Accessible.name: qsTr("Buscar fotografías"); onAccepted: catalog.page(filter.currentText,text,0); enabled: !catalog.busy }
        ActionButton { text: qsTr("Buscar"); enabled: !catalog.busy; onClicked: catalog.page(filter.currentText,search.text,0) }
    }
    RowLayout {
        visible: catalog.opened; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
        GridView {
            id: browser; visible: catalog.opened&&(page.browseOnly||page.width>1060); clip: true
            Layout.fillWidth: page.browseOnly; Layout.preferredWidth: 210; Layout.fillHeight: true
            cellWidth: page.browseOnly ? Math.max(160,Math.floor(width/Math.max(1,Math.floor(width/180)))) : width
            cellHeight: 166; model: catalog.items; reuseItems: true
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                required property var modelData
                width: browser.cellWidth-8; height: browser.cellHeight-8; enabled: !catalog.busy
                Accessible.name: modelData.name; onClicked: {page.choose(modelData.id);page.browseOnly=false}
                background: Rectangle { color: page.photo.id===modelData.id ? Theme.accentSoft : Theme.content; radius: Theme.radiusSmall; border.color: page.photo.id===modelData.id ? Theme.accent : Theme.line }
                contentItem: ColumnLayout {
                    spacing: 4
                    Image { source: modelData.thumbnail || ""; asynchronous: true; fillMode: Image.PreserveAspectFit; Layout.fillWidth: true; Layout.fillHeight: true }
                    Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.caption; elide: Text.ElideMiddle; Layout.fillWidth: true }
                    Label { text: modelData.registered ? qsTr("Registrada") : modelData.skipped ? qsTr("Omitida") : qsTr("Pendiente"); color: Theme.muted; font.pixelSize: Theme.caption }
                }
            }
        }
        Rectangle {
            visible: !page.browseOnly; Layout.fillWidth: true; Layout.fillHeight: true; color: Theme.content; radius: Theme.radius; border.color: Theme.line
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 8
                Label { text: page.hasPhoto ? page.photo.name : qsTr("Vista previa"); textFormat: Text.PlainText; elide: Text.ElideMiddle; color: Theme.ink; font.pixelSize: Theme.bodySize; Layout.fillWidth: true }
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                Flickable {
                    id: preview; objectName: "catalogPreview"; anchors.fill: parent; clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    readonly property real naturalWidth: (page.photo.preview||{}).width||1
                    readonly property real naturalHeight: (page.photo.preview||{}).height||1
                    readonly property real rotatedWidth: page.turn%180 ? naturalHeight : naturalWidth
                    readonly property real rotatedHeight: page.turn%180 ? naturalWidth : naturalHeight
                    readonly property real fitScale: Math.min(8,width/Math.max(1,rotatedWidth),height/Math.max(1,rotatedHeight))
                    function zoomAt(value,px,py){
                        if(!page.hasPhoto||fitScale<=0)return;
                        cancelFlick();
                        const next=Math.max(0.001/fitScale,Math.min(8/fitScale,value));
                        const oldWidth=rotatedWidth*fitScale*page.zoom,oldHeight=rotatedHeight*fitScale*page.zoom;
                        const u=Math.max(0,Math.min(1,(contentX+px-(contentWidth-oldWidth)/2)/oldWidth));
                        const v=Math.max(0,Math.min(1,(contentY+py-(contentHeight-oldHeight)/2)/oldHeight));
                        const newWidth=rotatedWidth*fitScale*next,newHeight=rotatedHeight*fitScale*next;
                        const canvasWidth=Math.max(width,newWidth),canvasHeight=Math.max(height,newHeight);
                        page.zoom=next;
                        contentX=Math.max(0,Math.min(canvasWidth-width,(canvasWidth-newWidth)/2+u*newWidth-px));
                        contentY=Math.max(0,Math.min(canvasHeight-height,(canvasHeight-newHeight)/2+v*newHeight-py));
                    }
                    contentWidth: Math.max(width,rotatedWidth*fitScale*page.zoom); contentHeight: Math.max(height,rotatedHeight*fitScale*page.zoom)
                    Image { id: photoImage; objectName: "catalogPreviewImage"; anchors.centerIn: parent; anchors.alignWhenCentered: false; width: preview.naturalWidth*preview.fitScale*page.zoom; height: preview.naturalHeight*preview.fitScale*page.zoom; source: (page.photo.preview || {}).url || ""; asynchronous: true; fillMode: Image.PreserveAspectFit; rotation: page.turn; transformOrigin: Item.Center }
                    Label { anchors.centerIn: parent; width: parent.width-32; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; visible: !page.hasPhoto || !!((page.photo.preview || {}).imageError); text: page.hasPhoto ? (page.photo.preview || {}).imageError || "" : catalog.opened ? qsTr("Selecciona una fotografía en “Ver fotografías”.") : qsTr("Elige las carpetas y el código del sitio para comenzar."); color: Theme.muted }
                    ScrollBar.horizontal: ScrollBar {} ScrollBar.vertical: ScrollBar {}
                }
                    MouseArea { anchors.fill: parent; acceptedButtons: Qt.NoButton; enabled: page.hasPhoto
                        onWheel: event=>{let delta=event.angleDelta.y;if(delta===0)delta=event.pixelDelta.y;
                            if(delta!==0){preview.zoomAt(page.zoom*(delta>0?1.2:1/1.2),event.x,event.y);event.accepted=true}
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true; spacing: 6
                    ActionButton { text: "←"; enabled: catalog.opened&&!catalog.busy&&catalog.items.length>0; onClicked: page.step(-1); Accessible.name: qsTr("Fotografía anterior") }
                    ActionButton { text: "−"; enabled: page.hasPhoto; onClicked: preview.zoomAt(page.zoom/1.25,preview.width/2,preview.height/2); Accessible.name: qsTr("Alejar imagen") }
                    ActionButton { text: qsTr("Ajustar"); enabled: page.hasPhoto; onClicked: page.fitView() }
                    ActionButton { text: "+"; enabled: page.hasPhoto; onClicked: preview.zoomAt(page.zoom*1.25,preview.width/2,preview.height/2); Accessible.name: qsTr("Ampliar imagen") }
                    ActionButton { text: qsTr("100 %"); enabled: page.hasPhoto; onClicked: preview.zoomAt(1/preview.fitScale,preview.width/2,preview.height/2); Accessible.name: qsTr("Tamaño real de la vista previa") }
                    ActionButton { text: qsTr("Girar"); enabled: page.hasPhoto; onClicked: {page.turn=(page.turn+90)%360;page.fitView()} }
                    ActionButton { text: "→"; enabled: catalog.opened&&!catalog.busy&&catalog.items.length>0; onClicked: page.step(1); Accessible.name: qsTr("Fotografía siguiente") }
                    Label { text: Math.round(preview.fitScale*page.zoom*100)+" %"; color: Theme.muted; height: 40; verticalAlignment: Text.AlignVCenter; font.pixelSize: Theme.caption; Accessible.name: qsTr("Escala de la vista previa") }
                }
                Label { text: qsTr("Zoom y giro sólo cambian esta vista."); color: Theme.muted; font.pixelSize: Theme.caption; Layout.alignment: Qt.AlignHCenter }
            }
        }
        ScrollView {
            id: inspectorView
            visible: !page.browseOnly; Layout.preferredWidth: Math.min(330,page.width*0.43); Layout.fillHeight: true; contentWidth: availableWidth; clip: true
            ColumnLayout {
                width: inspectorView.availableWidth; spacing: 8
                Label { text: qsTr("Contexto arqueológico"); color: Theme.ink; font.pixelSize: Theme.h2; font.weight: Font.DemiBold }
                Label { text: qsTr("Operación"); color: Theme.body }
                AppField { id: operation; Layout.fillWidth: true; text: legacyOperation.text; enabled: !catalog.busy&&(!page.hasPhoto||page.editable); onTextEdited: page.dirty=page.hasPhoto; Accessible.name: qsTr("Operación") }
                Label { text: qsTr("Tipo de unidad"); color: Theme.body }
                ComboBox { id: unitType; Layout.fillWidth: true; model: ["Pozo","Pozo auxiliar","Trinchera"]; enabled: page.editable; onActivated: page.dirty=true; Accessible.name: qsTr("Tipo de unidad") }
                Label { text: qsTr("Unidad / pozo"); color: Theme.body }
                AppField { id: unit; Layout.fillWidth: true; enabled: page.editable; onTextEdited: page.dirty=true; Accessible.name: qsTr("Unidad o pozo") }
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout { Layout.fillWidth: true; Label { text: qsTr("Subunidad (opcional)"); color: Theme.body } ComboBox { id: subunitType; Layout.fillWidth: true; model: ["","Cuadro","Cuadrante","Suboperación","Ampliación","Rasgo cultural","Rasgo funerario","Otro"]; enabled: page.editable; onActivated: page.dirty=true; Accessible.name: qsTr("Tipo de subunidad") } }
                    AppField { id: subunit; Layout.preferredWidth: 85; Layout.alignment: Qt.AlignBottom; enabled: page.editable&&subunitType.currentIndex>0; onTextEdited: page.dirty=true; Accessible.name: qsTr("Código de subunidad") }
                }
                Label { text: qsTr("Nivel"); color: Theme.body }
                AppField { id: level; Layout.fillWidth: true; enabled: page.editable; onTextEdited: page.dirty=true; Accessible.name: qsTr("Nivel") }
                Label { text: qsTr("Vista o detalle"); color: Theme.body }
                AppField { id: detail; Layout.fillWidth: true; enabled: page.editable; onTextEdited: page.dirty=true; Accessible.name: qsTr("Vista o detalle") }
                Label { text: qsTr("Observaciones"); color: Theme.body }
                TextArea { id: notes; Layout.fillWidth: true; Layout.preferredHeight: 88; wrapMode: TextEdit.Wrap; color: Theme.body; enabled: page.editable; Accessible.name: qsTr("Observaciones"); background: Rectangle { color: Theme.content; radius: Theme.radiusSmall; border.color: notes.activeFocus?Theme.accent:Theme.strongLine } onTextChanged: {if(activeFocus)page.dirty=true} }
                CheckBox { id: keepContext; text: qsTr("Mantener contexto para la siguiente"); checked: true; palette.windowText: Theme.body; font.pixelSize: Theme.caption }
                ActionButton { text: qsTr("Previsualizar nombre"); enabled: page.editable&&unit.text.length>0&&level.text.length>0; onClicked: catalog.propose(page.form()) }
                Label { text: catalog.proposedName; textFormat: Text.PlainText; color: Theme.muted; font.family: Theme.mono; font.pixelSize: Theme.caption; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                Flow {
                    Layout.fillWidth: true; spacing: 6
                    ActionButton { text: page.photo.skipped ? qsTr("Recuperar omitida") : qsTr("Omitir"); enabled: page.hasPhoto&&!page.photo.registered&&!catalog.busy; onClicked: {if(page.dirty){page.pendingSelection="__skip";leaveDialog.open()}else catalog.skip()} }
                    ActionButton { text: qsTr("Retirar registro…"); enabled: page.hasPhoto&&!!page.photo.registered&&!catalog.busy; onClicked: {page.removal="withdraw";removeDialog.open()} }
                    ActionButton { text: qsTr("Resguardar original…"); enabled: page.hasPhoto&&!catalog.busy; onClicked: {page.removal="discard";removeDialog.open()} }
                    ActionButton { text: qsTr("Cancelar corrección"); visible: page.correcting; enabled: !catalog.busy; onClicked: page.loadSelection() }
                    ActionButton { text: qsTr("Siguiente GPS por revisar"); enabled: page.hasPhoto&&!catalog.busy&&catalog.state.gpsReview>0; onClicked: {if(page.dirty){page.pendingSelection="__gps";leaveDialog.open()}else{filter.currentIndex=0;catalog.nextGps()}} }
                }
                ActionButton { id: gpsDetails; text: qsTr("GPS y detalles técnicos"); variant: "quiet"; checkable: true }
                ColumnLayout {
                    visible: gpsDetails.checked; Layout.fillWidth: true; spacing: 8
                    Label { text: page.photo.path || ""; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                    Label { text: qsTr("El GPS de la cámara es una referencia de la fotografía, no la ubicación validada del contexto. No se añade al Excel del Museo."); color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: Object.keys(page.photo.gps || {}).length ? qsTr("Latitud: %1° · Longitud: %2°").arg(page.photo.gps.latitude).arg(page.photo.gps.longitude) : qsTr("Sin GPS válido en el original."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { visible: !!page.photo.registered; text: Object.keys(page.photo.copyGps || {}).length ? qsTr("Copia de entrega: %1°, %2°").arg(page.photo.copyGps.latitude).arg(page.photo.copyGps.longitude) : qsTr("Copia de entrega: GPS pendiente de revisión."); color: Theme.body; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: qsTr("GPS manual para la copia JPEG registrada"); color: Theme.body; font.pixelSize: Theme.caption }
                    Label { text: qsTr("Latitud (° WGS84)"); color: Theme.body } AppField { id: gpsLatitude; Layout.fillWidth: true; onTextEdited: page.dirty=true; Accessible.name: qsTr("Latitud GPS manual") }
                    Label { text: qsTr("Longitud (° WGS84)"); color: Theme.body } AppField { id: gpsLongitude; Layout.fillWidth: true; onTextEdited: page.dirty=true; Accessible.name: qsTr("Longitud GPS manual") }
                    Label { text: qsTr("Altitud (m, opcional)"); color: Theme.body } AppField { id: gpsAltitude; Layout.fillWidth: true; onTextEdited: page.dirty=true; Accessible.name: qsTr("Altitud GPS manual") }
                    Label { text: qsTr("Error horizontal (m, opcional)"); color: Theme.body } AppField { id: gpsAccuracy; Layout.fillWidth: true; onTextEdited: page.dirty=true; Accessible.name: qsTr("Error horizontal GPS manual") }
                    ActionButton { text: qsTr("Guardar GPS en la copia"); enabled: page.hasPhoto&&!!page.photo.registered&&!catalog.busy&&gpsLatitude.text.length>0&&gpsLongitude.text.length>0; onClicked: gpsConfirm.open() }
                }
            }
        }
    }
    RowLayout {
        visible: catalog.opened; Layout.fillWidth: true
        ActionButton { text: qsTr("Página anterior"); enabled: !catalog.busy&&catalog.offset>0; onClicked: catalog.page(filter.currentText,search.text,Math.max(0,catalog.offset-60)) }
        Label { text: qsTr("%1–%2 de %3").arg(catalog.matched?catalog.offset+1:0).arg(Math.min(catalog.offset+60,catalog.matched)).arg(catalog.matched); color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true }
        ActionButton { text: qsTr("Página siguiente"); enabled: !catalog.busy&&catalog.offset+60<catalog.matched; onClicked: catalog.page(filter.currentText,search.text,catalog.offset+60) }
        ActionButton { visible: !page.browseOnly; text: page.photo.registered&&!page.correcting ? qsTr("Corregir registro…") : page.correcting ? qsTr("Guardar corrección") : qsTr("Guardar y siguiente"); variant: "primary"; enabled: page.hasPhoto&&!catalog.busy&&!catalog.state.recoveryRequired&&operation.text.length>0&&unit.text.length>0&&level.text.length>0; onClicked: {if(page.photo.registered&&!page.correcting)page.correcting=true;else page.savePhoto()} }
    }
    ProgressBar { visible: catalog.busy; indeterminate: true; Layout.fillWidth: true; Layout.preferredHeight: 4 }
    Label { visible: catalog.message.length>0; text: catalog.message; textFormat: Text.PlainText; color: catalog.error?Theme.error:Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { visible: !!catalog.state.recoveryRequired; text: qsTr("Reabre este catálogo antes de guardar más cambios. No repitas el registro: la operación quedó confirmada y necesita completar su recuperación."); color: Theme.warning; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }
    FolderDialog { id: folderPicker; property bool choosingOutput: false; title: choosingOutput?qsTr("Carpeta de trabajo y entrega"):qsTr("Carpeta de fotografías originales"); onAccepted: {let path=hub.localPath(selectedFolder);if(choosingOutput)output.text=path;else source.text=path} }
    FileDialog { id: catalogPicker; title: qsTr("Cargar catálogo existente"); nameFilters: [qsTr("Catálogo (*.xlsx *.json)")]; fileMode: FileDialog.OpenFile; onAccepted: page.loadCatalog(selectedFile) }
    FileDialog { id: gpsExport; title: qsTr("Crear reporte GPS fuera de Entrega_Museo"); nameFilters: [qsTr("Excel (*.xlsx)")]; fileMode: FileDialog.SaveFile; defaultSuffix: "xlsx"; onAccepted: catalog.exportGps(selectedFile) }
    Dialog {
        id: leaveDialog; parent: Overlay.overlay; anchors.centerIn: parent; implicitWidth: 520; implicitHeight: 240; width: Math.min(implicitWidth,parent.width-64); height: implicitHeight; modal: true; title: qsTr("Formulario sin guardar"); standardButtons: Dialog.Discard|Dialog.Cancel
        contentItem: Label { text: qsTr("Hay cambios en el formulario. ¿Descartarlos y continuar?"); color: Theme.body; wrapMode: Text.Wrap }
        onDiscarded: {page.dirty=false;if(page.pendingSelection==="__open")page.openCatalog(false);else if(page.pendingSelection==="__load")page.loadCatalog(page.pendingCatalog);else if(page.pendingSelection==="__gps"){filter.currentIndex=0;catalog.nextGps()}else if(page.pendingSelection==="__step")catalog.navigate(page.pendingStep);else if(page.pendingSelection==="__skip")catalog.skip();else catalog.select(page.pendingSelection)}
    }
    Dialog {
        id: migrationDialog; parent: Overlay.overlay; anchors.centerIn: parent; implicitWidth: 560; implicitHeight: 260; width: Math.min(implicitWidth,parent.width-64); height: implicitHeight; modal: true; title: qsTr("Actualizar esquema anterior"); standardButtons: Dialog.Ok|Dialog.Cancel
        contentItem: Label { text: catalog.message; textFormat: Text.PlainText; color: Theme.body; wrapMode: Text.Wrap }
        onAccepted: page.openCatalog(true)
    }
    Connections { target: catalog; function onChanged(){if(catalog.migrationRequired&&!catalog.busy&&!migrationDialog.visible)migrationDialog.open();if(catalog.opened&&!catalog.busy&&!catalog.error&&!catalog.awaitingInput&&!catalog.migrationRequired)page.configuring=false} }
    Dialog {
        id: removeDialog; parent: Overlay.overlay; anchors.centerIn: parent; implicitWidth: 560; implicitHeight: 260; width: Math.min(implicitWidth,parent.width-64); height: implicitHeight; modal: true; title: page.removal==="withdraw"?qsTr("Retirar de la entrega"):qsTr("Resguardar original"); standardButtons: Dialog.Ok|Dialog.Cancel
        contentItem: Label { text: page.removal==="withdraw"?qsTr("Se retirará la fila del Excel y se guardará la copia fuera de Entrega_Museo, con historial. El original se conserva y la foto quedará omitida para revisión."):qsTr("El original se moverá a Descartadas_catalogador con un registro de recuperación. Si está registrada, también se retirarán su fila y copia de Entrega_Museo. No se elimina definitivamente. ¿Continuar?"); color: Theme.body; wrapMode: Text.Wrap }
        onAccepted: {page.dirty=false;if(page.removal==="withdraw")catalog.withdraw(true);else catalog.discard(true)}
    }
    Dialog {
        id: gpsConfirm; parent: Overlay.overlay; anchors.centerIn: parent; implicitWidth: 560; implicitHeight: 260; width: Math.min(implicitWidth,parent.width-64); height: implicitHeight; modal: true; title: qsTr("Transcribir GPS de cámara"); standardButtons: Dialog.Ok|Dialog.Cancel
        contentItem: Label { text: qsTr("Confirma que estas coordenadas proceden de la cámara. Se añadirá GPS a la copia JPEG sin recomprimir, con respaldo e historial; el original y el Excel del Museo se conservan."); color: Theme.body; wrapMode: Text.Wrap }
        onAccepted: catalog.manualGps([gpsLatitude.text,gpsLongitude.text,gpsAltitude.text,gpsAccuracy.text])
    }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+S":"Ctrl+S"; enabled: page.editable; onActivated: page.savePhoto() }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+O":"Ctrl+O"; enabled: !catalog.busy; onActivated: catalogPicker.open() }
    Shortcut { sequence: "Ctrl+PgDown"; enabled: !catalog.busy; onActivated: page.step(1) }
    Shortcut { sequence: "Ctrl+PgUp"; enabled: !catalog.busy; onActivated: page.step(-1) }
    Shortcut { sequences: [StandardKey.ZoomIn,Qt.platform.os==="osx"?"Meta+=":"Ctrl+="]; enabled: page.zoomKeysAllowed(); onActivated: preview.zoomAt(page.zoom*1.25,preview.width/2,preview.height/2) }
    Shortcut { sequence: StandardKey.ZoomOut; enabled: page.zoomKeysAllowed(); onActivated: preview.zoomAt(page.zoom/1.25,preview.width/2,preview.height/2) }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+0":"Ctrl+0"; enabled: page.zoomKeysAllowed(); onActivated: page.fitView() }
    Shortcut { sequence: Qt.platform.os==="osx"?"Meta+1":"Ctrl+1"; enabled: page.zoomKeysAllowed(); onActivated: preview.zoomAt(1/preview.fitScale,preview.width/2,preview.height/2) }
}
