# Contribuir

Abre un issue con el problema o una propuesta concreta. Para cambios, crea una
rama y un pull request pequeño que explique comportamiento, pruebas y límites.
Conserva los límites entre Core, servicios, UI y módulos; no cargues motores
pesados al arrancar ni reemplaces implementaciones funcionales sin comparación.

Lee DESIGN.md para UI. Mantén identificadores y nombres internos en inglés.
Todos los comentarios y docstrings deben estar en español. Los textos de interfaz
son traducibles con Qt, inicialmente en español. Usa C++20/Qt 6, RAII y CMake moderno.

Ejecuta las comprobaciones de docs/TESTING.md que correspondan a tu cambio.
Todos los ejemplos de pruebas deben ser sintéticos. No adjuntes coordenadas de
sitios, fotografías, audios, informes, rutas personales ni bases reales a issues,
PR o logs públicos. Conserva la trazabilidad y evita sobrescribir originales.

Las contribuciones de código propio se ofrecen bajo MIT. Si incorporas código o
recursos de terceros, identifica autor, fuente, revisión y licencia; añade los avisos
correspondientes. No añadas SDK, modelos grandes, ejecutables ni datos a Git.
