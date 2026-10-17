# Pruebas y alcance

## Preparación pública, 17/10/2026

En una carpeta limpia de Windows x64 se ejecutaron 82 pruebas Python (incluidas
cinco de la frontera de publicación), dos contratos C++ del modo Core sin SDK de
inferencia y las tres suites C++ de la compilación completa. Todas pasaron. El
build completo utilizó SDK y runtimes de desarrollo externos, excluidos de Git.
Se verificó la disponibilidad en PyPI de las versiones fijadas. Estos resultados
locales no equivalen a resultados de GitHub Actions ni de otros sistemas.

## Referencia de la aplicación

La referencia local 0.5.4 de Windows x64 pasó tres suites C++, 11 escenarios de
material de vidrio, 18 vistas del Hub, 25 vistas de módulos, cinco del selector
CRS, nueve de unidades, cuatro conversiones y 2.000 acciones de estrés. Se probaron
instalación, reinstalación, desinstalación y actualización 0.5.3→0.5.4 en carpetas
aisladas, conservando archivos y preferencias. No fue una VM independiente.

La CI de este repositorio se ha preparado pero **aún no se ha ejecutado en GitHub**.
Sus jobs de Core en Windows/Linux/macOS no prueban inferencia, deployment completo,
hardware gráfico ni el instalador de esos sistemas. No se afirma que esas tres
plataformas estén ya validadas para uso de campo.

## Comprobaciones sin datos privados

```text
python tools/check_publication.py --tracked
python -m unittest discover -s tests -p "*tests.py" -v
ctest --test-dir build-core -C Release --output-on-failure
```

Las pruebas Python usan archivos sintéticos y modelos simulados; no descargan
pesos ni requieren Ollama. Los contratos C++ del Core comprueban manifiestos,
rutas/volúmenes serializados, configuración, SQLite y formato de coordenadas.
La compilación completa con motores preparados habilita native_controller_tests.

Los scripts `verify_*`, `run_ui_chaos.py` y las pruebas de instalador son herramientas
de aceptación independientes, principalmente Windows. Requieren un payload ya
construido, carpetas vacías de QA y, en determinados casos, dependencias o modelos
locales indicados en sus argumentos. No deben ejecutarse todos indiscriminadamente
contra bases reales ni interpretarse como jobs de CI ya aprobados.

Las pruebas geodésicas con pyproj comparten PROJ: no son un motor matemático
independiente. Los ensayos no certifican precisión topográfica de campo.
Los resúmenes y vínculos generados por modelos deben revisarse. No se certifican
ausencia de fallos, recuperación de cualquier desconexión/disco lleno ni legalidad
de redistribuir cualquier SDK que se seleccione.

Los informes locales completos, rutas de unidades, datos arqueológicos y perfiles
se conservan fuera del repositorio público. No se publican para justificar un test.
