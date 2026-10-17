# Revisión de paridad del catálogo — 0.3.0

Referencia conservada: paquete_catalogador_2.4.5, no las múltiples copias históricas.
Se revisaron dominio, GUI Tkinter, GPS, comportamiento de reanudación, selección,
registro/corrección, retiros, bloqueo, resguardo y recuperación. No es suficiente
que una tarjeta abra una ventana: el flujo de trabajo se mantiene en el Hub.

Catalog/CatalogLock/helpers extraídos mantienen el AST de la referencia; GPS se
copia byte a byte. ORIGIN.json identifica procedencia. La app original sigue intacta.
Los 39 tests de referencia se repitieron en una copia aislada; 23 pruebas nuevas
del puente cubren la integración. Las suites C++ ejercitan el controlador real y
el runtime empaquetado; las capturas usan sólo fotos sintéticas.

| Comportamiento de referencia | Estado del Hub | Verificación y límites |
|---|---|---|
| Abrir catálogo existente | Cargar catálogo… separado de Nuevo | Excel canónico/estado/salida, sin pedir sitio u operación ya presentes; varios sitios se rechazan y operaciones múltiples no inventan un valor |
| Elegir originales | Conserva referencia validada y propone ruta si falta | La raíz común de filas no prueba el origen completo; requiere confirmación. No se cambia silenciosamente una letra de unidad antigua |
| Crear catálogo | Sitio, operación y salida vacía obligatorios | Rechazo de salida ocupada; cargar nunca crea catálogo accidentalmente |
| Reanudar trabajo | Marca de navegación, primera pendiente, selección visible | Prueba reabre tras guardar/siguiente; cambia de página de 60 cuando corresponde |
| Registro y consecutivos | Mismo dominio Python y conflictos | Copia de entrega/Excel/estado; no reemplaza archivos existentes ni avanza si falla el guardado |
| Guardar y siguiente | Con o sin GPS manual | GPS se aplica a copia, original y Excel original preservados; reanuda próxima pendiente |
| Corregir y cancelar | Draft de formulario/GPS en memoria, cancelar corrección | No confundir draft temporal con guardado durable. Preview/nombre se actualizan tras cambios |
| Omitir, recuperar, retirar | Acciones explícitas y confirmaciones | Retiro recuperable; corrupción o commit parcial bloquean mutaciones y piden reabrir |
| Resguardar original | Confirmado, incluso si estaba registrada | Retira primero la entrega según GUI anterior y mueve con journal; informa retiro ya confirmado si falla movimiento posterior |
| GPS | Original y copia diferenciados, siguiente revisión y reporte aparte | Backups; errores de cámara se muestran sin corregir datos reales automáticamente |
| Imágenes y navegación | JPEG/HEIC, miniaturas, filtros, zoom/giro, siguiente/anterior | Giro ajusta dimensión visible; cruza páginas. Previews hasta 2.000 px, Abrir original para resolución completa |
| Bloqueo y recuperación | CatalogLock y dominio transaccional conservados | Dos sesiones no escriben al mismo catálogo. Excel bloqueado conserva selección/contexto; advertencia tras commit obliga revisión |

No se declara paridad de cada gesto de Tkinter. No hay edición masiva, SQLite propio,
OCR, relocalización automática del formato legado ni edición de fotos. Se conservan
las funciones de dominio comprobadas sin introducir esas migraciones a la vez.

La prueba real de 1.714 fotos corresponde a 0.2.0 y se conserva como evidencia
histórica, con originales verificados por hashes. En 0.3 se repiten regresiones
focalizadas y carga de metadatos de sólo lectura; no se presenta aquel esfuerzo
como una repetición completa del nuevo instalador.

## Restauración del visor 0.4

Releído ImageViewer._set_zoom/_on_wheel/_pan_move/fit/actual_size/rotate_clockwise
original. Rueda1.20 anclada al cursor, Ctrl+rueda también; botones1.25 al centro.
Pan limitado al área visible, Ajustar conserva giro y reinicia desplazamiento;
Girar ajusta automáticamente, cambiar fotografía reinicia giro/zoom/pan. 100% se
refiere a los píxeles de la vista previa (máximo2000px), no a resolución original.
Límites de escala0.001–8 sobre esa vista previa. No se modifica la fotografía.
QA envía QWheelEvent reales y verifica el punto bajo el cursor en0/90/180/270°,
rueda inversa y Ajustar; no equivale a certificar todos los gestos de trackpad.
