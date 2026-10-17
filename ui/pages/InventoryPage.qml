import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../themes"
import "../components"

ColumnLayout {
    id: page; objectName: "inventoryWorkspace"; spacing: 10
    property var stats: inventory.state.stats || ({})
    function openEntry(){entryDialog.open()}
    RowLayout { Layout.fillWidth: true
        Symbol { name: "tools"; tint: Theme.accent }
        Label { text: qsTr("Inventario de materiales"); color: Theme.ink; font.pixelSize: Theme.h1; font.weight: Font.DemiBold; Layout.fillWidth: true }
    }
    GlassSurface { Layout.fillWidth: true; implicitHeight: toolbar.implicitHeight+20
        Flow { id: toolbar; anchors.fill: parent; anchors.margins: 10; spacing: 8
            ActionButton { text: qsTr("Nuevo inventario…"); enabled: !inventory.busy; onClicked: createDialog.open() }
            ActionButton { text: qsTr("Cargar inventario…"); symbol: "folder"; enabled: !inventory.busy; onClicked: openFile.open() }
            ActionButton { text: qsTr("Importar tabla…"); enabled: inventory.opened&&!inventory.busy; onClicked: sourceFile.open() }
            ActionButton { text: qsTr("Ingresar material…"); variant: "primary"; enabled: inventory.opened&&!inventory.busy; onClicked: entryDialog.open() }
            ActionButton { text: qsTr("Exportar…"); enabled: inventory.opened&&!inventory.busy; onClicked: exportFile.open() }
            ActionButton { text: qsTr("Cancelar"); visible: inventory.busy; onClicked: inventory.cancel() }
        }
    }
    Label { text: inventory.databasePath || qsTr("Crea un inventario SQLite o carga uno existente. Importa una copia normalizada del trabajo anterior, después de revisar la vista previa."); textFormat: Text.PlainText; wrapMode: Text.Wrap; color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true }
    Label { visible: inventory.opened; text: qsTr("%1 registros · %2 analizables · %3 excluidos · %4 duplicados · %5 piezas analizables").arg(page.stats.records||0).arg(page.stats.usable||0).arg(page.stats.excluded||0).arg(page.stats.duplicates||0).arg(page.stats.quantity||0); color: Theme.body; wrapMode: Text.Wrap; Layout.fillWidth: true }
    Label { visible: inventory.opened; text: qsTr("Envases distintos: %1 cajas · %2 bolsas grandes · %3 bolsitas. Los conteos de material y de envases se mantienen separados.").arg((inventory.state.containers||{}).box||0).arg((inventory.state.containers||{}).large_bag||0).arg((inventory.state.containers||{}).small_bag||0); color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: Theme.caption; Layout.fillWidth: true }
    Label { text: inventory.message; textFormat: Text.PlainText; color: inventory.error?Theme.error:Theme.body; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: Theme.caption }
    ProgressBar { visible: inventory.busy; indeterminate: true; Layout.fillWidth: true; Layout.preferredHeight: 4 }
    RowLayout { visible: inventory.opened; Layout.fillWidth: true
        AppField { id: query; placeholderText: qsTr("Buscar contexto, envase, material o registro"); Layout.fillWidth: true; onAccepted: inventory.search(text,onlyPending.checked) }
        CheckBox { id: onlyPending; text: qsTr("Excluidos / pendientes"); font.pixelSize: Theme.caption; onToggled: if(!inventory.busy)inventory.search(query.text,checked) }
        ActionButton { text: qsTr("Buscar"); enabled: !inventory.busy; onClicked: inventory.search(query.text,onlyPending.checked) }
    }
    ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: inventory.rows; ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate { required property var modelData; width: ListView.view.width; height: 76
            contentItem: ColumnLayout { spacing: 3
                RowLayout { Layout.fillWidth: true
                    Label { text: modelData.registro; textFormat: Text.PlainText; color: Theme.ink; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                    Label { text: modelData.cantidad_analisis===null||modelData.cantidad_analisis===undefined?qsTr("Excluido"):qsTr("%1 unidades").arg(modelData.cantidad_analisis); color: Theme.accent; font.pixelSize: Theme.caption }
                }
                Label { text: qsTr("Op. %1 · Pozo %2 · Nivel %3 · Caja %4 / Bolsa %5 / Bolsita %6").arg(modelData.operacion||"—").arg(modelData.pozo||"—").arg(modelData.nivel_std||"—").arg(modelData.caja||"—").arg(modelData.bolsa_grande||"—").arg(modelData.bolsita||"—"); textFormat: Text.PlainText; color: Theme.body; font.pixelSize: Theme.caption; Layout.fillWidth: true; elide: Text.ElideRight }
                Label { text: modelData.cantidad_estado||""; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true; elide: Text.ElideRight }
            }
            onClicked: { detail.text=JSON.stringify(modelData,null,2); detailDialog.open() }
        }
        Label { anchors.centerIn: parent; visible: parent.count===0; text: inventory.opened?qsTr("Todavía no hay registros que mostrar."):qsTr("Tus materiales, su contexto y su recorrido, en un mismo inventario."); color: Theme.muted; width: parent.width-32; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter }
    }
    RowLayout { visible: inventory.opened; Layout.fillWidth: true
        ActionButton { text: qsTr("Anterior"); enabled: !inventory.busy&&(inventory.state.page||0)>0; onClicked: inventory.search(query.text,onlyPending.checked,inventory.state.page-1) }
        Label { text: qsTr("Página %1 de %2 · %3 resultados").arg((inventory.state.page||0)+1).arg(inventory.state.pages||1).arg(inventory.state.filtered||0); color: Theme.muted; font.pixelSize: Theme.caption; Layout.fillWidth: true }
        ActionButton { text: qsTr("Siguiente"); enabled: !inventory.busy&&(inventory.state.page||0)+1<(inventory.state.pages||1); onClicked: inventory.search(query.text,onlyPending.checked,inventory.state.page+1) }
    }
    Label { text: qsTr("Exporta Inventario, Análisis, Pendientes y Procedencia a Excel, o todos los registros a CSV. La preparación MNCR conserva los tipos de colección separados; la entrega oficial necesita su plantilla digital, sin alterar columnas."); color: Theme.muted; font.pixelSize: Theme.caption; wrapMode: Text.Wrap; Layout.fillWidth: true }

    Connections { target: inventory; function onCompleted(ok) { if(ok&&Object.keys(inventory.preview).length>0) previewDialog.open(); if(ok&&entryDialog.saving){
        entryDialog.close();entryDialog.saving=false;
        for(let i=0;i<counts.count;i++)counts.itemAt(i).value="";
        for(let i of [10,12,13])entryFields.itemAt(i).value="";
    } else if(!ok)entryDialog.saving=false } }
    FileDialog { id: openFile; title: qsTr("Cargar inventario"); nameFilters: [qsTr("Inventario ArchaeoHub (*.ahinv)")]; onAccepted: inventory.open(selectedFile) }
    FileDialog { id: newFile; title: qsTr("Guardar nuevo inventario"); fileMode: FileDialog.SaveFile; nameFilters: [qsTr("Inventario ArchaeoHub (*.ahinv)")]; defaultSuffix: "ahinv"; onAccepted: inventory.create(selectedFile,{project:projectName.text,site:siteCode.text}) }
    FileDialog { id: sourceFile; title: qsTr("Revisar fuente normalizada"); nameFilters: [qsTr("Tablas normalizadas (*.json *.xlsx *.csv)")]; onAccepted: inventory.reviewImport(selectedFile) }
    FileDialog { id: exportFile; title: qsTr("Exportar sin reemplazar archivos"); fileMode: FileDialog.SaveFile; nameFilters: [qsTr("Excel (*.xlsx)"),qsTr("CSV (*.csv)")]; defaultSuffix: selectedNameFilter.index===0?"xlsx":"csv"; onAccepted: inventory.exportFile(selectedFile) }
    Dialog { id: createDialog; title: qsTr("Nuevo inventario"); modal: true; anchors.centerIn: Overlay.overlay; width: Math.min(500,page.width); standardButtons: Dialog.Ok|Dialog.Cancel
        ColumnLayout { anchors.fill: parent
            Label { text: qsTr("Proyecto"); color: Theme.body }
            AppField { id: projectName; Layout.fillWidth: true }
            Label { text: qsTr("Código del sitio"); color: Theme.body }
            AppField { id: siteCode; Layout.fillWidth: true }
        }
        onAccepted: newFile.open()
    }
    Dialog { id: previewDialog; title: qsTr("Revisar importación"); modal: true; anchors.centerIn: Overlay.overlay; width: Math.min(660,page.width); standardButtons: Dialog.Cancel
        ColumnLayout { anchors.fill: parent; spacing: 12
            Label { text: inventory.preview.source||""; textFormat: Text.PlainText; wrapMode: Text.Wrap; Layout.fillWidth: true; color: Theme.muted }
            Label { text: qsTr("%1 registros · %2 analizables · %3 discrepancias · %4 duplicados\nCantidad analizable: %5").arg(inventory.preview.records||0).arg(inventory.preview.usable||0).arg(inventory.preview.pending||0).arg(inventory.preview.duplicates||0).arg(inventory.preview.quantity||0); color: Theme.body; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Label { text: qsTr("Se conservarán los valores originales y el desglose; no se reclasifican descripciones. Si la fuente cambia, debes revisarla otra vez."); wrapMode: Text.Wrap; Layout.fillWidth: true; color: Theme.body }
            Label { visible: inventory.preview.alreadyImported||(inventory.preview.collisions||[]).length>0; text: qsTr("Esta fuente ya se importó o sus identificadores colisionan. No se reemplazarán registros."); color: Theme.error; wrapMode: Text.Wrap; Layout.fillWidth: true }
            ActionButton { text: qsTr("Confirmar importación"); variant: "primary"; enabled: !inventory.busy&&!inventory.preview.alreadyImported&&(inventory.preview.collisions||[]).length===0; onClicked: { previewDialog.close();inventory.importReviewed() } }
        }
    }
    Dialog { id: detailDialog; title: qsTr("Registro y trazabilidad"); modal: true; anchors.centerIn: Overlay.overlay; width: Math.min(760,page.width); height: Math.min(580,page.height); standardButtons: Dialog.Close
        ScrollView { anchors.fill: parent; TextArea { id: detail; readOnly: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: Theme.body; font.pixelSize: Theme.caption } }
    }
    Dialog { id: entryDialog; property bool saving: false; title: qsTr("Ingreso de material fragmentario"); modal: true; anchors.centerIn: Overlay.overlay; width: Math.min(760,page.width); height: Math.min(650,page.height); standardButtons: Dialog.Close
        contentItem: ScrollView { clip: true; contentWidth: availableWidth; enabled: !inventory.busy
            ColumnLayout { width: parent.width; spacing: 10
                Label { text: qsTr("Cada ingreso representa un contexto y envase. Cantidad cuenta unidades de material, no bolsitas. No se infieren profundidades."); color: Theme.body; Layout.fillWidth: true; wrapMode: Text.Wrap }
                GridLayout { columns: 2; Layout.fillWidth: true
                    Repeater { id: entryFields; model: [qsTr("Operación"),qsTr("Pozo"),qsTr("Contexto"),qsTr("Trinchera"),qsTr("Unidad"),qsTr("Nivel / intervalo"),qsTr("Desde (cm)"),qsTr("Hasta (cm)"),qsTr("Caja"),qsTr("Bolsa grande"),qsTr("Bolsita"),qsTr("Fecha AAAA-MM-DD"),qsTr("Cantidad original"),qsTr("Comentarios")]
                        delegate: ColumnLayout { required property string modelData; Layout.fillWidth: true; property alias value: input.text
                            Label { text: modelData; color: Theme.muted; font.pixelSize: Theme.caption }
                            AppField { id: input; Layout.fillWidth: true; Accessible.name: modelData }
                        }
                    }
                }
                Label { text: qsTr("Desglose explícito por material (vacío = no determinado, 0 = ausencia contada)"); color: Theme.body; Layout.fillWidth: true; wrapMode: Text.Wrap }
                GridLayout { columns: 2; Layout.fillWidth: true
                    Repeater { id: counts; model: [qsTr("Arcilla"),qsTr("Arcilla cocida"),qsTr("Cerámica"),qsTr("Indeterminado"),qsTr("Lítica"),qsTr("Material constructivo"),qsTr("Muestra"),qsTr("Vidrio")]
                        delegate: ColumnLayout { required property string modelData; Layout.fillWidth: true; property alias value: input.text
                            Label { text: modelData; color: Theme.muted; font.pixelSize: Theme.caption }
                            AppField { id: input; Layout.fillWidth: true; Accessible.name: modelData }
                        }
                    }
                }
            Label { visible: inventory.error; text: inventory.message; textFormat: Text.PlainText; color: Theme.error; wrapMode: Text.Wrap; Layout.fillWidth: true }
                ActionButton { text: qsTr("Guardar ingreso"); variant: "primary"; enabled: !inventory.busy
                    onClicked: {
                        let row={};let keys=["operacion","pozo","contexto","trinchera","unidad","nivel_std","prof_desde_cm","prof_hasta_cm","caja","bolsa_grande","bolsita","fecha","cantidad_original","comentarios_laboratorio"];
                        for(let i=0;i<keys.length;i++)row[keys[i]]=entryFields.itemAt(i).value;
                        keys=["q_arc","q_arc_coc","q_cer","q_ind","q_lit","q_const","q_muestra","q_vidrio"];
                        for(let i=0;i<keys.length;i++)row[keys[i]]=counts.itemAt(i).value;
                        entryDialog.saving=true;inventory.add(row)
                    }
                }
            }
        }
    }
}
