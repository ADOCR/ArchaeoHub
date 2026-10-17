# Almacenamiento del Hub

Schema físico 1 implementado en services/Store.cpp y probado con Qt SQL.
PRAGMA user_version, foreign_keys=ON, sentencias preparadas y transacciones;
WAL y synchronous=FULL. La conexión pertenece al hilo GUI; workers no la comparten.

Tablas actuales: tools (JSON/hash), installations (programa/intérprete como
LocationRef y hash aprobado), favorites, recent_tools, volumes (identidad/key/
montaje/última observación), projects, recent_files, tasks y task_events.
Un índice parcial asegura una instalación preferida por herramienta. Preferencias
en preferences.ini; las bases internas de los módulos no se importan.

La creación del esquema es transaccional. Bases ajenas y versiones futuras se
rechazan conservando sus tablas. Antes de un cambio de schema 1 a otro release
se deberá implementar y probar backup consistente y recuperación. A continuación,
el modelo lógico de evolución de Fase 0, más amplio que el esquema actual.

## Entidades y relaciones

| Tabla | Campos/relaciones principales | Propósito |
|---|---|---|
| schema_migrations | version PK, checksum, applied_at | Migraciones inmutables y verificables |
| categories | id PK, name_key, icon_key, sort_order | Navegación independiente de herramientas |
| tools | id PK del manifiesto, metadata_json, schema_version | Identidad y metadatos lógicos; no una ruta ejecutable |
| tool_categories | tool_id FK, category_id FK, PK compuesta | Una herramienta en varias categorías |
| installations | id UUID PK, tool_id FK, location_id FK, runtime, version, config_json, last_seen | Variante concreta por plataforma/entorno |
| manifest_revisions | id, installation_id FK, sha256, manifest_json, observed_at | Procedencia y cambios del descriptor |
| approvals | installation_id FK, manifest_revision_id FK, executable_fingerprint, decision, decided_at | Revisión de candidato/contenido; no firma de confianza |
| dependency_probes | installation_id FK, dependency_id, detected_version, status, checked_at, detail_code | Estado cacheado y diagnóstico reproducible |
| volumes | id UUID PK, label_hint, last_seen, last_mount_hint, last_observation | Identidad interna; una letra no es PK |
| volume_aliases | volume_id FK, provider, kind, value, host_scope, confidence | GUID/UUID/serial por plataforma; índice de búsqueda no globalmente único |
| locations | id UUID PK, volume_id FK nullable, scheme, relative_path, absolute_hint, fingerprint_json | Referencia resoluble incluso offline |
| favorites | tool_id FK UNIQUE, sort_order, created_at | Favoritos lógicos, no por copia de ejecutable |
| recent_tools | tool_id FK PK, installation_id FK nullable, last_opened_at, use_count | Uso de herramientas |
| file_refs | id UUID PK, location_id FK, media_type, owner_tool_id FK nullable | Referencias a archivos; no su contenido |
| recent_files | file_id FK PK, last_opened_at | Archivos recientes |
| projects | id UUID PK, name, metadata_json, created_at, archived_at | Abstracción mínima de proyecto |
| project_resources | project_id FK, file_id FK, role, owner_tool_id FK nullable | Fotos/coordenadas/modelos/documentos por referencia |
| recent_projects | project_id FK PK, last_opened_at | Continuación de proyectos |
| tasks | id UUID PK, tool_id FK, installation_id FK nullable, action, state, progress nullable, timestamps, error_code | Ejecuciones y resultado resumido |
| task_resources | task_id FK, file_id FK, direction, role | Entradas/resultados locales por referencia |
| task_events | id PK, task_id FK, at, state, code, detail_json | Historial acotado de transiciones/avisos |
| scan_sessions | id, selected_location_id FK, timestamps, status, limits_json | Alcance y cancelación de búsquedas |
| scan_candidates | id, scan_id FK, location_id FK, evidence_json, fingerprint, decision | Agregar/ignorar/revisar sin ejecución implícita |

