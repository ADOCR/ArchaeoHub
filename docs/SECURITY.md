# Seguridad y límites — ArchaeoHub 0.5.2

## Límites de confianza

El Hub procesa archivos en este equipo. Los motores incluidos se ejecutan como
procesos separados, con los permisos del usuario: **no son un sandbox del sistema
operativo**. Los complementos externos requieren selección/configuración explícita;
su manifiesto no convierte un ejecutable desconocido en software confiable.

La interfaz QML sólo puede leer recursos `qrc` y archivos locales. Se bloquean HTTP,
HTTPS y rutas de archivo UNC desde su gestor de recursos, sin resolverlas primero.
Los nombres de herramientas/botones se muestran como texto, no como HTML. Las
operaciones explícitas de los servicios backend, como descargar un modelo tras
confirmarlo, tienen su propio gestor y no pasan por esta política de presentación.

## Modelos y privacidad

Resúmenes y minutas consultan Ollama por `127.0.0.1`, sin proxy ni redirecciones.
Antes de enviar contenido se consulta `/api/show`; se rechazan `remote_host`,
`remote_model` y nombres de modelos cloud. La biblioteca aplica la misma restricción.
Las respuestas C++ se limitan a 4 MB y las del analizador Python a 2 MB. Cancelar una
consulta no borra el último resultado válido.

Esto verifica los metadatos de un servicio local de confianza; no es una prueba
criptográfica de dónde ejecuta su código un servicio comprometido. No se descarga
un modelo para satisfacer una selección que no existe. Referencia del protocolo:
[esquema oficial de Ollama](https://github.com/ollama/ollama/blob/main/docs/openapi.yaml).

## Archivos y exportaciones

- Catálogo: Excel de hasta 64 MB, 128 MB expandidos, 4.096 entradas ZIP,
  100.000 registros y 128 columnas. El estado JSON admite hasta 64 MB. Se mantiene
  el dominio anterior y se añade un lector acotado en su frontera de entrada.
- Documentos: extracción acotada de PDF/Markdown/DOCX, sin ejecutar macros ni
  entidades externas. Los escaneos siguen requiriendo OCR fuera de esta versión.
- Coordenadas e inventario: texto de aspecto de fórmula se neutraliza al exportar
  CSV, también en encabezados. En XLSX se usan celdas de texto para datos externos.
  JSON y la base interna conservan el valor original.
- La protección CSV corresponde al archivo exportado; reabrir, editar y guardar
  con otras aplicaciones puede cambiar su interpretación. Para conservar tipos
  de texto, se prefiere XLSX. Véase [CSV Injection, OWASP](https://community.owasp.org/attacks/CSV_Injection).
- FFmpeg recibe una lista explícita `file,pipe`; no puede seguir recursos HTTP
  desde la entrada multimedia. El FFmpeg incluido ya rechazó el HLS remoto del
  ensayo antes de este cambio: se trata de refuerzo, no de una fuga demostrada.
  El PCM se valida antes de copiarlo a muestras y conserva su límite de memoria.
- No se reemplazan archivos existentes silenciosamente; se usan staging,
  transacciones o publicación exclusiva según el módulo. Una selección inválida
  conserva los resultados anteriores cuando el flujo lo permite.

## Perfil y recuperación

Sólo una instancia 0.5.2 puede escribir cada perfil. `QLockFile` protege el perfil;
un canal local limitado al usuario sólo permite activar la ventana existente, no
ejecutar acciones ni recibir rutas. Los perfiles diferentes siguen siendo independientes.
Al usar `--data-dir`, el transcriptor también guarda su configuración en ese perfil,
salvo que se haya indicado explícitamente `GAMBOA_CONFIG_DIR`. El inicio habitual
conserva la configuración y el historial de la instalación anterior.
Una terminación forzada de una instancia de prueba permitió recuperar el bloqueo
y obtener `integrity_check = ok` de SQLite. No equivale a probar todos los cortes
eléctricos, sistemas de archivos o dispositivos desconectados.

El SQLite del Hub usa WAL y `synchronous=FULL`. No se reduce la sincronización para
acelerarlo. Los inventarios y datos de cada módulo siguen siendo independientes.
Los logs son locales. No se añadió telemetría.

## Pendientes de seguridad y fiabilidad

La sesión y sus observaciones concretas se documentan en el informe de QA asociado
al instalador. No se certifica la ausencia de fallos en bibliotecas nativas de terceros.
Quedan una revisión completa de CVE, firma digital de distribuciones y verificación
automática de hashes de pesos de modelos. También conviene ampliar la inyección de
fallos de almacenamiento: disco lleno, errores físicos, perfil no escribible durante
una sesión y pérdida de conexión. Un plugin autorizado conserva los permisos del
usuario; no debe instalarse código ajeno sin revisar su procedencia.
