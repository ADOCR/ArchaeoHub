# Arquitectura de ArchaeoHub

Referencia actual: 0.5.4 en Windows. Se conservan a continuación los incrementos
históricos y el diseño de evolución; README.md resume el alcance actual.

## Incremento 0.3.0

Las cuatro vistas son nativas del Hub, resueltas por IDs compilados de confianza.
TranscriptionController es un proxy ligero: sólo al abrir la página inicia el
worker C++ empaquetado. El worker conserva Controller/Exports/Parakeet anteriores
(ORIGIN.json) y emite estado/progreso/resultados por JSONL con prefijo propio.
Whisper/ONNX/CUDA y FFmpeg no se enlazan ni cargan en el host. Modelos persistentes
fuera de instalación; se reutiliza la ubicación del adaptador anterior cuando
existe, o se elige una carpeta/descarga explícita. Conserva identidad de ajustes e
historial. Tareas reales y cancelación; no ventanas secundarias para transcribir.

DocumentController usa un lector empaquetado independiente para PDF/Markdown/DOCX
y Qt Network para resumir por bloques mediante Ollama local. Nunca sigue redirects
ni utiliza un proxy: sólo 127.0.0.1. No carga un modelo al iniciar el Hub; tampoco
incluye Ollama ni pesos en el instalador. La lectura conserva localizadores y avisa
de páginas sin texto. El resumen anterior permanece si falla o se cancela el nuevo.
La exportación crea un archivo nuevo, separado de los documentos originales.

Excel usa un worker independiente con openpyxl ya disponible, sin depender del
módulo catálogo. Lee hojas/valores con límites de tamaño, filas, columnas y ZIP
expandido; no ejecuta fórmulas. C++ controla importación y conservación de entrada,
conversión PROJ y exportación. El .xlsx final se publica sin reemplazar un nombre
existente, desde un temporal hermano; incluye valores, errores y procedencia.

El catálogo distingue carga de creación. Inspecciona Excel/estado sin escribir ni
tomar un bloqueo; datos inferidos ambiguos requieren confirmación. Conserva las
transacciones del dominio y restaura los recorridos de la UI Python: reanudación,
guardar+siguiente, GPS al registrar, correcciones, omisión y descarte recuperable.
Un retiro confirmado seguido de resguardo fallido se informa como parcial y exige
reabrir; no se presenta como fallo sin cambios. Ver docs/CATALOG_PARITY_REVIEW.md.

El ejecutable Windows incorpora recurso ICO/versionado y asigna el icono a la
ventana. El instalador presenta aceptación de licencias de terceros y conserva
sus textos. Las dependencias de inferencia aumentan el tamaño del instalador;
no se agregaron pesos grandes ni datos arqueológicos.

## Incremento 0.2.0

- Workspaces nativos de Catálogo y Coordenadas cargados sólo por la ruta activa.
  Registry resuelve únicamente IDs compilados de manifiestos bundled; importar un
  manifiesto no permite cargar QML externo. Las vistas no procesan archivos pesados.
- CatalogController (C++): proceso persistente JSONL local, una petición a la vez,
  respuestas identificadas, límites, errores, tareas y caché de vistas previas.
  Worker Python congelado mantiene Catalog/CatalogLock 2.4.5 con AST idéntico;
  conserva Excel/JSON, backups, journals y bloqueo. Lectura de imágenes fuera de GUI.
  Una advertencia de commit obliga a reabrir antes de otra escritura. No se mata
  una transacción lenta por timeout: el cierre espera al fin de la tarea.
- CoordinateController (C++) + WorkbenchCoordinateDomain (Qt Core): formato, tablas,
  resultados virtualizados, exportación sin reemplazo y tareas cancelables.
  CoordinateWorker carga PROJ por QLibrary sólo en su proceso; RAII para contexto/PJ,
  red apagada, base/entorno privados, operaciones sin ballpark y procedencia real.
  El Hub no enlaza PROJ, GDAL ni Python. Cada solicitud usa su propio contexto.
