# Publicar las fuentes en GitHub

Este árbol está preparado como repositorio **público MIT**. La preparación local
no crea repositorios en la cuenta ni envía contenido a Internet.

## Primer envío

1. Revisa `git status`, README, LICENSE, THIRD_PARTY y la lista de archivos.
2. Ejecuta `python tools/check_publication.py --tracked` y las pruebas descritas
   en TESTING.md. Mantén las credenciales fuera del repositorio.
3. Crea un repositorio vacío en tu cuenta de GitHub, sin README/licencia iniciales.
   El nombre propuesto es **ArchaeoHub**; verifica disponibilidad antes de crearlo.
4. Conecta la URL HTTPS o SSH que muestre GitHub y envía la rama:

   ```text
   git remote add origin <URL-del-repositorio-creado>
   git push -u origin main
   ```

No pegues tokens en la URL, archivos o comandos guardados. Usa el gestor de
credenciales de Git o SSH. No necesitas instalar gh para realizar ese envío.

Después del primer envío comprueba los jobs de CI: no hay resultados remotos
antes de crearse el repositorio. Activa notificación privada de vulnerabilidades,
secret scanning/push protection cuando estén disponibles y protección de `main`
con revisión y comprobaciones obligatorias, según tu configuración de cuenta.
Esas opciones no se han activado desde esta preparación local.

## Fuentes y binarios son entregas diferentes

No añadas `dist/`, SDK, perfiles, bases de datos ni pesos a Git. Los dos auxiliares
ONNX atribuidos son las únicas excepciones de modelos. Mantén los instaladores
fuera del historial de fuentes; GitHub Releases es una entrega separada.

El instalador Windows local de referencia no está firmado y su inventario completo
de binarios/SDK/GIS necesita revisión antes de distribución pública. No lo subas
automáticamente sólo porque las fuentes sean MIT. Una vez resuelta esa revisión,
usa una release revisada, notas con plataforma realmente probada, SHA-256 y avisos
exactos. No hay workflow que publique instaladores o acepte licencias por el usuario.

## Cambios posteriores

Trabaja sobre este repositorio limpio. La copia privada original conserva auditorías
y paquetes anteriores; no la publiques mediante `git add .`. Usa ramas, mantén
pruebas sintéticas y actualiza dependencias mediante revisión. CODEOWNERS propone
@ADOCR; ajústalo si el repositorio pertenece a otra cuenta u organización.

Referencia: [seguridad de GitHub Actions](https://docs.github.com/en/actions/reference/security/secure-use).
