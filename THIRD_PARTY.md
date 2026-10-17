# Componentes y atribuciones

MIT en la raíz cubre el código propio de ArchaeoHub. No cambia las licencias de
bibliotecas, código adaptado, datasets o recursos de terceros.

## Material de terceros presente en estas fuentes

| Material | Procedencia y licencia | Aviso conservado |
|---|---|---|
| Decodificación NeMo TDT adaptada en Parakeet.cpp | [onnx-asr](https://github.com/istupakov/onnx-asr), MIT, Ilya Stupakov | resources/licenses/transcription/onnx-asr-MIT.txt |
| nemo128.onnx, auxiliar de preprocesamiento | onnx-asr, MIT | mismo aviso y resources/auxiliary-models.json |
| silero_vad.onnx, VAD auxiliar | [Silero VAD](https://github.com/snakers4/silero-vad), MIT, Silero Team | resources/licenses/transcription/Silero-MIT.txt |
| Avisos y textos de licencias | Sus titulares originales | resources/licenses/ |

Los dos ONNX suman aproximadamente 2,5 MB; no son los pesos Whisper/Parakeet de
transcripción. Sus hashes se registran y verifican antes de publicar. No se incluyen
otros modelos ni se otorga una licencia propia sobre esas contribuciones.

## Dependencias externas, no vendidas como MIT del Hub

- **Qt:** enlace dinámico; Core/GUI/Quick/QML/Controls, SQL, Concurrent y Network;
  Widgets para el worker. [Licencias de Qt](https://doc.qt.io/qt-6.8/licensing.html).
  Qt Quick Effects pertenece al conjunto Qt Quick. Mantén las bibliotecas separadas
  y los avisos/fuentes correspondientes al SDK utilizado al distribuir binarios.
- **Whisper/ggml:** MIT; revisión de referencia en BUILDING.md y aviso incluido.
- **ONNX Runtime:** MIT; referencia 1.25.0, avisos incluidos.
- **PROJ:** MIT; la base EPSG conserva condiciones propias. No hay proj.db ni
  rejillas en Git. El programa no habilita descargas de rejillas en segundo plano.
- **Python/Pillow/openpyxl/pillow-heif/et-xmlfile/pypdf:** avisos originales bajo
  resources/licenses; requirements identifica la referencia de desarrollo.
  Los codecs HEIF incluidos por un wheel tienen condiciones propias.
- **PyInstaller:** herramienta de construcción, GPL con su excepción para los
  programas congelados; no es la licencia del código del Hub.
- **FFmpeg:** programa externo. El runtime de referencia utilizó una compilación
  GPLv3 o posterior; el aviso y las fuentes deben corresponder al binario real.
- **NVIDIA CUDA/cuBLAS:** redistribuibles sujetos a su EULA; no se incluyen en Git.
- **Ollama y modelos locales:** dependencias opcionales separadas. Cada modelo
  conserva sus condiciones. Whisper y Parakeet no se vuelven MIT por incluir sus
  URLs de descarga en el Hub.

Los avisos históricos describen versiones utilizadas durante desarrollo; no son
un inventario exhaustivo de cualquier SDK futuro. Antes de distribuir un instalador
completo, revisa los binarios, fuentes correspondientes y avisos exactos de **todas**
sus dependencias. La aceptación de un EULA no sustituye esas obligaciones.
La preparación de este repositorio público no certifica una auditoría legal del
instalador local ni lo publica automáticamente.
