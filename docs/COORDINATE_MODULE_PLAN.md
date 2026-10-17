# Conversor: revisión del original y ampliación

## Más sistemas y exploración — incremento 0.5.3

Selector con grupos de Costa Rica, Centroamérica, Sudamérica, geográficos,
WGS84/UTM Norte y Sur, y cartografía mundial. Incluye las 120 zonas WGS84/UTM,
Ocotepeque regional, SIRGAS 2000, MAGNA-SIRGAS, POSGAR 2007, PSAD56 y NAD27.
La búsqueda general existente se conserva y permite avanzar más allá de las
primeras 100 coincidencias. Buscar desde Recomendados consulta toda la base;
en los demás grupos, filtra dentro del grupo seleccionado. Editar el término
reinicia la paginación para no saltarse la primera página de una búsqueda nueva.

Definiciones y validación: `CRS_REVIEW.md`. No cambia la matemática de conversión,
no se añaden bibliotecas ni se habilitan aproximaciones/rejillas de red. Cada
selección mantiene información de ejes/unidades/datum y las restricciones 2D.

## Exportación segura — incremento 0.5.2

CSV neutraliza texto de aspecto de fórmula, incluidos encabezados e identificadores.
Los números con signo válidos siguen siendo números. XLSX recibe los valores sin
esa marca de CSV y los guarda como texto literal cuando corresponde; JSON conserva
los originales. La conversión geodésica no cambia. Véase `SECURITY.md` para límites.

## Claridad de unidades — incremento 0.5.1

La interfaz determina si origen y destino son geográficos por separado mediante
`geographic` y las unidades reales que devuelve PROJ. Un origen proyectado muestra
Este/Norte y sus unidades; para `metre`, «Metros (m)». No se supone que todo CRS
proyectado use metros: otras unidades mantienen su nombre de la base instalada.
Los selectores DD/DM/DMS sólo aparecen en el lado geográfico, con nombres completos.
La etiqueta visible se desacopla del código del protocolo; un selector angular
oculto no aplica su selección previa a las coordenadas lineales. Se distinguen
separador decimal de entrada, decimales de salida y unidades en la tabla de resultados.
La unidad se elige mediante el CRS, no convirtiendo manualmente metros a grados.

No cambian fórmulas, definiciones EPSG, operaciones, restricciones ni exportaciones.
`tools/verify_coordinate_units.py` cubre los formatos de grados, metros en ambos
sentidos, unidades lineales distintas, tamaños compactos, accesibilidad y la
conversión real con coma decimal. Los controles geodésicos generales continúan en
`tools/verify_coordinates.py`.

Fuente indicada por el usuario: https://github.com/ADOCR/Conversor-coordenadas.
Revisión consultada: `87c8532aa94e022cf7a4f11c46e5f4d14855f5c9`, 2025-02-06.
Acceso mediante Git ya configurado; no se publicaron datos ni cambios.

## Evidencia y conservación

El árbol tiene tres archivos: README (23 bytes), `Conversor de coordenadas.py`
(0 bytes) y `Conversor de coordenas_info.md` (4.320 bytes). La única rama publicada
es main; las dos revisiones disponibles sólo añaden esos archivos. La documentación
describe Python/Tkinter, pyproj, GD↔GMS, hemisferios, coma decimal, precisión de
formato y conversión opcional a CRTM05. Son funciones documentadas, no probadas.
No se puede determinar EPSG, operación geodésica o validaciones a partir de código
que falta. No ejecutar el archivo vacío ni registrar una app funcional ficticia.

Mantener esos requisitos, obtener otra copia si existe y preservar su comportamiento
con fixtures antes de reemplazarla. La instalación automática de pyproj descrita
se sustituye por diagnóstico y configuración explícitos, conforme al requisito del Hub.

## Alcance ampliado por incrementos

1. **Formatos:** grados decimales, grados/minutos/segundos y grados/minutos decimales;
   hemisferios, copiar/pegar, coma o punto decimal según perfil regional explícito.
   Rechazar signos/cardinales contradictorios, NaN/Inf y minutos/segundos fuera de rango.
   Redondear con acarreo correcto. Cambiar formato no cambia datum ni CRS.
2. **CRS:** origen/destino buscables por EPSG/nombre, favoritos y recientes. Pines
   Costa Rica: CR05/CRTM05, CR-SIRGAS/CRTM05, WGS84 geográficas, UTM 16N/17N,
   Ocotepeque/Lambert Norte y Sur. Conversión inversa y entre proyectadas. CRTM90
   permanece pendiente de definición; no asignarle parámetros inventados.
3. **Lotes:** pegar tabla e importar CSV/TXT con previsualización de separador,
   decimal, columnas, ejes y CRS. Conservar ID de punto, sitio, operación, contexto
   y columnas originales. Ejecutar en background, cancelar, exportar resultado y
   reporte de errores por fila sin sobrescribir el archivo original.
4. **Archivos adicionales:** XLSX con elección de hoja/columnas; GeoJSON con contrato
   explícito de coordenadas de exportación; GeoPackage/GIS posteriores, sin cargar
   GDAL para convertir un par de números. No añadir una suite GIS completa ahora.
5. **Procedencia:** guardar por operación CRS, datum/época, convención de ejes,
   versión de PROJ/EPSG, pipeline/grids y exactitud declarada. Presets por proyecto
   deben confirmarse al importar; nunca adivinar CRS sólo por el rango numérico.

## Arquitectura y UX

Servicio C++/PROJ aislado y lazy; parsers de formatos independientes del motor.
Contextos PROJ por worker y validación de operaciones, áreas y grids offline.
El Hub no carga PROJ al abrir Inicio. Un pyproj recuperado puede servir como
implementación temporal/comparación; no requiere Python embebido.

Aplicar DESIGN.md: tabla sólida, encabezado/acciones compactos, selector CRS con
búsqueda e información progresiva, números monoespaciados, ejes/unidades visibles,
errores por fila con texto y estado. Pegar/importar/preset arriba; convertir como
acción principal; exportación contextual. Vidrio limitado a navegación/transitorios.
Un mapa es opcional posterior; no descargar mapas ni enviar coordenadas sin configuración.

## Pruebas previas a publicar

- GD/GMS/GDM: hemisferios, cero negativo, límites, acarreo, redondeo y formatos regionales.
- Ejes oficiales y convención UI X/Y, incluidos EPSG:5367 frente a 8908.
- Puntos de control independientes por transformación; ida/vuelta es insuficiente.
- CRS/datum desconocido, zona UTM incorrecta, fuera de área y grids ausentes.
- CSV con coma decimal y delimitador distinto, datos incompletos, Unicode y IDs duplicados.
- Cancelación/publicación atómica, archivo destino existente y disco desconectado.
- Ninguna transformación vertical de Z/DEM sin datum y operación vertical verificados.

El [CRS_REVIEW.md](CRS_REVIEW.md) conserva las definiciones consultadas. La precisión
mostrada no promete precisión de levantamiento ni corrige un datum mal identificado.
