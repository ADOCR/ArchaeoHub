# Módulos y manifiestos

Manifiestos schema 1, adaptadores de proceso y workspaces compilados implementados
en 0.3.0. No hay ABI publicada de plugins binarios. Los contratos genéricos de jobs
de dominio siguen propuestos; los cinco módulos integrados actuales usan controladores propios.

## Alcance ejecutable actual

El contrato real está en core/Module.h: metadata(), initialize(ModuleContext),
open(LaunchRequest) y shutdown(). ProcessModule implementa external/python/script.
Launch se mantiene para herramientas externas adicionales. Transcripción tiene
ahora un protocolo local de estado/acciones, sin lanzar su ventana independiente.
Native y future-plugin no cargan bibliotecas arbitrarias.

Transcripción, Catálogo, Coordenadas, Documentos e Inventario son
workspaces bundled con capacidad `workspace`, interfaz C++/QML y workers privados.
El catálogo usa Python 3.12/Pillow/openpyxl/pillow-heif congelados; Coordenadas usa
C++/PROJ y un helper independiente Python/openpyxl para Excel. Transcripción utiliza
C++/Whisper/ONNX, con FFmpeg y bibliotecas incluidas, pesos externos persistentes.
Se informa un motor ausente al intentar usarlo; no se instala en silencio.
Documentos usa un lector Python/pypdf empaquetado bajo demanda y un controlador
C++ que divide texto y consulta sólo Ollama local por Qt Network, sin proxy ni
redirecciones. La interfaz QML no contiene extracción ni inferencia. Sus datos no
se mezclan con bases del catálogo ni con el historial del transcriptor.
Agregar un manifiesto externo guarda metadatos revisados y después exige
seleccionar programa e intérprete absoluto cuando corresponda. No instala Python.
El registro valida schema, tipos, ID, SemVer, tamaño máximo 256 KiB, listas y
entrypoints sin traversal. Rechaza IDs duplicados al importar. Guarda un hash
aprobado por instalación; QFileSystemWatcher detecta cambios y exige revisión.

El manifiesto real es modules/transcription/tool.json. Los ejemplos y contratos
de evolución siguientes contienen servicios y campos aún no implementados por
completo; el lifecycle de jobs y los plugins requieren otro incremento.

## Protocolos implementados en 0.3

`modules/documents/document_worker.py`: una solicitud JSON schemaVersion 1 con
paths locales; devuelve documents/blocks/locator/warnings. No ejecuta contenido ni
solicitudes remotas. El controlador limita lotes y divide texto en bloques de
6.000 caracteres para resumen y consolidación, con cancelación y progreso. Consulta
127.0.0.1 únicamente; Ollama y el modelo deben estar instalados aparte. Los PDFs
escaneados/protegidos y .doc antiguos producen mensajes útiles, sin resultado parcial
presentado como completo. Un PDF mixto identifica páginas omitidas en el resultado.

`modules/catalog/python/catalog_worker.py`: JSONL v1 por stdin/stdout, `requestId`,
sesión única, páginas de 60. Acciones open/page/select/propose/save/correct/skip/
withdraw/discard/manualGps/exportGps/close, load/navigate/nextGps. Load inspecciona
un catálogo existente y nunca crea uno por equivocación; open con create exige
salida vacía. Site/operation se recuperan cuando existen. Las fotos se identifican por hash de su
ruta original canónica: cambia el contexto sin cambiar ese ID. No es todavía un ID
independiente del volumen para rutas antiguas. No acepta escrituras sobre archivos
ajenos a la sesión. Los retiros y resguardos exigen `confirmed:true`; migraciones
de esquemas antiguos exigen confirmación antes de ejecutar el constructor migrador.

`modules/coordinates/worker.cpp`: una solicitud v1 por proceso, JSON de progreso y
resultado/error. Acciones search/describe/convert. Sólo códigos EPSG para CRS Earth
2D compatibles; no acepta pipelines arbitrarios. PROJ_NETWORK=OFF, ONLY_BEST=YES,
ALLOW_BALLPARK=NO. No se envía nada por red. Conserva valores originales y errores
por fila; los resultados contienen operaciones, WKT, ejes, unidades y metadata.

`modules/coordinates/spreadsheet_worker.py`: readSpreadsheet (lista de hojas o
lectura de una hoja) y writeSpreadsheet. Proceso privado de una solicitud, no red,
no Excel COM ni macros; openpyxl congelado. Entradas .xlsx <=32 MB, ZIP expandido
<=128 MB, 128 columnas / 10.000 puntos; exige cache en celdas con fórmulas. El
resultado usa TSV entre worker y C++, con comillas y decimal numérico de punto.
No se reemplaza un destino existente ni se descartan resultados ante error.