- TaskManager admite trabajos externos además de hashes. Cancelar Coordenadas
  termina sólo su worker y conserva resultados anteriores. Catálogo difiere el
  cierre durante escrituras para proteger la recuperación.
- SQLite guarda cada tarea y su historial en una transacción, y cada inventario
  de volúmenes en otra transacción conjunta. Reduce commits síncronos en discos
  lentos y evita estados parciales. No se debilita la sincronización ni cambia
  el esquema. Si falla un inventario, se conserva el último snapshot válido.
- Datos del catálogo siguen en su carpeta, separados del SQLite del Hub. No se
  anuncia todavía una ABI binaria ni sandbox del sistema operativo para workers.

## Implementado en 0.1.0

- WorkbenchCore: Manifest, Paths/LocationRef, ProjectTemplates e IModule; sólo Qt Core.
- WorkbenchServices: Store SQLite, Settings, Logger JSONL, Volumes, Registry,
  TaskManager y ProcessModule. No enlaza motores de herramientas.
- ArchaeoHub: composición, Hub de presentación, modelos filtrados y QML separado
  en components, pages y themes. No hay SQL ni procesamiento pesado en QML.
- Registry valida manifiestos y permite importar metadatos tras revisión. Cambios
  de hash requieren revisar la instalación. No ejecuta código al descubrir.
- ProcessModule abre external/python/script sin shell implícita. Workspaces native
  compilados se resuelven aparte; future-plugin permanece pendiente de adaptador.
- TaskManager ejecuta SHA-256 con QtConcurrent, máximo dos, progreso/cancelación;
  guarda historial y marca tareas interrumpidas al reabrir. El lanzamiento de una
  herramienta externa no simula progreso ni finalización de su procesamiento.
- Volumes enumera fuera de GUI cada 10 s. GUID Windows probado; proveedores UUID
  Linux/macOS pendientes de pruebas allí. Espera el primer inventario antes de
  mostrar unidades conocidas como ausentes.
- ProjectTemplates valida un manifiesto de carpetas, crea un staging hermano y
  lo renombra al terminar, rechazando un destino existente. Hub lo ejecuta fuera
  de GUI y registra la referencia al completar. Un fallo elimina sólo carpetas
  vacías creadas por esa operación; no utiliza borrado recursivo.
- SQLite schema 1 y QSettings separado; los módulos mantienen sus propios datos.
- Sidebar según DESIGN.md: Inicio, Herramientas con sus módulos, Proyectos,
  Favoritos, Recientes, Actividad, Configuración y Ayuda. Ctrl/⌘+K abre acciones;
  sólo la página activa se instancia. No hay categorías vacías permanentes.
- Superficies translúcidas controladas y trabajo sólido. No se aplica aún blur
  de escritorio ni refracción nativa. La alternativa opaca es completamente usable.

El diseño de largo plazo que sigue conserva decisiones de Fase 0. Puertos más
especializados, plugins binarios, IPC, probes de versiones, aliases de volúmenes,
categorías avanzadas, ubicaciones recordadas y modo portable no están completos.
No deben interpretarse las propuestas como funciones entregadas.

En Windows el proveedor de volúmenes usa GetLogicalDriveStringsW, GetDriveTypeW,
GetVolumeInformationW, GetDiskFreeSpaceExW y GetVolumeNameForVolumeMountPointW.
Inventaría discos locales fijos/extraíbles; no consulta shares de red al arrancar.
La identidad sigue siendo el GUID, conservando las claves previas de QStorageInfo,
no la letra. Linux/macOS mantienen sus proveedores Qt pendientes de pruebas allí.
Montajes sólo en carpeta y redes requieren un incremento específico.

## Límites y dependencias

