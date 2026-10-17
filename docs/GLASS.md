# Material de vidrio — 0.5.4

La navegación, barras de herramientas y paneles flotantes usan difusión real del
fondo, un tinte translúcido, reflejo vertical, doble borde y sombra suave. Las
tablas, formularios y áreas de lectura mantienen superficies sólidas. El fondo
del material es estático y discreto; no hay animaciones decorativas continuas.

## Separación de responsabilidades

- `Theme.qml` centraliza colores y preferencias; mantiene referencias a las dos
  fuentes visuales y la disponibilidad del renderizador.
- `MaterialBackdrop.qml` dibuja el campo de color detrás de la navegación. Esta
  fuente no contiene los propios paneles: evita capturas recursivas.
- `GlassSurface.qml` decide entre vidrio y superficie opaca. Calcula la región
  del fondo después del layout y al redimensionar la ventana o mover su padre.
- `FrostedGlass.qml` se carga sólo cuando se necesita. Usa ShaderEffectSource y
  MultiEffect para difusión, máscara redondeada y sombra. La textura del fondo
  se recorta al panel con margen y se captura a media resolución.
- Los paneles de acciones, selección CRS y creación de proyecto toman una
  instantánea de la escena al abrirse o cambiar su región. La escena excluye
  Overlay, por lo que no incluye el panel que se está dibujando. Un cambio de
  tema solicita una nueva captura. La navegación usa el campo de material vivo.

Es vidrio dentro de la aplicación. No captura el escritorio, otras ventanas ni
información externa; no depende de un efecto nativo de Windows/macOS. Tampoco
implementa refracción física ni reproduce una interfaz propietaria.

## Accesibilidad y recursos

Reducir transparencia o activar contraste alto desactiva la carga de efectos y
utiliza superficies opacas. El renderizador software y los backends sin soporte
conocido también usan esa alternativa. Reducir movimiento sigue disponible y
el material no añade animaciones. Los cambios de tema y texto ampliado conservan
la navegación y los controles existentes.

Los efectos consumen texturas y tiempo de GPU, aunque la captura esté recortada.
La comprobación en este equipo usa Direct3D11. No garantiza rendimiento en todos
los equipos ni acredita compilación o renderizado en Linux/macOS. Las pruebas y
capturas concretas se identifican en el informe de entrega de 0.5.4.

## Verificación

`tools/verify_glass.py` comprueba los temas, dos tamaños de ventana, paneles,
texto ampliado, reducir movimiento, transparencia reducida, contraste alto y
software. El diagnóstico registra backend y efectos realmente cargados, además
de errores QML. Las regresiones existentes comprueban módulos y coordenadas;
el estrés de interfaz recorre páginas y abre/cierra/redimensiona paneles.

Referencias de implementación:
[MultiEffect, Qt 6.8](https://doc.qt.io/qt-6.8/qml-qtquick-effects-multieffect.html),
[ShaderEffectSource, Qt](https://doc.qt.io/qt-6/qml-qtquick-shadereffectsource.html).
