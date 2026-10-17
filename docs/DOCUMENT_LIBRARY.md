# Gestión documental 0.5 — fichas y conexiones con evidencia

Tres espacios nativos del módulo workbench.document-summary: resumen rápido,
biblioteca de lectura para Obsidian y Convertir a Markdown. La biblioteca añade análisis individual de cada
documento, conceptos contextualizados y relaciones explicadas. La conversión simple
sigue disponible mediante una opción explícita; la interfaz ya no crea la red de
investigación por mera coincidencia de vocabulario.

Convertir a Markdown tiene selección, revisión, destino y resultado independientes
de la biblioteca; permite PDF/Word por archivos o carpetas y no requiere Ollama.
Filtra .pdf/.docx, no modifica los Markdown que ya estén en esas carpetas. Exporta
texto/localizadores/procedencia en un lote nuevo; no conserva de forma exacta el
diseño de Word ni tablas complejas. No OCR ni .doc antiguo.

## Flujo

1. Añadir PDF con texto, Markdown, DOCX o carpetas y analizar el lote sin IA.
2. Seleccionar un modelo instalado en Ollama y Crear fichas y conexiones.
3. Abrir las filas Documentos, Conceptos y Conexiones para revisar síntesis y citas;
   exportar después una carpeta nueva.

No instala ni descarga un modelo. El cliente usa HTTP directo exclusivamente a
127.0.0.1, sin proxies ni redirecciones; rechaza modelos ausentes y declarados
remotos/cloud. Inferencia en el servicio local ya instalado. Procesamiento en worker
privado, progreso y cancelación; los libros y lotes grandes pueden tardar.

## Lectura y trazabilidad

document_analysis.py divide todos los bloques en pasajes acotados con IDs estables
y localizadores: página PDF, párrafo DOCX o líneas Markdown. No recorta el final
para ahorrar solicitudes. Extrae afirmaciones por tipo, conceptos con definición y
relaciones. La síntesis jerárquica resume las afirmaciones aceptadas, conservando
los IDs de su evidencia. Secciones de ficha: objetivo, método, argumentos, resultados
y limitaciones cuando existen; no se inventan apartados faltantes.

El modelo selecciona fragmentos por identificadores breves restringidos en el esquema;
el programa copia la cita original, sin pedir al modelo que la reproduzca o traduzca.
Valida esquema JSON y cada cita literal contra su pasaje, normalizando sólo espacios.
Rechaza IDs inexistentes, citas traducidas/inventadas y síntesis sin referencias.
Respuestas truncadas o de estructura inválida tienen un reintento; después, error.
Propuestas rechazadas y cobertura constan en manifiesto. Una fuente sin evidencia
suficiente queda identificada, sin sustituir su ficha por una invención.

**Una cita cotejada no valida automáticamente la interpretación del modelo.**
Fichas y relaciones son propuestas pendientes de revisión, sin porcentajes de certeza
inventados. La extracción tampoco recupera tablas, diseño u OCR de forma perfecta.

Los conceptos tienen identidad por documento. Nombres iguales no fusionan autores
ni sentidos: conservan definición, variantes y evidencia. Para conexiones entre
fuentes se comparan hasta 10 conceptos por documento y citas de ambos. Compartir una
palabra sólo puede nominar una comparación; nunca crea un enlace de investigación.
Cada relación incluye extremos, tipo, explicación y citas de ambas fuentes si es
entre documentos.

La comparación clasifica primero related/unrelated/insufficient_evidence. Diferencias
entre dominios sin cuestión común no se convierten en contrastes. Una explicación
que niega explícitamente la relación tampoco puede generar una arista.

Se comparan todos los pares para hasta 8 documentos. En lotes mayores, recuperación
por vocabulario contextual y hasta 3 comparaciones por documento, máximo 120 pares.
Manifiesto e índice indican candidatos y pares revisados: no se declara búsqueda
exhaustiva, resolución automática de sinónimos ni validación histórica. El modelo
puede omitir relaciones válidas o proponer interpretaciones discutibles aun cuando
las citas sean auténticas.

## Exportación y seguridad

Documentos/ conserva el texto convertido; Fichas/ las síntesis; Conceptos/ las
definiciones; Conexiones/ el porqué de cada relación; Evidencias/ las citas con
anclas y enlaces al original. Índice.md y archaeohub-library.json conservan modelo,
procedencia, hashes, cobertura y límites. No requiere plugins Obsidian.

Textos generados no introducen rutas o enlaces wiki arbitrarios: los crea el programa.
Los enlaces Markdown originales conservan el comportamiento anterior. Recursos
externos sólo referenciados, nunca abiertos ni descargados por el conversor. Citas
con caracteres Markdown se escapan al mostrar.

Plan base inmutable y nuevo plan enriquecido separado; fallo/cancelación conserva
el último plan completo. Hashes de origen antes y después del análisis y antes de
exportar. Publicación desde stage privado; no sustituye notas ni configuración
.obsidian. En una bóveda existente, enlaces relativos a esa raíz. Cancelación retira
sólo el stage propio; si llega después de publicar se informa éxito.

Sin fusión incremental ni modificación de notas editadas. Originales y adjuntos
permanecen en sus discos; los enlaces dependen de esas rutas. Sin garantía de
recuperación ante corte eléctrico o desconexión física.

## Lectores y límites conservados

Biblioteca: 1000 documentos, 32 MB/archivo, 500 páginas PDF, 2M caracteres/archivo y
8M/lote. Carpetas sin seguir reparse/symlinks/ocultas. Escaneos sin texto pendientes
de OCR; no detienen los convertibles.

Resumen rápido: 1–16 documentos, 2M caracteres/archivo y 4M/lote, síntesis por bloques,
consolidación y reintento. Sigue separado de las fichas de investigación.

Lector pypdf + stdlib. Sin MarkItDown, OCR ni Word binario .doc. Protocolos de
conversión 0.4 disponibles para compatibilidad; los temas de vocabulario del modo
legado no se presentan como conocimiento validado.