```text
app (composition root)
 ├─ ui / presentation ────────────────┐
 ├─ services (use cases, adapters) ────┤
 └─ module adapters ──────────────────┤
                                     v
                          core (contracts, values)

services ── persistence / filesystem / process / platform providers
module adapter ── own process, own dependencies and optional own data store
```

`core` usa C++20 y Qt Core, sin Quick, Widgets, SQL, Python, PROJ ni motores IA.
Contiene identificadores, DTO, estados, contratos y errores tipados. Los repositorios
y servicios implementan puertos del Core. `app` construye las dependencias y las
inyecta; no se usa un service locator global que permita a cualquier módulo tocar
cualquier servicio. Los módulos reciben capacidades mínimas en `ModuleContext`.

El Hub enlaza sólo Qt Core/Gui/Quick/QuickControls2/SQL/Network y lo necesario para
sus servicios. Concurrent puede apoyar trabajos CPU pequeños. Multimedia se añade
si existe un caso de reproducción real; no se incorpora a toda la app por anticipado.
Widgets permanece en herramientas heredadas; no es necesario para el shell QML.

Targets de evolución: `WorkbenchCore`, `WorkbenchServices`, `WorkbenchPersistence`,
`WorkbenchUi` y `WorkbenchApp`; cada módulo tiene su propio target/paquete. PROJ,
ONNX, Whisper, GDAL y OpenCV no son dependencias transitivas del ejecutable Hub.

## Carpetas y responsabilidades

```text
ArchaeoHub/
  app/                      main, composition root, identity/bootstrap
  core/
    modules/                descriptors, actions, lifecycle contracts
    tasks/                  requests, states, cancellation contracts
    locations/              VolumeId, LocationRef, typed paths
    projects/               project/resource references
  services/
    registry/               discovery, validation, approval, dependency probes
    execution/              process adapters and task scheduling
    persistence/            SQLite repositories and numbered migrations
    filesystem/             selection, hashes, watchers, open/reveal
    volumes/                platform identity providers and reconciliation
    configuration/          typed settings, profiles, import/export
    diagnostics/            structured logging and redaction
  ui/
    shell/                  sidebar, toolbar, routing, dashboard
    components/             accessible cards, panels, controls
    pages/                  tools, activity, settings, diagnostics
    themes/                 semantic tokens; light/dark/system
  modules/
    transcription/          descriptor and external/worker adapters
    coordinates/            later: isolated PROJ engine, tests and QML
    photo-catalog/          later: domain and independent data store
  plugins/                  trusted extension packages, not arbitrary scripts
  resources/
    branding/               display name, icons, palette
    translations/           Qt Linguist source/catalogues
  tests/                    core, services, integration, packaging
  docs/                     evidence, decisions and module authoring
  cmake/                    options, dependency discovery, deployment
  packaging/                Windows/Linux/macOS packaging definitions
  tools/                    source-audit/developer utilities
```

No se crea `shared/` inicialmente: tiende a convertirse en un contenedor sin
responsabilidad. Un contrato compartido va a Core; una implementación reutilizable
va a un servicio con un propósito concreto. Los directorios anteriores se crean
cuando tengan código, evitando una estructura vacía presentada como funcional.

## Identidad, configuración e internacionalización

Usar namespace `workbench` e identidad técnica estable separada de `displayName`.
El identificador de almacenamiento/instalación se fija antes del primer release;
cambiar nombre, logo o tema no debe cambiar rutas, plugins o claves de preferencias.
`branding.json` será un recurso de presentación, no una dependencia del dominio.

QStandardPaths resuelve datos/configuración/cache por sistema. QSettings administra
preferencias pequeñas: idioma, tema, movimiento/transparencia y geometría. SQLite
guarda relaciones y registros del Hub. Una preferencia tiene una única fuente de
verdad; no duplicar la misma clave en ambos sistemas. SettingsService valida tipos,
versiona importaciones y admite una raíz de prueba sin tocar el perfil real.