Recientes se separan por tipo para conservar claves foráneas reales; evitar una
columna `kind/id` que apunte indistintamente a tres tablas sin integridad referencial.
Las propiedades JSON llevan schema/version y validación; los campos consultados
frecuentemente se mantienen tipados/indexados, no enterrados en JSON.

Configuración de UI pequeña en QSettings mediante SettingsService. Configuración
de instalación/adaptador en `installations.config_json`, sin duplicar claves de UI.
Credenciales futuras van a un almacén del sistema o una política explícita; no
guardar secretos en manifiestos o logs.

## Invariantes

- toolId estable, InstallationId distinto por variante/entorno; la versión del
  adaptador y la del ejecutable no se confunden.
- Etiqueta, tamaño, mount point o serial aislado no identifican inequívocamente
  un volumen. Aliases colisionados producen ambigüedad, no una fusión automática.
- La pérdida de un montaje/dependencia actualiza observación, no borra tools,
  installations, favorites, projects o locations. Un mount point es una pista.
- Ruta relativa validada por esquema y proveedor; no reutilizar reglas Windows
  como si fueran las mismas de un filesystem sensible a mayúsculas.
- Timestamps UTC ISO-8601, IDs opacos; estado desconocido al iniciar hasta refrescar
  observaciones. No presentar como conectado un estado persistido antiguo.
- FK con borrados restrictivos por defecto; archivar/deshabilitar explícitamente
  antes que eliminar referencias. Borrar un favorito sí es una acción reversible
  simple. Políticas de cascada sólo donde no destruyan historia de trabajo.
- Fotos, audio, pesos IA y grandes miniaturas no van a la base Hub. La cache de
  miniaturas del catalogador tendrá tamaño/límite propios y será regenerable.
- Datos internos del catalogador/RTI/transcriptor no se importan implícitamente.

## Concurrencia y recuperación

Una conexión por hilo con nombre/ciclo de vida propio; no compartir QSqlDatabase o
QSqlQuery activos entre workers. Servicio de escrituras inicialmente serializado,
transacciones cortas, sentencias preparadas, foreign_keys activado en cada conexión
y busy_timeout limitado. No hacer I/O lento de archivos dentro de una transacción.

Migraciones con versión/checksum y backup consistente previo; si fallan, rollback
y error útil sin crear una base vacía sobre datos antiguos. La versión de esquema
mayor no soportada abre un diagnóstico de incompatibilidad, no intenta downgrade.

WAL se evalúa para la base en almacenamiento local compatible. No ubicar la base
principal en OneDrive/un recurso de red ni suponer que un `.db` copiado solo es un
backup de una base abierta en WAL. Portable usará política de journal y cierre
validada sobre el filesystem elegido; detener el trabajo si falta el volumen.

Al reiniciar, tareas que estaban en ejecución pasan a Interrupted con motivo. No
usar un PID persistido como autorización para terminar un proceso nuevo. Reanudar
requiere contrato idempotente del módulo y verificación de resultados parciales.
Los logs detallados viven en JSONL rotados; SQLite guarda historia de tareas útil,
con política de retención, no cada línea técnica sin límite.

## Pruebas requeridas antes de uso real

Crear/abrir/migrar y rollback con fixtures; integridad de FK/duplicados; favoritos
tras relocalizar; colisión de alias de volumen; unidad ausente; ruta Unicode;
reformateo; sólo lectura; lock/busy; conexiones por hilo; interrupción de tareas;
backup/restore con journal activo; rechazo de esquema futuro. El modo portable
necesita pruebas de extracción/desconexión; un shutdown correcto no prueba por sí
solo resistencia a una retirada física durante escritura.

## Base de inventario independiente (0.4)

application_id 0x4148494E, user_version1. Tablas metadata, sources(hash único),
contexts(firma única), containers(padre/tipo/código/firma), records(ID/contexto/
envase/fuente/colección/data/raw_data/revision), materials(registro/campo/cantidad)
y audit(acción/fecha/before/after). Foreign keys activas por conexión y transacciones
por importación completa. No migra ni cambia el esquema1 del Store del Hub.
El primer incremento audita ingresos; edición auditada de importaciones es futura.