`modules/transcription/native/worker.cpp`: JSONL con prefijo @ARCHAEOHUB@, acciones
add/remove/clear/settings/start/cancel/history/download/models/close. Eventos state,
finished y downloadFinished. Estado de cola/modelos/resultados separado del host;
historial transmite sólo encabezados hasta abrir un resultado. Configuración e
historial conservan identidad GamboaGlass; carpeta de pesos es una preferencia
persistente del módulo Hub, independiente del directorio instalado. Las descargas
requieren acción y aviso explícito; audio/video no salen de este equipo. Minutas
opcionalmente usan Ollama en loopback. No se exige instalar el transcriptor anterior.

## Identidad y ciclo de vida

Descriptor inmutable: id estable, versión SemVer, nombres/descripciones localizados,
categorías, autor, icono local, tipos de archivo, acciones/capacidades y dependencias.
Una instalación es otra entidad: ubicación/volumen, runtime/versión detectada,
argumentos/configuración, aprobación y fecha de disponibilidad. Dos instalaciones
pueden implementar el mismo toolId sin duplicar favoritos o perder su historia.

Contratos conceptuales C++20:

- `IModule`: descriptor y capacidades, `initialize(ModuleContext&)`,
  `createJob(ActionRequest)` y cierre coordinado.
- `IJob`: estado, progreso opcional, resultado/error y solicitud de cancelación.
- `ModuleContext`: interfaces de tareas, archivos, logging, configuración y
  repositorio privado del módulo cuando se autorice; no acceso irrestricto al Core.
- `ModuleViewDescriptor`: vista de acción estándar, URL QML compilada de confianza
  o aplicación externa. La vista se resuelve en presentación, no con `getWidget()`
  obligatorio dentro de un Core que no depende de Widgets.

Son propuestas de responsabilidades, no cabeceras listas para compilar. Antes de
fijar API binaria se definen ownership RAII, afinidad de hilos, resultados tipados,
compatibilidad y tratamiento de excepciones en los límites del plugin.

Descubierto -> validado -> candidato revisable -> registrado -> inicializado al
abrir -> ejecutando -> disponible. Disponibilidad y lifecycle no son la misma
columna. Un módulo desconectado sigue registrado; nunca se elimina por un scan.

## Ejemplo de un adaptador externo adicional

Ejemplo documental: **no registrar ni ejecutar automáticamente**.

```json
{
  "schemaVersion": 1,
  "id": "example.external-tool",
  "version": "0.1.0",
  "name": {"es": "Herramienta externa", "en": "External tool"},
  "description": {"es": "Abre una herramienta adicional seleccionada."},
  "categoryIds": ["documentation"],
  "author": "Local tools",
  "icon": "tools",
  "runtime": "external",
  "entrypoint": {"binding": "selected-installation"},
  "supportedFileTypes": ["audio/*", "video/*"],
  "capabilities": ["launch"],
  "view": {"kind": "standard-tool-page"},
  "dependencies": [
    {"id": "external-installation", "kind": "installation", "required": true}
  ],
  "privacy": {
    "processing": "local",
    "networkUses": ["optional-model-download", "optional-local-ollama"]
  }
}
```

`selected-installation` es un binding cerrado del host a una ubicación configurada,
no una expresión evaluable. La versión del adaptador no se confunde con la versión
del ejecutable encontrado. `supportedFileTypes` sirve para clasificación:
este adaptador sólo permite abrir la app, no promete enviarle un archivo si aún
no hay un contrato de apertura documentado.

## Validación y descubrimiento

Schema version obligatoria, id con formato estable, límites de tamaño/profundidad,
campos tipados y SemVer validado. Errores de campos conocidos se explican; una
versión de schema mayor no soportada produce estado incompatible, sin intentar
interpretarla. Manifest duplicado: agrupar por toolId y comparar instalaciones;
no permitir que el último archivo escaneado reemplace silenciosamente otro.

Entrypoints de paquetes: rutas relativas confinadas a la instalación, sin escapes
`..`, symlinks/junctions hacia otra raíz ni comandos de shell. Rutas externas
requieren una ubicación explícita aprobada. Validar plataforma/arquitectura,
binario/intérprete, existencia, permisos y dependencias antes de ejecutar.

Buscar `.git`, pyproject/requirements, CMake, package.json, README, scripts o
ejecutables sólo genera candidatos con evidencia/confianza. Carpetas de dependencias,
build, node_modules y entornos se excluyen; escaneo cancelable por carpeta/unidad
seleccionada. `Agregar`, `Ignorar`, `Revisar`; guardar decisión y fingerprint para
que el asistente no reofrezca cada copia. No arrancar binarios ni importar módulos
desconocidos para identificarlos. Mantener candidatos desconectados sin descartarlos.

Una aprobación se vincula a contenido/hash del manifiesto e instalación, no sólo
al nombre; cambios de entrypoint/runtime requieren revisión. Un hash detecta cambios,
no demuestra que un autor o programa sea seguro. Plugins/QML son código ejecutable.

