# Seguridad

Los procesos de módulos tienen los permisos del usuario: no son un sandbox del
sistema operativo. No ejecutes herramientas o plugins de procedencia desconocida.
El procesamiento es local; los resúmenes requieren un servicio Ollama local de
confianza. Consulta [límites técnicos](docs/SECURITY.md).

Para informar una vulnerabilidad, utiliza **Report a vulnerability** en la pestaña
Security si el repositorio tiene activada la notificación privada. Si no aparece,
abre únicamente un issue solicitando un canal privado, sin publicar el exploit ni
datos sensibles. La preparación local no activa opciones de la cuenta de GitHub.

Se recomienda usar la versión de desarrollo más reciente; no se promete un SLA ni
soporte de seguridad para todas las versiones anteriores. No hay firma digital de
los instaladores ni auditoría externa de todas las dependencias. Ninguna prueba
garantiza la ausencia de errores o la recuperación ante cualquier fallo de disco.
