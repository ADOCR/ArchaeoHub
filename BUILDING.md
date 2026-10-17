# Compilar ArchaeoHub

Referencia: C++20, CMake >=3.24, Qt >=6.8 y Python 3.12. La entrega completa 0.5.4
se validó en Windows x64 con MSVC y Qt 6.9.3. Los flujos de CI no acreditan por sí
solos el funcionamiento completo en Linux/macOS.

## Core sin modelos, PROJ ni SDK de inferencia

Qt Core, Gui, Sql, Concurrent y Test son suficientes. Añade el prefijo de Qt a
`CMAKE_PREFIX_PATH`; en Windows utiliza una terminal con MSVC configurado.

```text
cmake -S . -B build-core -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt-SDK> -DWORKBENCH_BUILD_APP=OFF -DWORKBENCH_BUILD_INTEGRATION_TESTS=OFF
cmake --build build-core --config Release --parallel 2
ctest --test-dir build-core -C Release --output-on-failure
```

Este modo compila Core, servicios y formatos de coordenadas con sus contratos;
no produce la aplicación ni pretende sustituir los módulos reales.

## Python y pruebas de dominio

Crea un entorno de desarrollo e instala explícitamente los requisitos:

```text
python -m venv .venv
<python-del-entorno> -m pip install -r requirements.txt
<python-del-entorno> -m unittest discover -s tests -p "*tests.py" -v
```

Para congelar workers instala `requirements-build.txt`. `requirements-geodesy.txt`
añade pyproj para comparaciones geodésicas opcionales. No hay dependencias Python
globales obligatorias en el programa instalado.

## Aplicación completa

Por defecto `WORKBENCH_BUILD_APP`, `WORKBENCH_BUILD_TRANSCRIPTION_WORKER` y
`WORKBENCH_BUILD_INTEGRATION_TESTS` están activados. Se requieren además Qt Quick,
QuickControls2, Network y LinguistTools; Widgets en el worker de transcripción.

1. **PROJ 9.x:** proporciona `proj.h` mediante `WORKBENCH_PROJ_INCLUDE_DIR`. El worker
   carga un runtime PROJ y su `proj.db` compatibles al utilizar coordenadas.
2. **Whisper/ggml:** prepara un SDK en un directorio propio. La referencia utiliza
   [whisper.cpp](https://github.com/ggml-org/whisper.cpp), revisión
   `0c7be696371069e0baa2515ed7aa08d5a5473aa6`. Estructura esperada:

   ```text
   <sdk>/third_party/whisper.cpp/include/whisper.h
   <sdk>/third_party/whisper.cpp/ggml/include/ggml-backend.h
   <sdk>/build/third_party/whisper.cpp/src/<whisper-library>
   <sdk>/build/third_party/whisper.cpp/ggml/src/<ggml-libraries>
   ```

   CMake permite indicar explícitamente `WHISPER_LIBRARY`, `GGML_LIBRARY` y
   `GGML_BASE_LIBRARY` si tu generador produce otra estructura. Compila para la misma
   arquitectura y toolchain de Qt. CUDA es una opción del SDK, no del Core.
3. **ONNX Runtime:** la referencia usa 1.25.0. Extrae el SDK oficial para el sistema
   objetivo y pasa `WORKBENCH_ORT_ROOT`; debe contener `include/` y `lib/`.
4. **FFmpeg:** se ejecuta por separado en el runtime de transcripción. No se incluye
   su binario en Git. Identifica compilación, fuentes correspondientes y licencia.

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt-SDK> -DWORKBENCH_PROJ_INCLUDE_DIR=<PROJ-include> -DWORKBENCH_TRANSCRIPTION_SDK=<SDK-transcripcion> -DWORKBENCH_ORT_ROOT=<ONNX-Runtime-SDK> -DWORKBENCH_BUILD_INTEGRATION_TESTS=OFF
cmake --build build --config Release --parallel 4
```

Para desarrollar sólo la interfaz, `WORKBENCH_BUILD_TRANSCRIPTION_WORKER=OFF`
omite inferencia nativa y no pide ese SDK. No presenta un transcriptor funcional
si su motor no está instalado; no distribuyas ese modo como paquete completo.

## Preparar motores y pruebas de integración en Windows

Desde un entorno Python con `requirements-build.txt`:

```text
python tools/build_catalog_engine.py
python tools/build_spreadsheet_engine.py
python tools/build_document_engine.py
python tools/build_inventory_engine.py
python tools/stage_coordinate_engine.py --destination build/modules/coordinates/engine --bin <PROJ-bin> --data <PROJ-data>
python tools/stage_transcription_engine.py --runtime <runtime-transcripcion>
```

El último comando utiliza las bibliotecas Whisper/ggml/ONNX/CUDA, FFmpeg y plugins
Qt de un runtime preparado por el desarrollador. Consulta su lista explícita en
el script; un SDK de cabeceras por sí solo no es un runtime. No se copian los pesos
grandes de IA. Los dos modelos auxiliares MIT ya están en los recursos del worker.
Después reactiva `WORKBENCH_BUILD_INTEGRATION_TESTS=ON`, recompila y ejecuta CTest.

`tools/build_windows.ps1` puede leer `.local/build-environment.json`. Crea ese archivo
a partir de [config/build-environment.example.json](config/build-environment.example.json)
y sustituye los marcadores por tus rutas. No lo añadas a Git. El script sólo es una
comodidad para el build Windows completo; para Core usa los comandos CMake anteriores.

## Staging e instalador

Con PowerShell 7, Qt SDK, Python de desarrollo y motores ya preparados:

```powershell
./tools/deploy_windows.ps1 -QtRoot <Qt-SDK> -PythonExe <Python-desarrollo> -StagingDirectory dist/ArchaeoHub-0.5.4
python tools/build_installer.py --payload dist/ArchaeoHub-0.5.4 --nsis <makensis.exe> --icon resources/app.ico --output dist/ArchaeoHub-0.5.4-Setup-x64.exe
```

Usa staging nuevo y verifica los módulos con un PATH mínimo antes de empaquetar.
Los scripts de deployment buscan la carpeta `build`; con un generador multi-config
debes preparar los ejecutables de Release en la estructura esperada. La referencia
local Windows usa Ninja. El instalador no lleva modelos grandes, conserva archivos
ajenos al desinstalar y exige aceptación explícita de avisos de terceros.

No hay publicación automática de binarios. La revisión del SDK/GIS y las obligaciones
de redistribución del paquete completo están separadas de la licencia MIT de las
fuentes; consulta [publicación](docs/PUBLISHING.md) y [terceros](THIRD_PARTY.md).
