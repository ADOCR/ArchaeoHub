# Revisión geodésica

## Catálogo ampliado — incremento 0.5.3

La selección dispone de ocho grupos curados y de exploración paginada de la base
EPSG completa. Los ocho grupos reúnen **201 CRS distintos**. Los grupos son accesos a definiciones existentes, no fórmulas ni
realizaciones de datum creadas por el Hub. Los grupos pueden compartir códigos.

| Grupo | Definiciones | Contenido |
|---|---:|---|
| Recomendados | 25 | Costa Rica, WGS84, principales datums americanos y Mercator |
| Costa Rica | 12 | CR05, CR-SIRGAS, CRTM05, UTM 16N/17N y Ocotepeque geográfico/Lambert |
| Centroamérica | 21 | Guatemala Norte/Sur, El Salvador, Nicaragua Norte/Sur, Panamá, NAD27/UTM y referencias regionales |
| Sudamérica | 46 | SIRGAS 2000 geográfico/UTM, MAGNA-SIRGAS, POSGAR 2007 y PSAD56 geográfico/UTM |
| Geográficos | 15 | Longitud/latitud en datums verificados, incluidos NAD27, NAD83, SAD69 y ETRS89 |
| WGS84 / UTM Norte | 60 | EPSG:32601–32660, zonas 1–60N |
| WGS84 / UTM Sur | 60 | EPSG:32701–32760, zonas 1–60S |
| Cartografía mundial/otros países | 10 | WGS84, Web Mercator, World Mercator, ETRS89/UTM, British National Grid, Lambert-93 y NAD83/Conus Albers |

