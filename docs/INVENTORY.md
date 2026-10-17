# Inventario de materiales — primer incremento 0.4

Módulo nativo QML/C++ con dominio Python privado SQLite/openpyxl. Se carga al usarlo,
sin otro Python instalado y sin importar código de los scripts históricos. Su base
`.ahinv` es independiente de `workbench.sqlite` y no está dentro de la instalación.

## Comportamiento conservado

El diccionario aportado y el inventario normalizado de 14/10/2026 son la referencia.
Importación JSON (`inv_headers` + `inventario`, o filas objeto), CSV UTF-8 o XLSX
(hoja Inventario/Registros, encabezados normalizados, valores sin fórmulas).
No interpreta otra vez el lenguaje libre de laboratorio ni procesa directamente
libros crudos con títulos/encabezados distintos. Los originales se conservan.

Antes de confirmar: muestra registros, cantidad analizable, excluidos, discrepancias,
duplicados y colisiones. Verifica SHA256 al aplicar; una fuente repetida o ID repetido
aborta toda la transacción. Cada fila conserva todos sus campos y una copia cruda,
hoja/fila fuente, ruta/hash del archivo y fecha de importación. No se reclasifican
automáticamente artefactos, muestras o colecciones MNCR a partir de una abreviatura.

`q_arc` Arcilla; `q_arc_coc` Arcilla cocida; `q_cer` Cerámica; `q_ind` Indeterminado;
`q_lit` Lítica; `q_const` Material constructivo; `q_muestra` Muestra; `q_vidrio` Vidrio.
`n_*` son cantidades descritas/asignadas por subtipo. No sumar como verificadas sin
aplicar `usar_analisis`; los duplicados tampoco se suman. `Ind.` del manual MNCR
significa individuo y no debe confundirse con `q_ind`.

Cantidad original, desglose, diferencia y cantidad analizable son distintos. Vacío
no es cero. Un ingreso nuevo con desglose explícito diferente del original queda
pendiente y excluido; el desglose declarado se conserva. Un desglose explícito sin
cantidad original puede producir cantidad derivada, con estado que lo identifica.
No se adivinan conteos desde comentarios. Las unidades de peso de colecciones
especializadas no se deben mezclar con conteos de piezas.

Niveles combinados se conservan juntos. No se infieren profundidades: 20 cm para
pozos y 10 cm para trincheras correspondían al proyecto histórico, no a una regla
global. Coordenadas y altitud se conservan con sus campos de procedencia; no se
inventa datum vertical ni georreferencia faltante.

## Estructura y uso

Metadata de proyecto/sitio, fuentes, contextos, envases jerárquicos, registros,
materiales en formato largo y auditoría de ingreso. PK/FK y transacciones SQLite;
`application_id` y `user_version` rechazan bases ajenas/futuras. Envases se cuentan
por ID jerárquico distinto, separados de piezas; números con ceros iniciales se
conservan como texto. No contar un total de bolsitas repetido en varias filas.

UI: Nuevo/Cargar inventario, revisar/importar tabla, ingreso fragmentario por
contexto/caja/bolsa/bolsita, búsqueda, filtro de excluidos, paginación de 100 filas,
detalle completo, export CSV/XLSX. Los pendientes importados se inspeccionan y
exportan: su corrección auditada dentro de la UI es trabajo posterior, no un
botón ficticio. Tampoco hay borrado, traslados de almacén o etiquetas QR aún.

Excel: Resumen, Análisis por operación/pozo/nivel, Inventario completo, Pendientes,
Procedencia, Proyecto y requisitos MNCR. CSV conserva todos los campos; valores
complejos se serializan como JSON. Texto con prefijos de fórmula se neutraliza;
números permanecen numéricos. No sustituye archivos existentes. Cancelación antes
de commit/publicación conserva datos; no se certifica recuperación ante cortes de
energía o desconexión física. Un cierre forzado puede dejar una reserva de export
vacía: no se identifica como exportación terminada.

## MNCR

Manual aportado: Manual de normas y procedimientos para el tratamiento y manejo de
las colecciones arqueológicas con contexto, MNCR2016, pp.55–58,65, anexos3–8.
Seis formularios separados: fragmentarias, artefactos, restos humanos, fauna,
botánica, carbón. Fragmentaria marca presencia X por naturaleza, no estos conteos.
Orgánicos especializados no se agrupan automáticamente en Otros de fragmentaria.
El formato digital debe conservar las columnas oficiales; fechas DD/MM/YYYY.

El manual PDF no es el archivo Excel original. No se localizó una plantilla XLSX
oficial verificable. Esta entrega prepara datos/procedencia y una lista de requisitos;
NO genera una imitación de los formularios ni declara conformidad de recepción.
Pendiente: adaptador por plantilla digital oficial, campos especializados, validación
de obligatoriedad, pruebas de conservación de celdas/columnas y revisión del MNCR.
No presentar la hoja `MNCR - requisitos` como formulario oficial listo para entregar.