QML usa `qsTr`/`qsTrId`; los errores de servicios usan códigos y argumentos, traducidos
en presentación. Qt Linguist/lupdate/lrelease desde el primer build. Español inicial,
catálogos ampliables; nombres/código/API en inglés. Metadatos de módulos incluyen
nombres localizados con fallback, separados de sus IDs persistentes.

## UI y navegación

`NavigationService` gestiona rutas estables, argumentos validados, volver/adelante
y contexto de proyecto. Ejemplos: `home`, `tools`, `category/gis`,
`tool/<id>`, `favorites`, `recent`, `activity`, `settings/dependencies`.
La categoría filtra herramientas; no instancia sus motores ni borra su estado.
Los IDs de categoría son datos del registro, no una cadena de `if` por herramienta.

Las categorías Datos, SIG, Fotografía y 3D, Documentación, Laboratorio y Utilidades
se conservan como metadatos/filtros según DESIGN.md. Favoritos/Recientes son vistas; una herramienta puede pertenecer
a más de una categoría. Buscar filtra `ToolListModel` con roles explícitos.

Toolbar con búsqueda y contexto de proyecto. Tarjetas con descripción, estado,
versión, tecnología y acción principal; ruta/dependencias en detalles expandibles.
No mostrar un error técnico largo en cada tarjeta. Estados: disponible, unidad
no disponible, dependencia ausente, incompatible, requiere revisión, no encontrado.

Dashboard consulta repositorios: recientes, favoritos, proyectos/archivos usados,
tareas y estado de discos. Si aún no hay proyectos registrados, ofrece crearlos
sin inventar proyectos ni llenar métricas con ejemplos. Datos del disco muestran
fecha de observación; un snapshot no proporciona historial de crecimiento.

QML contiene composición, interacción y formato visual ligero. Los modelos de
presentación C++ exponen acciones y estados; no hacer SQL, hashing, recorridos,
transformaciones geodésicas ni procesos largos en JavaScript/QML.

El vidrio es una capa de tema con tokens semánticos. Fondos sólidos bajo texto,
contraste medido, navegación por teclado y etiquetas accesibles. Blur/transparencia
reducibles o desactivables; animación breve, respeta preferencia de movimiento.
No depender de efectos privados de Apple ni de un blur de escritorio idéntico en
todos los sistemas. La alternativa con paneles opacos sigue siendo una UI completa.

## Registro, módulos y Python bridge

ToolRegistry separa descriptor lógico, instalación concreta, estado de dependencia,
aprobación y ubicación. Descubrimiento lee metadatos, nunca ejecuta herramientas.
Los manifiestos permiten `native`, `python`, `external`, `script`, `future-plugin`;
el último se registra como no soportado hasta implementar su adaptador.

Dos niveles de extensión: procesos independientes (primera opción) y plugins Qt
de confianza con ABI controlada (posterior). Un proceso puede tener GUI propia o
un protocolo de jobs. Un plugin no debe obligar a que el Core conozca su dominio.
Ver [MODULES.md](MODULES.md) para lifecycle, seguridad y contrato propuesto.

Python bridge inicial = ProcessAdapter, no un Python embebido. Lanza intérprete o
paquete elegido con argumentos separados, directorio y entorno explícitos.
No instala paquetes ni ejecuta `pip` para comprobar disponibilidad. Probes pequeños
usan versiones/timeouts; importaciones pesadas sólo bajo demanda en el entorno
seleccionado, con alcance explicado y sin abrir datos de campo.

## Tareas, procesos y concurrencia

TaskManager posee trabajos con IDs, módulo, entradas por referencia, estados,
timestamps, progreso opcional, resultados y error tipado. Estados propuestos:
Queued, Starting, Running, Cancelling, Succeeded, Failed, Cancelled, Interrupted.
El progreso desconocido es indeterminado, no un porcentaje inventado.

Los workers trabajan fuera del hilo GUI. QProcess es asíncrono; tareas CPU/I/O
pequeñas usan pool limitado; procesos pesados se aíslan. Los módulos declaran
recursos (por ejemplo GPU exclusiva) y cancelación soportada. La cola inicial
limita concurrencia de motores IA para evitar agotar memoria.

