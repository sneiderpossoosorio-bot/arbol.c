 Sistema de Gestión de Tiquetes para el Muelle Turístico

Proyecto desarrollado como parte del taller de Estructuras de Datos, implementando un Árbol Binario de Búsqueda (ABB) en lenguaje C para administrar la venta y reserva de tiquetes de lancha.



 Objetivo del Proyecto

Implementar un sistema que permita gestionar pasajeros utilizando un Árbol Binario de Búsqueda, donde cada pasajero se identifica por su número de documento, el cual funciona como clave para insertar, buscar y eliminar nodos dentro del ABB.



 Características del Sistema

1. Estructura del Nodo (Pasajero)

Cada pasajero está representado por una estructura con los siguientes campos:
	•	documento: número entero (clave única de búsqueda)
	•	destino: string (Timbiquí, Juanchaco, Tumaco, Guapi)
	•	tipo_pasaje: “Ida” o “Ida y Regreso”



2. Registro de Pasajeros

El sistema permite insertar pasajeros dentro del ABB, ordenados por documento.
	•	Si el documento ya existe, el nodo es ignorado.

⸻

3. 📋 Listado de Viajeros

Se pueden mostrar los pasajeros usando recorridos clásicos del ABB:
	•	Inorden
	•	Preorden
	•	Postorden


4. Conteo de Pasajeros

Función que devuelve cuántos pasajeros están actualmente registrados en el sistema.


5.  Eliminación de Pasajeros

Se implementa la función para eliminar un pasajero utilizando su número de documento.

 
6.  Menú Interactivo

El programa incluye un menú que permite ejecutar todas las funciones:
	•	Insertar pasajero
	•	Listar pasajeros (inorden / preorden / postorden)
	•	Contar pasajeros
	•	Eliminar pasajero
	•	Salir
