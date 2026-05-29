**Arranque:**

* Carga zonas, atracciones y personas desde CSV  
* Construye lista de zonas con sus atracciones asociadas  
* Registra todas las entradas en un árbol binario, convirtiendo las activas a utilizadas  
* Valida capacidad máxima por zona al ingresar una persona, bloqueando el ingreso si está llena  
* Ingresa al parque solo a personas con entrada activa y las encola en su atracción

**Operación durante el día:**

* Agrega una zona nueva validando código duplicado  
* Modifica los datos de una zona sincronizando arreglo y lista enlazada  
* Elimina una zona validando que no tenga atracciones asociadas  
* Agrega una atracción nueva validando zona existente y código duplicado, inicializando su fila y contador  
* Modifica los datos de una atracción sincronizando arreglo y lista interna de su zona  
* Elimina una atracción vaciando su fila y compactando los arreglos  
* Ingresa un visitante manualmente con selección guiada de tipo de entrada y atracción, validando código duplicado y capacidad de zona  
* Busca una entrada por código mostrando todos sus datos  
* Muestra el total de personas actualmente dentro del parque  
* Lista todos los visitantes actualmente dentro del parque con su entrada y atracción elegida  
* Registra la salida individual de una persona por código de entrada  
* Cambia el estado de una atracción suspendiendo, vaciando o retomando su fila según corresponda  
* Lista todas las atracciones que no están operativas  
* Lista las atracciones de cada zona con su estado y datos de ciclo  
* Consulta cuántas personas hay en la fila de una atracción  
* Calcula tiempo para vaciar una fila completa y tiempo de espera para una posición específica  
* Simula ciclos de atención desencolando grupos de personas de una fila y acumula el contador de atendidos  
* Muestra la atracción con la fila más larga del momento  
* Muestra la atracción más visitada del día según ciclos simulados  
* Calcula los ingresos del día sumando solo las entradas utilizadas  
* Muestra la ocupación de todas las zonas ordenada de mayor a menor según personas en fila  
* Muestra la ocupación actual versus capacidad máxima de cada zona con alertas de zona llena o casi llena

**Cierre:**

* Genera reporte de cierre del día con visitantes ingresados, personas dentro, ingresos, atracción más visitada, fila más larga y ocupación por zona