Cancelación cooperativa primero; para workers propios, timeout y terminación del
árbol mediante mecanismos de plataforma. Nunca matar procesos por nombre global.
Una GUI externa sin protocolo no acepta cancelación de una transcripción desde
Hub ni se cierra a la fuerza al salir del Hub. Abrir una app y finalizar un trabajo
de dominio son eventos distintos. Interrupciones se registran; no se reintenta una
operación con efectos en archivos sin confirmar idempotencia y estado parcial.

El shutdown deja de aceptar trabajos, solicita cancelación donde corresponde y
espera asíncronamente. No bloquear el hilo GUI esperando `QThread::wait()` durante
minutos. Cada conexión SQLite pertenece a su hilo; los objetos QObject mantienen
afinidad correcta y actualizan modelos por señales encoladas.

## Persistencia

SQLite contiene exclusivamente metadatos del Hub; los módulos administran sus
propios datasets. Diseño detallado en [docs/DATABASE.md](docs/DATABASE.md).
Migraciones transaccionales numeradas, claves foráneas activadas por conexión,
sentencias preparadas y respaldos verificados antes de upgrades. Un solo servicio
serializa escrituras iniciales y conexiones de lectura tienen ciclo de vida propio.

La base principal vive en datos locales del usuario, no en la carpeta de fotos ni
en una unidad de red. WAL sólo en almacenamiento compatible; no asumir que
cualquier volumen portable/red admite el mismo journal. Un backup usa API/flujo
consistente de SQLite, no copiar sólo el `.db` abierto ignorando sus journals.

## Archivos y volúmenes

FileService ofrece selección recordada por propósito, abrir/revelar, copiar ruta,
hashing cancelable y vigilancia de ubicaciones aprobadas. Usa QDir/QFileInfo/QUrl,
sin concatenar separadores arbitrarios. Escrituras propias con QSaveFile o staging
y publicación atómica cuando el filesystem lo permita. Nunca sobrescribir fuentes
por el simple hecho de integrar una herramienta.

`LocationRef` contiene VolumeId interno, ruta relativa y pista absoluta histórica.
No guardar una URL local o letra de disco como identidad permanente. Resolver rutas
con validación de límites; impedir `..` y escapes por symlink/junction al ejecutar
desde una raíz aprobada. Mayúsculas, Unicode, recursos UNC, archivos ausentes y
volúmenes read-only requieren pruebas por plataforma; no convertir todos los paths
a minúsculas ni usar `canonicalFilePath()` como único registro de archivos offline.