## Runtimes

| Runtime | Primera implementación | Aislamiento / carga |
|---|---|---|
| external | QProcess sobre ejecutable seleccionado | GUI o worker separado; no asumir protocolo |
| python | QProcess sobre intérprete absoluto y script/CLI | Entorno por instalación, dependencias declaradas |
| script | Intérprete explícito, argumentos separados | Sin shell implícito; revisar efectos primero |
| native | Adaptador propio o plugin Qt compatible de confianza | Lazy; pesados preferentemente fuera del proceso |
| future-plugin | Descriptor conservado | Estado no soportado; nunca lanzar como fallback |

No concatenar programa/argumentos, interpolar manifiestos en shell ni ejecutar el
README. Directorio de trabajo y entorno son explícitos; stderr/stdout tienen
límites para evitar agotar memoria. Probes de versión tienen timeout y se ejecutan
sólo sobre herramientas seleccionadas/de confianza, sin instalar componentes.

## Primer módulo: transcripción

1. Registrar candidato de la aplicación nativa, sin moverla ni copiar modelos.
2. Abrir su GUI conservando preferencias/historial. El Hub registra último uso y
   errores de arranque. No importa automáticamente las 30 transcripciones previas.
3. Mantener la versión Python como instalación alternativa para funciones aún
   distintas; no afirmar paridad de TXT/minutas, idiomas o pesos cuantizados.
4. Caracterizar motor/exportación/minutas, luego introducir worker productivo con
   protocolo v1 y UI del módulo. Mantener la app independiente como cliente.

El futuro worker usa mensajes JSON limitados/versionados por canal local con jobId,
acción, entradas por ruta/referencia, eventos de progreso, warnings y resultado.
Nunca interpretar stdout libre como comandos; stderr es diagnóstico. Definir
handshake, negociación de capacidades, cancelación/timeout, códigos de error y
publicación atómica de resultados. Datos grandes por archivos locales, no copias
de audio codificado dentro de JSON. Esto no existe aún en el ejecutable actual.

## Plugins Qt nativos

Q_DECLARE_INTERFACE/Q_PLUGIN_METADATA y QPluginLoader permiten una interfaz de
extensión. Leer metadata antes de instanciar; comprobar versión API, Qt, compilador,
arquitectura y build compatible. Qt no proporciona ABI universal entre cualquier
toolchain/versión. Distribuir un SDK y matriz concretos cuando exista una segunda
implementación que justifique fijar ese contrato.
Referencia: [plugins Qt](https://doc.qt.io/qt-6/plugins-howto.html) y
[QPluginLoader](https://doc.qt.io/qt-6/qpluginloader.html).

Plugins sólo desde raíces de confianza, no cualquier USB con una DLL. No prometer
hot-unload: vistas QML, objetos, threads y callbacks pueden sobrevivir a una acción.
Inicialmente cargar bajo demanda y conservar el plugin hasta cerrar el host;
actualización al reiniciar. El plugin no debe lanzar excepciones a través de un
límite ABI sin contrato ni bloquear inicialización esperando modelos o red.

## Pruebas de contrato

Manifiestos válidos/rotos/versionados; IDs/instalaciones duplicadas; escapes de
ruta, Unicode y desconexión; dependencias/versiones; estados de jobs; cancelación,
procesos fallidos, salida parcial y protocolo inválido; conservación de recientes
y favoritos después de relocalizar. Pruebas de cada módulo separadas de las del
Core. Fixtures sintéticos sin coordenadas ni material arqueológico privado.

## Incremento 0.4 vigente

Gestión documental conserva ID workbench.document-summary e incluye dos pestañas:
resumen y biblioteca. DocumentLibraryController/DocumentWorker se comunican por
JSONL de progreso y una respuesta final; planes privados y hashes, cancelación por
sentinela en exportación. No hay OCR ni MarkItDown en el paquete.

workbench.inventory → InventoryPage.qml → InventoryController → InventoryWorker.
Runtime Python SQLite/openpyxl empaquetado, local, lazy. JSONL UTF-8 explícito,
protocolo acotado y una operación por proceso. Las funciones SQL quedan en el módulo,
no en el Store común. Véase docs/INVENTORY.md para alcance real y límites MNCR.

### Lectura documental 0.5

library.enrich recibe planFile/planDigest, analysisFile nuevo en la misma caché,
model y cancelFile; produce JSONL de progreso y análisis final con un nuevo digest.
No modifica el plan de entrada. El modelo debe existir localmente; endpoint sólo
loopback. Los IDs de pasaje, citas literales y referencias de síntesis se validan.
Las relaciones incluyen evidencia, explicación y reviewStatus=proposed. El protocolo
export conserva library.export y añade fichas/conceptos/conexiones/evidencias cuando
el plan las contiene. Conversión legada independiente, sin exigir un modelo.