Las listas de identificadores se mantienen en `modules/coordinates/CrsCatalog.h`.
Datum, elipsoide, unidades, ejes, área y operaciones proceden exclusivamente del
PROJ privado incluido (9.8.1, EPSG v12.029). POSGAR 2007 horizontal es **EPSG:5340**;
5342 es 3D y se rechaza. Ocotepeque geográfico es **EPSG:5451**. No confundir esos
códigos con proyecciones del mismo datum. La base revisada se puede contrastar con
los SQL oficiales de [CRS geográficos](https://raw.githubusercontent.com/OSGeo/PROJ/9.8.1/data/sql/geodetic_crs.sql)
y [proyectados](https://raw.githubusercontent.com/OSGeo/PROJ/9.8.1/data/sql/projected_crs.sql).

«Toda la base EPSG» permite explorar/buscar por nombre o código con páginas de
100 resultados y sin duplicar un código que tenga varias áreas de uso en la base.
El filtrado inicial excluye retirados, no terrestres y tipos no 2D;
al seleccionar se verifican además los ejes, unidades y datum dinámico. Que un CRS
figure en la base no garantiza que toda transformación desde/hacia él esté disponible
sin conexión. Se mantienen `ALLOW_BALLPARK=NO`, `ONLY_BEST=YES`, áreas de uso y
procedencia. No se añaden descargas de rejillas ni se rebaja la precisión exigida.
CRTM90 y transformaciones verticales/dinámicas siguen fuera del alcance.

`tools/verify_crs_catalog.py` valida los metadatos de todos los accesos curados,
meridianos centrales/unidades de las 120 zonas WGS84/UTM, nueve pares regionales
y mundiales mediante controles/ida-vuelta y comparación con pyproj, paginación y
rechazo de filtros/hemisferios/CRS 3D inválidos. pyproj comparte PROJ; no se presenta
como una implementación matemática independiente ni como exactitud de campo.

## Implementación inicial — incremento 0.2

Se implementaron transformaciones horizontales 2D mediante PROJ 9.8.1 en un proceso
privado con red apagada. Se probaron controles sintéticos; no se procesaron
coordenadas de campo ni se certificó la precisión de un levantamiento real.

`tools/verify_coordinates.py` verifica el control publicado de Transverse Mercator
de la [nota IOGP/EPSG 7-2](https://www.iogp.org/bookstore/wp-content/uploads/sites/2/woocommerce_uploads/2017/01/373-07-02.pdf),
controles en meridiano central CRTM05/UTM, ejes/unidades, nueve pares con un entorno
pyproj 3.7.2/PROJ 9.5.1 separado, ida/vuelta, lotes y rechazo de datos inválidos.
La comparación de versiones **no es un motor matemático independiente**. Se
documenta precisión declarada por operación, sin convertir decimales en exactitud.
La tolerancia del ejemplo publicado es 2 cm porque sus valores están redondeados;
no implica que WGS84↔CRTM05 tenga exactitud de 2 cm para todos los datos.

**Discrepancia de fuentes:** la tabla de códigos de la norma técnica SNIT/IGN 2023
intercambia etiquetas de CRS 2D/3D. No se cambia el tipo de un EPSG por esa tabla:
5365/8907 son geographic 2D; 5364/8906 son geographic 3D en la base local y en el
[SQL oficial de PROJ 9.8.1](https://raw.githubusercontent.com/OSGeo/PROJ/9.8.1/data/sql/geodetic_crs.sql).
El programa consulta los tipos reales y rechaza 3D/dinámicos sin época. El documento
SNIT se conserva como referencia normativa, sin usar su tabla como parámetros:
[marco geodésico 2023](https://www.snitcr.go.cr/pdfs/normativa_tecnica/NTIG_CR01_06_2023%20V2%20MARCO%20DE%20REFERENCIA%20GEOD%C3%89SICO%20DE%20COSTA%20RICA.pdf).

## Evidencia

Base PROJ de QGIS local, consultada con SQLite en modo lectura y `projinfo` con
`PROJ_NETWORK=OFF`. Metadata de la base: PROJ 9.8.1, PROJ_DATA 1.24, EPSG v12.029
(2025-10-02). Se guardaron PROJJSON y metadata en `audit/local/`.
El paquete copia su propia versión de PROJ/base y dependencias, sin depender de
que QGIS esté instalado. Cada cambio de versión requiere repetir la validación.

| EPSG | Nombre en la base verificada | Datum / elipsoide | Ejes oficiales y unidades |
|---|---|---|---|
| 4326 | WGS 84 | Ensemble WGS 84 / WGS 84 | Latitud, longitud; grados |
| 5367 | CR05 / CRTM05 | Costa Rica 2005 / WGS 84 | **Norte, Este**; metros |
| 8907 | CR-SIRGAS epoch 2014.59 | CR-SIRGAS epoch 2014.59 / GRS 1980 | Latitud, longitud; grados |
| 8908 | CR-SIRGAS epoch 2014.59 / CRTM05 | CR-SIRGAS epoch 2014.59 / GRS 1980 | **Este, Norte**; metros |
| 5456 | Ocotepeque 1935 / Costa Rica Norte | Ocotepeque 1935 / Clarke 1866 | Este, Norte; metros |
| 5457 | Ocotepeque 1935 / Costa Rica Sur | Ocotepeque 1935 / Clarke 1866 | Este, Norte; metros |
| 32616 | WGS 84 / UTM zone 16N | Ensemble WGS 84 / WGS 84 | Este, Norte; metros |
| 32617 | WGS 84 / UTM zone 17N | Ensemble WGS 84 / WGS 84 | Este, Norte; metros |

Las dos entradas CRTM05 no son intercambiables por compartir proyección/nombre
abreviado. Se conserva la realización/época del datum; no fabricar una corrección
temporal. Elegir zona UTM por ubicación/área de uso y declaración de los datos,
no establecer una sola zona para todo el país por defecto.

**CRTM90:** no se identificó una definición inequívoca bajo ese nombre en los
registros consultados. Esto no demuestra que el sistema no exista. Solicitar
documentación cartográfica/definición WKT y autoridad de origen antes de asignar
EPSG, datum, parámetros o transformaciones. Estado inicial: pendiente/no convertible.

## Contrato antes de implementar

UI con campos nombrados `X / Este / Longitud` y `Y / Norte / Latitud` según CRS,
datum, unidades y código completos. La convención de entrada de la aplicación será
explícita X/Y; cualquier adaptación al orden oficial debe probarse por separado.

Usar definiciones de autoridad, contextos PROJ por worker y una selección de
operaciones documentada. Verificar área de uso, grids locales y precisión; rechazar
aproximaciones no autorizadas. La normalización para visualización cambia el orden
de interfaz, no la definición del CRS. Ver [API PROJ](https://proj.org/en/stable/development/reference/functions.html).

Para cada par soportado, registrar versión de base, operación/pipeline escogida,
grids/checksums, precisión declarada, ejes y límites; probar puntos de control
independientes, no sólo ida/vuelta. La exactitud de la transformación no corrige
una medición GPS imprecisa ni un datum de entrada mal identificado.

No descargar grids ni efectuar consultas de coordenadas a servicios externos sin
configuración explícita. Diferenciar altura elipsoidal, ortométrica y valor DEM:
no transformar el campo Z ni atribuirle datum vertical sin evidencia.

Los scripts existentes que usan `always_xy=True` u orden GIS tradicional explican
una convención de llamadas, no validan automáticamente la operación elegida. El
reporte EXIF del catalogador también requiere conservar su datum/procedencia y no
reinterpretar como coordenada de pozo la ubicación de la cámara.