QStorageInfo proporciona montajes/capacidad y, en Windows, `device()` devuelve el
GUID de volumen. En Unix suele ser un devpath: no es un UUID portable universal.
Véase [la documentación de QStorageInfo](https://doc.qt.io/qt-6/qstorageinfo.html).
Providers propuestos: GUID/serial filesystem en Windows; UUID de filesystem vía
proveedor del sistema en Linux; UUID de volumen/Disk Arbitration en macOS.
Consultar APIs/dispositivos sin elevar privilegios ni lanzar shell no confiable.

Una tabla de aliases asocia esos identificadores al VolumeId del Hub, con tipo,
origen y confianza. Clones/reformateos y cambios entre SO pueden ser ambiguos:
solicitar relocalización cuando no haya evidencia suficiente, sin fusionar por
etiqueta/tamaño. No escribir marcadores en discos del usuario automáticamente.
Un marcador portable opcional requeriría una acción explícita.

Conservar nombre, última ubicación y lastSeen cuando falte una unidad. Reconectar
reconcilia aliases/rutas y verifica un fingerprint sólo cuando sea necesario; no
hashear un disco al iniciar. QFileSystemWatcher invalida entradas/caché, pero se
complementa con notificaciones/enumeración de volúmenes; no garantiza todos los
eventos de desconexión por sí solo.

## Dependencias, logging, privacidad y errores

DependencyService almacena probes cacheados con versión, fecha, ubicación y motivo.
Separar archivo ausente, runtime incompatible y volumen desconectado. No cargar un
modelo para comprobar que existe ni instalar dependencias en segundo plano.
Errores: código estable, contexto traducible, detalles diagnósticos y acción útil.
Ejemplo: versión Python requerida/detectada y acceso a configuración del módulo.

Logging estructurado JSONL: UTC, severity, categoría, código, jobId/moduleId y
mensaje. INFO/WARNING/ERROR/DEBUG, rotación/tamaño/retención y nivel configurable;
no llenar las tarjetas de logs. Diagnóstico opt-in con rutas/datos sensibles
redactables. No registrar transcripciones, fotos o coordenadas completas por defecto.

Procesamiento local como valor inicial. Manifiestos declaran usos de red y la UI
los explica, diferenciando descarga de modelos de subida de datos. Una declaración
de privacidad no convierte QProcess/plugins en sandbox de red. Las herramientas
externas conservan sus permisos de usuario; no prometer aislamiento inexistente.
No telemetría ni actualización/descarga automática de módulos o modelos.

## Modo instalado y portable

`StoragePolicy` resuelve raíces de configuración/datos/cache antes de abrir SQLite.
Instalado: QStandardPaths. Portable futuro: configuración explícita y raíz marcada,
con detección de sólo lectura, espacio, desconexión y cierre seguro. No caer
silenciosamente al perfil global cuando falle un SSD portable. El portátil no
significa que un ejecutable Windows corra en macOS/Linux: habrá paquetes por SO.

## Proyectos y extensibilidad

ProjectId y ResourceRef enlazan archivos/módulos sin absorber todos sus esquemas.
El Hub conoce tipo, procedencia, ubicación y módulo propietario; un visor GIS o
base de laboratorio futura define su propio dominio. Añadir una herramienta
debería requerir su manifiesto/adaptador y pruebas, no modificar sidebar, dashboard,
base de fotos y código de transcripción conjuntamente.

## Incremento 0.4

DocumentLibraryController es independiente de DocumentController; comparte lector
privado bajo demanda, no un modelo cargado al iniciar. Plan/preview privados,
export a lote nuevo, vocabulario compartido para redes Obsidian. DocumentController
acepta libros completos con límites explícitos, consolida secciones y reintenta una
respuesta truncada antes de fallar. Actividad conserva el error útil del módulo.

InventoryController coordina un worker privado SQLite/openpyxl sin bloquear QML.
Su base .ahinv y auditoría son independientes del Store del Hub. Importación
normalizada primero; no se ejecutan clasificadores históricos ni se reescriben
fuentes. Detalles, límites y esquema en docs/INVENTORY.md y DOCUMENT_LIBRARY.md.
Cinco workspaces integrados; ninguna dependencia nueva de Python en el host.

## Incremento 0.5: lectura con evidencia

DocumentLibraryController incorpora library.enrich entre extracción y exportación.
document_analysis.py mantiene el cliente Ollama directo loopback, esquemas, pasajes,
validación literal de citas, síntesis y contraste de conceptos. document_notes.py
renderiza fichas, conceptos, relaciones y evidencia; ninguna ruta procede del modelo.
El host C++ conserva navegación, tareas, cancelación y revisión de resultados; QML
sólo presenta y solicita operaciones. No se carga el motor al abrir el Hub.
El plan enriquecido se escribe separado y sólo reemplaza el binding en memoria si
termina completo. Propuestas verificables por cita, no interpretaciones certificadas.
DocumentConversionPage reutiliza el servicio de conversión con una instancia
independiente de DocumentLibraryController, restringida a PDF/DOCX y sin temas/IA;
selección, plan, tareas y resultados no sustituyen los de la biblioteca.
