#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ZONAS 10
#define MAX_ATRACCIONES 50
#define MAX_VISITANTES 200

struct Zona {
    char nombre[100];
    char codigo[20];
    char tematica[100];
    int horario_apertura;
    int horario_cierre;
    int capacidad;
};

struct Atraccion {
    char codigo_zona[20];
    char nombre[100];
    char codigo[20];
    int minutos_ciclo;
    int personas_ciclo;
    char estado[30];
};

/* Dato temporal de lectura del CSV, puente hacia las estructuras reales */
struct Visitante {
    char nombre[100];
    char codigo_entrada[20];
    char tipo_entrada[20];
    int valor_entrada;
    char codigo_atraccion_elegida[20];
    char estado_entrada[20];
};

/* Entidad real de persona dentro del parque, vinculada a entrada por codigo_entrada */
struct Persona {
    char nombre[100];
    char codigo_entrada[20];
    char codigo_atraccion_elegida[20];
};

/* Entidad real de entrada, usada en el ABB con codigo_entrada como clave */
struct Entrada {
    char codigo_entrada[20];
    char nombre_persona[100];
    char tipo_entrada[20];
    int valor_entrada;
    char estado_entrada[20];
};

/* Nodo del ABB de entradas, ordenado por codigo_entrada */
struct NodoABB {
    struct Entrada dato;
    struct NodoABB *izq;
    struct NodoABB *der;
};

/* Nodo de la lista enlazada de atracciones, cada zona tiene su propia lista */
struct NodoAtraccion {
    struct Atraccion dato;
    struct NodoAtraccion *siguiente;
};

/* Nodo de la lista enlazada de zonas, contiene puntero al head de su lista de atracciones */
struct NodoZona {
    struct Zona dato;
    struct NodoAtraccion *atracciones;
    struct NodoZona *siguiente;
};

/* Lista de zonas con puntero al primer nodo y contador */
struct ListaZonas {
    struct NodoZona *head;
    int cantidad;
};

/* Nodo de la lista doble de personas presentes en el parque */
struct NodoPersona {
    struct Persona dato;
    struct NodoPersona *anterior;
    struct NodoPersona *siguiente;
};

/* Lista doble de personas dentro del parque con contador para control de capacidad */
struct ListaPersonas {
    struct NodoPersona *head;
    struct NodoPersona *tail;
    int cantidad;
};

/* Nodo de la cola de espera de una atraccion */
struct NodoCola {
    struct Persona dato;
    struct NodoCola *siguiente;
};

/* Cola de espera con head, tail, cantidad y flag de suspension por mantenimiento */
struct Cola {
    struct NodoCola *head;
    struct NodoCola *tail;
    int cantidad;
    int suspendida; /* 1 = suspendida por mantenimiento, 0 = activa */
};

struct ListaZonas lista_zonas = {NULL, 0};
struct ListaPersonas lista_personas = {NULL, NULL, 0};
struct NodoABB *abb_entradas = NULL;

struct Zona zonas[MAX_ZONAS];
struct Atraccion atracciones[MAX_ATRACCIONES];
struct Visitante visitantes[MAX_VISITANTES];
int total_zonas = 0;
int total_atracciones = 0;
int total_visitantes = 0;

/* Una cola por cada atraccion, indexada igual que el arreglo atracciones[] */
struct Cola filas[MAX_ATRACCIONES];

/* Contador de personas atendidas por atraccion (para atraccion mas visitada del dia) */
int atendidos_por_atraccion[MAX_ATRACCIONES] = {0};

/* Prototipos necesarios por orden de llamada */
const char *zona_de_atraccion(const char *codigo_atraccion);
int contar_personas_en_zona(const char *codigo_zona);
int capacidad_de_zona(const char *codigo_zona);

/* ─────────────────────────────────────────────
   INSERCION Y CARGA
   ───────────────────────────────────────────── */

/* Inserta una zona al final de lista_zonas */
void insertar_zona(struct Zona z) {
    struct NodoZona *nuevo = (struct NodoZona *)malloc(sizeof(struct NodoZona));
    nuevo->dato = z;
    nuevo->atracciones = NULL;
    nuevo->siguiente = NULL;
    if (lista_zonas.head == NULL)
        lista_zonas.head = nuevo;
    else {
        struct NodoZona *actual = lista_zonas.head;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
    lista_zonas.cantidad++;
}

/* Busca el nodo zona por codigo y le inserta una atraccion en su lista interna */
void insertar_atraccion_en_zona(struct Atraccion a) {
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        if (strcmp(zona->dato.codigo, a.codigo_zona) == 0) {
            struct NodoAtraccion *nuevo = (struct NodoAtraccion *)malloc(sizeof(struct NodoAtraccion));
            nuevo->dato = a;
            nuevo->siguiente = zona->atracciones;
            zona->atracciones = nuevo;
            return;
        }
        zona = zona->siguiente;
    }
}

void cargar_zonas(const char *ruta) {
    char linea[500];
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) { printf("No se pudo abrir zonas.\n"); return; }
    fgets(linea, sizeof(linea), archivo);
    while (fgets(linea, sizeof(linea), archivo)) {
        struct Zona z;
        char *token;
        token = strtok(linea, ","); strcpy(z.nombre, token);
        token = strtok(NULL, ","); strcpy(z.codigo, token);
        token = strtok(NULL, ","); strcpy(z.tematica, token);
        token = strtok(NULL, ","); z.horario_apertura = atoi(token);
        token = strtok(NULL, ","); z.horario_cierre = atoi(token);
        token = strtok(NULL, ","); z.capacidad = atoi(token);
        zonas[total_zonas++] = z;
        insertar_zona(z);
    }
    fclose(archivo);
    printf("Zonas cargadas: %d\n", total_zonas);
}

void cargar_atracciones(const char *ruta) {
    char linea[500];
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) { printf("No se pudo abrir atracciones.\n"); return; }
    fgets(linea, sizeof(linea), archivo);
    while (fgets(linea, sizeof(linea), archivo)) {
        struct Atraccion a;
        char *token;
        token = strtok(linea, ","); strcpy(a.codigo_zona, token);
        token = strtok(NULL, ","); strcpy(a.nombre, token);
        token = strtok(NULL, ","); strcpy(a.codigo, token);
        token = strtok(NULL, ","); a.minutos_ciclo = atoi(token);
        token = strtok(NULL, ","); a.personas_ciclo = atoi(token);
        strcpy(a.estado, "Operativa");
        atracciones[total_atracciones++] = a;
        insertar_atraccion_en_zona(a);
    }
    fclose(archivo);
    printf("Atracciones cargadas: %d\n", total_atracciones);
}

void cargar_visitantes(const char *ruta) {
    char linea[500];
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) { printf("No se pudo abrir visitantes.\n"); return; }
    fgets(linea, sizeof(linea), archivo);
    while (fgets(linea, sizeof(linea), archivo)) {
        struct Visitante v;
        char *token;
        token = strtok(linea, ","); strcpy(v.nombre, token);
        token = strtok(NULL, ","); strcpy(v.codigo_entrada, token);
        token = strtok(NULL, ","); strcpy(v.tipo_entrada, token);
        token = strtok(NULL, ","); v.valor_entrada = atoi(token);
        token = strtok(NULL, ","); strcpy(v.codigo_atraccion_elegida, token);
        token = strtok(NULL, ","); strcpy(v.estado_entrada, token);
        v.estado_entrada[strcspn(v.estado_entrada, "\n")] = '\0';
        visitantes[total_visitantes++] = v;
    }
    fclose(archivo);
    printf("Visitantes cargados: %d\n", total_visitantes);
}

/* ─────────────────────────────────────────────
   ABB DE ENTRADAS
   ───────────────────────────────────────────── */

/* Inserta una entrada en el ABB usando codigo_entrada como clave de comparacion */
struct NodoABB *insertar_abb(struct NodoABB *nodo, struct Entrada e) {
    if (nodo == NULL) {
        struct NodoABB *nuevo = (struct NodoABB *)malloc(sizeof(struct NodoABB));
        nuevo->dato = e;
        nuevo->izq = nuevo->der = NULL;
        return nuevo;
    }
    if (strcmp(e.codigo_entrada, nodo->dato.codigo_entrada) < 0)
        nodo->izq = insertar_abb(nodo->izq, e);
    else
        nodo->der = insertar_abb(nodo->der, e);
    return nodo;
}

/* Busca una entrada en el ABB por codigo y muestra sus datos */
void buscar_entrada_abb(struct NodoABB *nodo, const char *codigo) {
    if (nodo == NULL) { printf("Entrada no encontrada.\n"); return; }
    int cmp = strcmp(codigo, nodo->dato.codigo_entrada);
    if (cmp == 0) {
        printf("Codigo:  %s\n", nodo->dato.codigo_entrada);
        printf("Persona: %s\n", nodo->dato.nombre_persona);
        printf("Tipo:    %s\n", nodo->dato.tipo_entrada);
        printf("Valor:   %d\n", nodo->dato.valor_entrada);
        printf("Estado:  %s\n", nodo->dato.estado_entrada);
    } else if (cmp < 0)
        buscar_entrada_abb(nodo->izq, codigo);
    else
        buscar_entrada_abb(nodo->der, codigo);
}

/* ─────────────────────────────────────────────
   LISTA DOBLE DE PERSONAS
   ───────────────────────────────────────────── */

/* Inserta una persona al final de la lista doble */
void insertar_persona(struct Persona p) {
    struct NodoPersona *nuevo = (struct NodoPersona *)malloc(sizeof(struct NodoPersona));
    nuevo->dato = p;
    nuevo->siguiente = NULL;
    nuevo->anterior = lista_personas.tail;
    if (lista_personas.tail == NULL)
        lista_personas.head = nuevo;
    else
        lista_personas.tail->siguiente = nuevo;
    lista_personas.tail = nuevo;
    lista_personas.cantidad++;
}

/* ─────────────────────────────────────────────
   POBLADO INICIAL
   ───────────────────────────────────────────── */

/* Recorre visitantes[], inserta todas las entradas en el ABB y solo ingresa
   al parque a quienes tienen estado "Activa", marcandola como "Utilizada" */
void poblar_personas_y_entradas() {
    int ingresados = 0;
    for (int i = 0; i < total_visitantes; i++) {
        struct Entrada e;
        strcpy(e.codigo_entrada, visitantes[i].codigo_entrada);
        strcpy(e.nombre_persona, visitantes[i].nombre);
        strcpy(e.tipo_entrada, visitantes[i].tipo_entrada);
        e.valor_entrada = visitantes[i].valor_entrada;
        strcpy(e.estado_entrada, visitantes[i].estado_entrada);

        if (strcmp(e.estado_entrada, "Activa") == 0)
            strcpy(e.estado_entrada, "Utilizada");

        abb_entradas = insertar_abb(abb_entradas, e);

        if (strcmp(visitantes[i].estado_entrada, "Activa") == 0) {
            const char *cod_zona = zona_de_atraccion(visitantes[i].codigo_atraccion_elegida);
            int cap = (cod_zona != NULL) ? capacidad_de_zona(cod_zona) : -1;
            int ocupacion = (cod_zona != NULL) ? contar_personas_en_zona(cod_zona) : 0;

            if (cap != -1 && ocupacion >= cap) {
                printf("Zona llena. No se permite ingreso de %s (zona: %s)\n",
                       visitantes[i].nombre, cod_zona);
            } else {
                struct Persona p;
                strcpy(p.nombre, visitantes[i].nombre);
                strcpy(p.codigo_entrada, visitantes[i].codigo_entrada);
                strcpy(p.codigo_atraccion_elegida, visitantes[i].codigo_atraccion_elegida);
                insertar_persona(p);
                ingresados++;
            }
        }
    }
    printf("Entradas registradas en ABB: %d\n", total_visitantes);
    printf("Personas ingresadas al parque: %d\n", ingresados);
}

/* ─────────────────────────────────────────────
   COLAS DE ESPERA
   ───────────────────────────────────────────── */

/* Agrega una persona al final de la cola de su atraccion */
void encolar(struct Cola *cola, struct Persona p) {
    struct NodoCola *nuevo = (struct NodoCola *)malloc(sizeof(struct NodoCola));
    nuevo->dato = p;
    nuevo->siguiente = NULL;
    if (cola->tail == NULL)
        cola->head = cola->tail = nuevo;
    else {
        cola->tail->siguiente = nuevo;
        cola->tail = nuevo;
    }
    cola->cantidad++;
}

/* Recorre la lista de personas y encola cada una en la fila de su atraccion */
void poblar_filas() {
    for (int i = 0; i < total_atracciones; i++) {
        filas[i].head = NULL;
        filas[i].tail = NULL;
        filas[i].cantidad = 0;
        filas[i].suspendida = 0;
    }
    struct NodoPersona *actual = lista_personas.head;
    while (actual != NULL) {
        for (int j = 0; j < total_atracciones; j++) {
            if (strcmp(actual->dato.codigo_atraccion_elegida, atracciones[j].codigo) == 0) {
                encolar(&filas[j], actual->dato);
                break;
            }
        }
        actual = actual->siguiente;
    }
}

/* Vacia completamente la fila de una atraccion liberando sus nodos */
void vaciar_fila(int idx) {
    struct NodoCola *actual = filas[idx].head;
    while (actual != NULL) {
        struct NodoCola *temp = actual;
        actual = actual->siguiente;
        free(temp);
    }
    filas[idx].head = NULL;
    filas[idx].tail = NULL;
    filas[idx].cantidad = 0;
    filas[idx].suspendida = 0;
}

/* ─────────────────────────────────────────────
   FUNCIONES DE CONSULTA Y GESTION
   ───────────────────────────────────────────── */

/* Busca la atraccion por codigo y retorna cuantas personas hay en su fila */
void personas_por_atraccion() {
    char codigo[20];
    printf("Ingrese codigo de atraccion: ");
    scanf("%s", codigo);
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo) == 0) {
            printf("Personas en fila de %s: %d\n", codigo, filas[i].cantidad);
            return;
        }
    }
    printf("Atraccion no encontrada.\n");
}

/* Pide un codigo de entrada y muestra sus datos buscando en el ABB */
void consultar_entrada() {
    char codigo[20];
    printf("Ingrese codigo de entrada: ");
    scanf("%s", codigo);
    buscar_entrada_abb(abb_entradas, codigo);
}

/* Calcula y muestra el tiempo de espera de la fila completa y de una posicion especifica */
void calcular_tiempo_espera() {
    char codigo[20];
    printf("Ingrese codigo de atraccion: ");
    scanf("%s", codigo);

    int idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo) == 0) { idx = i; break; }
    }

    if (idx == -1) { printf("Atraccion no encontrada.\n"); return; }

    int personas_espera = filas[idx].cantidad;
    int personas_ciclo  = atracciones[idx].personas_ciclo;
    int minutos_ciclo   = atracciones[idx].minutos_ciclo;

    int ciclos_total = (personas_espera + personas_ciclo - 1) / personas_ciclo;
    int tiempo_total = ciclos_total * minutos_ciclo;

    printf("Personas en fila: %d\n", personas_espera);
    printf("Tiempo para vaciar la fila: %d minutos\n", tiempo_total);

    printf("Consultar tiempo para posicion en fila (0 para omitir): ");
    int posicion;
    scanf("%d", &posicion);
    if (posicion > 0 && posicion <= personas_espera) {
        int ciclos_pos = (posicion + personas_ciclo - 1) / personas_ciclo;
        printf("Persona en posicion %d espera: %d minutos\n", posicion, ciclos_pos * minutos_ciclo);
    } else if (posicion > personas_espera) {
        printf("Posicion fuera del rango de la fila.\n");
    }
}

/* Muestra el total de personas actualmente dentro del parque */
void total_personas_parque() {
    printf("Personas en el parque: %d\n", lista_personas.cantidad);
}

/* Busca una persona en lista_personas por codigo_entrada y registra su salida */
void registrar_salida() {
    char codigo[20];
    printf("Ingrese codigo de entrada: ");
    scanf("%s", codigo);

    struct NodoPersona *actual = lista_personas.head;
    while (actual != NULL) {
        if (strcmp(actual->dato.codigo_entrada, codigo) == 0) {
            if (actual->anterior != NULL)
                actual->anterior->siguiente = actual->siguiente;
            else
                lista_personas.head = actual->siguiente;

            if (actual->siguiente != NULL)
                actual->siguiente->anterior = actual->anterior;
            else
                lista_personas.tail = actual->anterior;

            printf("Salida registrada: %s\n", actual->dato.nombre);
            free(actual);
            lista_personas.cantidad--;
            printf("Personas en el parque: %d\n", lista_personas.cantidad);
            return;
        }
        actual = actual->siguiente;
    }
    printf("Persona no encontrada en el parque.\n");
}

/* Recorre el ABB en inorden sumando solo entradas con estado Utilizada */
int sumar_ingresos_abb(struct NodoABB *nodo) {
    if (nodo == NULL) return 0;
    int valor = (strcmp(nodo->dato.estado_entrada, "Utilizada") == 0) ? nodo->dato.valor_entrada : 0;
    return sumar_ingresos_abb(nodo->izq) + valor + sumar_ingresos_abb(nodo->der);
}

/* Muestra el total recaudado por entradas utilizadas en el dia */
void calcular_ingresos() {
    int total = sumar_ingresos_abb(abb_entradas);
    printf("Ingresos totales del dia: %d\n", total);
}

/* Recorre filas[] y muestra la atraccion con mayor cantidad de personas en espera */
void atraccion_fila_mas_larga() {
    int max = -1, idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (filas[i].cantidad > max) { max = filas[i].cantidad; idx = i; }
    }
    if (idx == -1 || max == 0) { printf("No hay personas en ninguna fila.\n"); return; }
    printf("Atraccion con fila mas larga:\n");
    printf("  %s | %s | Personas en espera: %d\n", atracciones[idx].codigo, atracciones[idx].nombre, max);
}

/* Recorre zonas sumando personas en fila y muestra ordenado de mayor a menor */
void zona_mas_ocupada() {
    int sumas[MAX_ZONAS] = {0};
    char nombres[MAX_ZONAS][100];
    int total = 0;

    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        int suma = 0;
        struct NodoAtraccion *at = zona->atracciones;
        while (at != NULL) {
            for (int i = 0; i < total_atracciones; i++) {
                if (strcmp(atracciones[i].codigo, at->dato.codigo) == 0) {
                    suma += filas[i].cantidad;
                    break;
                }
            }
            at = at->siguiente;
        }
        sumas[total] = suma;
        strcpy(nombres[total], zona->dato.nombre);
        total++;
        zona = zona->siguiente;
    }

    for (int i = 0; i < total - 1; i++) {
        for (int j = 0; j < total - i - 1; j++) {
            if (sumas[j] < sumas[j + 1]) {
                int tmp = sumas[j]; sumas[j] = sumas[j + 1]; sumas[j + 1] = tmp;
                char tmpn[100]; strcpy(tmpn, nombres[j]); strcpy(nombres[j], nombres[j + 1]); strcpy(nombres[j + 1], tmpn);
            }
        }
    }

    printf("\nZonas por ocupacion (mayor a menor):\n");
    for (int i = 0; i < total; i++)
        printf("  %d. %s | Personas en fila: %d\n", i + 1, nombres[i], sumas[i]);
}

/* Recorre lista_personas y muestra todos los visitantes dentro del parque */
void listar_visitantes_en_parque() {
    if (lista_personas.cantidad == 0) { printf("No hay personas en el parque.\n"); return; }
    printf("\nVisitantes en el parque (%d):\n", lista_personas.cantidad);
    struct NodoPersona *actual = lista_personas.head;
    while (actual != NULL) {
        printf("  %s | Entrada: %s | Atraccion: %s\n",
               actual->dato.nombre,
               actual->dato.codigo_entrada,
               actual->dato.codigo_atraccion_elegida);
        actual = actual->siguiente;
    }
}

/* Lista todas las atracciones con estado distinto a Operativa */
void listar_atracciones_no_operativas() {
    int encontradas = 0;
    printf("\nAtracciones no operativas:\n");
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].estado, "Operativa") != 0) {
            printf("  %s | %s | Estado: %s\n", atracciones[i].codigo, atracciones[i].nombre, atracciones[i].estado);
            encontradas++;
        }
    }
    if (encontradas == 0)
        printf("  Todas las atracciones estan operativas.\n");
}

/* Cambia el estado de una atraccion y aplica la logica correspondiente a su fila */
void cambiar_estado_atraccion() {
    char codigo[20];
    printf("Ingrese codigo de atraccion: ");
    scanf("%s", codigo);

    int idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo) == 0) { idx = i; break; }
    }

    if (idx == -1) { printf("Atraccion no encontrada.\n"); return; }

    printf("Estado actual: %s\n", atracciones[idx].estado);
    printf("Nuevo estado:\n1. Operativa\n2. Mantenimiento\n3. Fuera de servicio\n4. Cerrada por horario\nOpcion: ");

    int opcion;
    scanf("%d", &opcion);

    if (opcion == 1) {
        strcpy(atracciones[idx].estado, "Operativa");
        filas[idx].suspendida = 0;
        printf("Atraccion operativa. Fila retomada con %d personas.\n", filas[idx].cantidad);
    } else if (opcion == 2) {
        strcpy(atracciones[idx].estado, "Mantenimiento");
        filas[idx].suspendida = 1;
        printf("Atraccion en mantenimiento. Fila suspendida con %d personas en espera.\n", filas[idx].cantidad);
    } else if (opcion == 3) {
        strcpy(atracciones[idx].estado, "Fuera de servicio");
        vaciar_fila(idx);
        printf("Atraccion fuera de servicio. Fila vaciada.\n");
    } else if (opcion == 4) {
        strcpy(atracciones[idx].estado, "Cerrada por horario");
        vaciar_fila(idx);
        printf("Atraccion cerrada por horario. Fila vaciada.\n");
    } else {
        printf("Opcion invalida.\n");
    }
}

/* Desencola N ciclos de personas de una atraccion acumulando contador de atendidos */
void simular_ciclos() {
    char codigo[20];
    printf("Ingrese codigo de atraccion: ");
    scanf("%s", codigo);

    int idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo) == 0) { idx = i; break; }
    }

    if (idx == -1) { printf("Atraccion no encontrada.\n"); return; }

    int personas_espera = filas[idx].cantidad;
    int personas_ciclo  = atracciones[idx].personas_ciclo;
    int minutos_ciclo   = atracciones[idx].minutos_ciclo;

    if (personas_espera == 0) { printf("No hay personas en la fila.\n"); return; }

    int ciclos_maximos = (personas_espera + personas_ciclo - 1) / personas_ciclo;
    printf("Ciclos posibles: %d\nCuantos ciclos simular: ", ciclos_maximos);
    int ciclos;
    scanf("%d", &ciclos);

    if (ciclos > ciclos_maximos) {
        printf("Se ajusta a los ciclos posibles: %d\n", ciclos_maximos);
        ciclos = ciclos_maximos;
    }

    int total_atendidos = 0;
    for (int c = 0; c < ciclos; c++) {
        printf("\nCiclo %d:\n", c + 1);
        int atendidos = 0;
        while (atendidos < personas_ciclo && filas[idx].head != NULL) {
            struct NodoCola *temp = filas[idx].head;
            printf("  Atendido: %s\n", temp->dato.nombre);
            filas[idx].head = filas[idx].head->siguiente;
            free(temp);
            filas[idx].cantidad--;
            atendidos++;
            total_atendidos++;
        }
        if (filas[idx].head == NULL) filas[idx].tail = NULL;
    }

    /* Acumular en el contador global de atendidos por atraccion */
    atendidos_por_atraccion[idx] += total_atendidos;

    printf("\nTotal atendidos: %d | Tiempo simulado: %d minutos | Restantes en fila: %d\n",
           total_atendidos, ciclos * minutos_ciclo, filas[idx].cantidad);
}

/* ─────────────────────────────────────────────
   NUEVAS FUNCIONES
   ───────────────────────────────────────────── */

/* Dado un codigo de atraccion retorna el codigo de zona al que pertenece */
const char *zona_de_atraccion(const char *codigo_atraccion) {
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo_atraccion) == 0)
            return atracciones[i].codigo_zona;
    }
    return NULL;
}

/* Cuenta cuantas personas en lista_personas pertenecen a una zona especifica */
int contar_personas_en_zona(const char *codigo_zona) {
    int cuenta = 0;
    struct NodoPersona *actual = lista_personas.head;
    while (actual != NULL) {
        const char *zona = zona_de_atraccion(actual->dato.codigo_atraccion_elegida);
        if (zona != NULL && strcmp(zona, codigo_zona) == 0)
            cuenta++;
        actual = actual->siguiente;
    }
    return cuenta;
}

/* Retorna la capacidad maxima de una zona por su codigo, -1 si no existe */
int capacidad_de_zona(const char *codigo_zona) {
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        if (strcmp(zona->dato.codigo, codigo_zona) == 0)
            return zona->dato.capacidad;
        zona = zona->siguiente;
    }
    return -1;
}

/* Muestra ocupacion actual vs capacidad maxima de cada zona */
void reporte_capacidad_zonas() {
    printf("\nCapacidad por zona:\n");
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        int ocupacion = contar_personas_en_zona(zona->dato.codigo);
        int capacidad = zona->dato.capacidad;
        printf("  %s | Ocupacion: %d / %d", zona->dato.nombre, ocupacion, capacidad);
        if (ocupacion >= capacidad)
            printf(" [LLENA]");
        else if (ocupacion >= capacidad * 0.8)
            printf(" [CASI LLENA]");
        printf("\n");
        zona = zona->siguiente;
    }
}


/* Cuenta entradas con estado Utilizada recorriendo el ABB en inorden */
int contar_entradas_utilizadas(struct NodoABB *nodo) {
    if (nodo == NULL) return 0;
    int cuenta = (strcmp(nodo->dato.estado_entrada, "Utilizada") == 0) ? 1 : 0;
    return contar_entradas_utilizadas(nodo->izq) + cuenta + contar_entradas_utilizadas(nodo->der);
}

/* Reporte completo de cierre del dia integrando todos los indicadores */
void reporte_cierre_dia() {
    printf("\n========================================\n");
    printf("       REPORTE DE CIERRE - IBCLANDIA\n");
    printf("========================================\n");

    /* Visitantes que ingresaron */
    int ingresados = contar_entradas_utilizadas(abb_entradas);
    printf("\nVisitantes que ingresaron hoy: %d\n", ingresados);

    /* Personas que siguen dentro */
    printf("Personas aun dentro del parque: %d\n", lista_personas.cantidad);

    /* Ingresos del dia */
    int ingresos = sumar_ingresos_abb(abb_entradas);
    printf("Ingresos del dia: $%d\n", ingresos);

    /* Atraccion mas visitada */
    printf("\n--- Atraccion mas visitada ---\n");
    int max = -1, idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (atendidos_por_atraccion[i] > max) { max = atendidos_por_atraccion[i]; idx = i; }
    }
    if (idx == -1 || max == 0)
        printf("  Sin ciclos simulados en el dia.\n");
    else
        printf("  %s | %s | Personas atendidas: %d\n",
               atracciones[idx].codigo, atracciones[idx].nombre, max);

    /* Atraccion con fila mas larga al cierre */
    printf("\n--- Fila mas larga al cierre ---\n");
    int fmax = -1, fidx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (filas[i].cantidad > fmax) { fmax = filas[i].cantidad; fidx = i; }
    }
    if (fidx == -1 || fmax == 0)
        printf("  No hay personas en ninguna fila.\n");
    else
        printf("  %s | %s | Personas en espera: %d\n",
               atracciones[fidx].codigo, atracciones[fidx].nombre, fmax);

    /* Ocupacion por zona al cierre */
    printf("\n--- Ocupacion por zona al cierre ---\n");
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        int ocupacion = contar_personas_en_zona(zona->dato.codigo);
        int capacidad = zona->dato.capacidad;
        printf("  %s | %d / %d", zona->dato.nombre, ocupacion, capacidad);
        if (ocupacion >= capacidad)
            printf(" [LLENA]");
        else if (ocupacion >= capacidad * 0.8)
            printf(" [CASI LLENA]");
        printf("\n");
        zona = zona->siguiente;
    }

    printf("\n========================================\n");
}

void atraccion_mas_visitada() {
    int max = -1, idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (atendidos_por_atraccion[i] > max) {
            max = atendidos_por_atraccion[i];
            idx = i;
        }
    }
    if (idx == -1 || max == 0) {
        printf("Aun no se han atendido personas en ninguna atraccion.\n");
        return;
    }
    printf("Atraccion mas visitada del dia:\n");
    printf("  %s | %s | Personas atendidas: %d\n",
           atracciones[idx].codigo, atracciones[idx].nombre, max);
}

/* Recorre lista_zonas y por cada zona lista sus atracciones con estado y datos de ciclo */
void listar_atracciones_por_zona() {
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        printf("\nZona: %s (%s)\n", zona->dato.nombre, zona->dato.codigo);
        struct NodoAtraccion *at = zona->atracciones;
        if (at == NULL) {
            printf("  Sin atracciones registradas.\n");
        } else {
            while (at != NULL) {
                printf("  - %s | %s | Estado: %s | Ciclo: %d min | %d personas/ciclo\n",
                       at->dato.codigo,
                       at->dato.nombre,
                       at->dato.estado,
                       at->dato.minutos_ciclo,
                       at->dato.personas_ciclo);
                at = at->siguiente;
            }
        }
        zona = zona->siguiente;
    }
}

void ingresar_visitante_manual() {
    struct Persona p;
    struct Entrada e;

    printf("Nombre: ");
    scanf(" %[^\n]", p.nombre);
    strcpy(e.nombre_persona, p.nombre);

    printf("Codigo de entrada: ");
    scanf("%s", p.codigo_entrada);
    /* Validar que el codigo no exista ya en el ABB */
    /* Reutilizamos busqueda simple recorriendo el ABB */
    struct NodoABB *nodo = abb_entradas;
    while (nodo != NULL) {
        int cmp = strcmp(p.codigo_entrada, nodo->dato.codigo_entrada);
        if (cmp == 0) { printf("Ya existe una entrada con ese codigo.\n"); return; }
        else if (cmp < 0) nodo = nodo->izq;
        else nodo = nodo->der;
    }
    strcpy(e.codigo_entrada, p.codigo_entrada);

    printf("Tipo de entrada:\n");
    printf("  1. General   ($3000)\n");
    printf("  2. Familiar  ($5000)\n");
    printf("  3. Infantil  ($2000)\n");
    printf("  4. Rapido    ($6000)\n");
    printf("Opcion: ");
    int tipo;
    scanf("%d", &tipo);
    switch (tipo) {
        case 1: strcpy(e.tipo_entrada, "General");   e.valor_entrada = 3000; break;
        case 2: strcpy(e.tipo_entrada, "Familiar");  e.valor_entrada = 5000; break;
        case 3: strcpy(e.tipo_entrada, "Infantil");  e.valor_entrada = 2000; break;
        case 4: strcpy(e.tipo_entrada, "Rapido");    e.valor_entrada = 6000; break;
        default: printf("Opcion invalida.\n"); return;
    }

    /* Listar atracciones disponibles para elegir */
    printf("Atraccion elegida:\n");
    int operativas = 0;
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].estado, "Operativa") == 0) {
            printf("  %d. %s | %s\n", i + 1, atracciones[i].codigo, atracciones[i].nombre);
            operativas++;
        }
    }
    if (operativas == 0) { printf("No hay atracciones operativas.\n"); return; }
    printf("Numero: ");
    int op;
    scanf("%d", &op);
    if (op < 1 || op > total_atracciones || strcmp(atracciones[op - 1].estado, "Operativa") != 0) {
        printf("Seleccion invalida.\n"); return;
    }
    strcpy(p.codigo_atraccion_elegida, atracciones[op - 1].codigo);

    /* Verificar capacidad de la zona */
    const char *cod_zona = zona_de_atraccion(p.codigo_atraccion_elegida);
    int cap = (cod_zona != NULL) ? capacidad_de_zona(cod_zona) : -1;
    int ocupacion = (cod_zona != NULL) ? contar_personas_en_zona(cod_zona) : 0;
    if (cap != -1 && ocupacion >= cap) {
        printf("Zona llena. No se permite el ingreso.\n"); return;
    }

    /* Registrar entrada como Utilizada en el ABB */
    strcpy(e.estado_entrada, "Utilizada");
    abb_entradas = insertar_abb(abb_entradas, e);

    /* Ingresar persona al parque y a su fila */
    insertar_persona(p);
    encolar(&filas[op - 1], p);

    printf("Visitante %s ingresado correctamente a la fila de %s.\n",
           p.nombre, atracciones[op - 1].nombre);
    printf("Personas en el parque: %d\n", lista_personas.cantidad);
}



/* Retorna 1 si el codigo de zona ya existe en lista_zonas */
int zona_existe(const char *codigo) {
    struct NodoZona *actual = lista_zonas.head;
    while (actual != NULL) {
        if (strcmp(actual->dato.codigo, codigo) == 0) return 1;
        actual = actual->siguiente;
    }
    return 0;
}

/* Retorna 1 si el codigo de atraccion ya existe en atracciones[] */
int atraccion_existe(const char *codigo) {
    for (int i = 0; i < total_atracciones; i++)
        if (strcmp(atracciones[i].codigo, codigo) == 0) return 1;
    return 0;
}

void agregar_zona() {
    if (total_zonas >= MAX_ZONAS) { printf("Limite de zonas alcanzado.\n"); return; }
    struct Zona z;
    printf("Nombre: "); scanf(" %[^\n]", z.nombre);
    printf("Codigo: "); scanf("%s", z.codigo);
    if (zona_existe(z.codigo)) { printf("Ya existe una zona con ese codigo.\n"); return; }
    printf("Tematica: "); scanf(" %[^\n]", z.tematica);
    printf("Horario apertura (hora): "); scanf("%d", &z.horario_apertura);
    printf("Horario cierre (hora): "); scanf("%d", &z.horario_cierre);
    printf("Capacidad: "); scanf("%d", &z.capacidad);
    zonas[total_zonas++] = z;
    insertar_zona(z);
    printf("Zona agregada correctamente.\n");
}

void modificar_zona() {
    char codigo[20];
    printf("Codigo de zona a modificar: ");
    scanf("%s", codigo);
    /* Modificar en arreglo zonas[] */
    int idx = -1;
    for (int i = 0; i < total_zonas; i++) {
        if (strcmp(zonas[i].codigo, codigo) == 0) { idx = i; break; }
    }
    if (idx == -1) { printf("Zona no encontrada.\n"); return; }
    printf("Nuevo nombre (%s): ", zonas[idx].nombre); scanf(" %[^\n]", zonas[idx].nombre);
    printf("Nueva tematica (%s): ", zonas[idx].tematica); scanf(" %[^\n]", zonas[idx].tematica);
    printf("Nuevo horario apertura (%d): ", zonas[idx].horario_apertura); scanf("%d", &zonas[idx].horario_apertura);
    printf("Nuevo horario cierre (%d): ", zonas[idx].horario_cierre); scanf("%d", &zonas[idx].horario_cierre);
    printf("Nueva capacidad (%d): ", zonas[idx].capacidad); scanf("%d", &zonas[idx].capacidad);
    /* Sincronizar en lista_zonas */
    struct NodoZona *nodo = lista_zonas.head;
    while (nodo != NULL) {
        if (strcmp(nodo->dato.codigo, codigo) == 0) { nodo->dato = zonas[idx]; break; }
        nodo = nodo->siguiente;
    }
    printf("Zona modificada correctamente.\n");
}

void eliminar_zona() {
    char codigo[20];
    printf("Codigo de zona a eliminar: ");
    scanf("%s", codigo);
    /* Verificar que no tenga atracciones asociadas */
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo_zona, codigo) == 0) {
            printf("No se puede eliminar: la zona tiene atracciones asociadas.\n");
            return;
        }
    }
    /* Eliminar de lista_zonas */
    struct NodoZona *actual = lista_zonas.head, *anterior = NULL;
    while (actual != NULL) {
        if (strcmp(actual->dato.codigo, codigo) == 0) {
            if (anterior == NULL) lista_zonas.head = actual->siguiente;
            else anterior->siguiente = actual->siguiente;
            free(actual);
            lista_zonas.cantidad--;
            /* Eliminar de arreglo zonas[] */
            for (int i = 0; i < total_zonas; i++) {
                if (strcmp(zonas[i].codigo, codigo) == 0) {
                    for (int j = i; j < total_zonas - 1; j++) zonas[j] = zonas[j + 1];
                    total_zonas--;
                    break;
                }
            }
            printf("Zona eliminada correctamente.\n");
            return;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
    printf("Zona no encontrada.\n");
}

void agregar_atraccion() {
    if (total_atracciones >= MAX_ATRACCIONES) { printf("Limite de atracciones alcanzado.\n"); return; }
    struct Atraccion a;
    printf("Codigo de zona: "); scanf("%s", a.codigo_zona);
    if (!zona_existe(a.codigo_zona)) { printf("La zona no existe.\n"); return; }
    printf("Nombre: "); scanf(" %[^\n]", a.nombre);
    printf("Codigo: "); scanf("%s", a.codigo);
    if (atraccion_existe(a.codigo)) { printf("Ya existe una atraccion con ese codigo.\n"); return; }
    printf("Minutos por ciclo: "); scanf("%d", &a.minutos_ciclo);
    printf("Personas por ciclo: "); scanf("%d", &a.personas_ciclo);
    strcpy(a.estado, "Operativa");
    atracciones[total_atracciones] = a;
    filas[total_atracciones].head = NULL;
    filas[total_atracciones].tail = NULL;
    filas[total_atracciones].cantidad = 0;
    filas[total_atracciones].suspendida = 0;
    atendidos_por_atraccion[total_atracciones] = 0;
    total_atracciones++;
    insertar_atraccion_en_zona(a);
    printf("Atraccion agregada correctamente.\n");
}

void modificar_atraccion() {
    char codigo[20];
    printf("Codigo de atraccion a modificar: ");
    scanf("%s", codigo);
    int idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo) == 0) { idx = i; break; }
    }
    if (idx == -1) { printf("Atraccion no encontrada.\n"); return; }
    printf("Nuevo nombre (%s): ", atracciones[idx].nombre); scanf(" %[^\n]", atracciones[idx].nombre);
    printf("Nuevos minutos por ciclo (%d): ", atracciones[idx].minutos_ciclo); scanf("%d", &atracciones[idx].minutos_ciclo);
    printf("Nuevas personas por ciclo (%d): ", atracciones[idx].personas_ciclo); scanf("%d", &atracciones[idx].personas_ciclo);
    /* Sincronizar en lista de zona */
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        if (strcmp(zona->dato.codigo, atracciones[idx].codigo_zona) == 0) {
            struct NodoAtraccion *at = zona->atracciones;
            while (at != NULL) {
                if (strcmp(at->dato.codigo, codigo) == 0) { at->dato = atracciones[idx]; break; }
                at = at->siguiente;
            }
            break;
        }
        zona = zona->siguiente;
    }
    printf("Atraccion modificada correctamente.\n");
}

void eliminar_atraccion() {
    char codigo[20];
    printf("Codigo de atraccion a eliminar: ");
    scanf("%s", codigo);
    int idx = -1;
    for (int i = 0; i < total_atracciones; i++) {
        if (strcmp(atracciones[i].codigo, codigo) == 0) { idx = i; break; }
    }
    if (idx == -1) { printf("Atraccion no encontrada.\n"); return; }
    /* Vaciar su fila antes de eliminar */
    vaciar_fila(idx);
    /* Eliminar de lista interna de su zona */
    struct NodoZona *zona = lista_zonas.head;
    while (zona != NULL) {
        if (strcmp(zona->dato.codigo, atracciones[idx].codigo_zona) == 0) {
            struct NodoAtraccion *actual = zona->atracciones, *anterior = NULL;
            while (actual != NULL) {
                if (strcmp(actual->dato.codigo, codigo) == 0) {
                    if (anterior == NULL) zona->atracciones = actual->siguiente;
                    else anterior->siguiente = actual->siguiente;
                    free(actual);
                    break;
                }
                anterior = actual;
                actual = actual->siguiente;
            }
            break;
        }
        zona = zona->siguiente;
    }
    /* Eliminar de arreglo atracciones[] compactando */
    for (int i = idx; i < total_atracciones - 1; i++) {
        atracciones[i] = atracciones[i + 1];
        filas[i] = filas[i + 1];
        atendidos_por_atraccion[i] = atendidos_por_atraccion[i + 1];
    }
    total_atracciones--;
    printf("Atraccion eliminada correctamente.\n");
}



/* Simula N ciclos para todas las atracciones operativas con personas en fila */
void simular_ciclos_todas() {
    printf("Cuantos ciclos simular por atraccion: ");
    int ciclos;
    scanf("%d", &ciclos);
    if (ciclos <= 0) { printf("Cantidad invalida.\n"); return; }

    int hubo_actividad = 0;
    printf("\n--- Simulacion global de %d ciclo(s) ---\n", ciclos);

    for (int idx = 0; idx < total_atracciones; idx++) {
        if (strcmp(atracciones[idx].estado, "Operativa") != 0) continue;
        if (filas[idx].cantidad == 0) continue;

        hubo_actividad = 1;
        int personas_ciclo  = atracciones[idx].personas_ciclo;
        int minutos_ciclo   = atracciones[idx].minutos_ciclo;
        int ciclos_maximos  = (filas[idx].cantidad + personas_ciclo - 1) / personas_ciclo;
        int ciclos_a_correr = (ciclos > ciclos_maximos) ? ciclos_maximos : ciclos;

        int total_atendidos = 0;
        for (int c = 0; c < ciclos_a_correr; c++) {
            int atendidos = 0;
            while (atendidos < personas_ciclo && filas[idx].head != NULL) {
                struct NodoCola *temp = filas[idx].head;
                filas[idx].head = filas[idx].head->siguiente;
                free(temp);
                filas[idx].cantidad--;
                atendidos++;
                total_atendidos++;
            }
            if (filas[idx].head == NULL) filas[idx].tail = NULL;
        }

        atendidos_por_atraccion[idx] += total_atendidos;
        printf("  %s | %s | Atendidos: %d | Tiempo: %d min | Restantes en fila: %d\n",
               atracciones[idx].codigo, atracciones[idx].nombre,
               total_atendidos, ciclos_a_correr * minutos_ciclo, filas[idx].cantidad);
    }

    if (!hubo_actividad)
        printf("  No hay atracciones operativas con personas en fila.\n");
}

void menu_zonas() {
    int opcion;
    while (1) {
        printf("\n-- Zonas --\n");
        printf("1. Ocupacion por zona\n");
        printf("2. Listar atracciones por zona\n");
        printf("3. Capacidad por zona\n");
        printf("4. Agregar zona\n");
        printf("5. Modificar zona\n");
        printf("6. Eliminar zona\n");
        printf("0. Volver\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        if (opcion == 1) zona_mas_ocupada();
        else if (opcion == 2) listar_atracciones_por_zona();
        else if (opcion == 3) reporte_capacidad_zonas();
        else if (opcion == 4) agregar_zona();
        else if (opcion == 5) modificar_zona();
        else if (opcion == 6) eliminar_zona();
        else if (opcion == 0) break;
    }
}

void menu_atracciones() {
    int opcion;
    while (1) {
        printf("\n-- Atracciones --\n");
        printf("1. Cambiar estado de atraccion\n");
        printf("2. Listar atracciones no operativas\n");
        printf("3. Atraccion mas visitada del dia\n");
        printf("4. Agregar atraccion\n");
        printf("5. Modificar atraccion\n");
        printf("6. Eliminar atraccion\n");
        printf("0. Volver\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        if (opcion == 1) cambiar_estado_atraccion();
        else if (opcion == 2) listar_atracciones_no_operativas();
        else if (opcion == 3) atraccion_mas_visitada();
        else if (opcion == 4) agregar_atraccion();
        else if (opcion == 5) modificar_atraccion();
        else if (opcion == 6) eliminar_atraccion();
        else if (opcion == 0) break;
    }
}

void menu_filas() {
    int opcion;
    while (1) {
        printf("\n-- Gestion de filas --\n");
        printf("1. Personas en fila de una atraccion\n");
        printf("2. Tiempo de espera\n");
        printf("3. Simular ciclos de atencion (una atraccion)\n");
        printf("4. Simular ciclos de atencion (todas las operativas)\n");
        printf("5. Atraccion con fila mas larga\n");
        printf("0. Volver\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        if (opcion == 1) personas_por_atraccion();
        else if (opcion == 2) calcular_tiempo_espera();
        else if (opcion == 3) simular_ciclos();
        else if (opcion == 4) simular_ciclos_todas();
        else if (opcion == 5) atraccion_fila_mas_larga();
        else if (opcion == 0) break;
    }
}

void menu_entradas() {
    int opcion;
    while (1) {
        printf("\n-- Entradas --\n");
        printf("1. Buscar entrada por codigo\n");
        printf("0. Volver\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        if (opcion == 1) consultar_entrada();
        else if (opcion == 0) break;
    }
}

void menu_visitantes() {
    int opcion;
    while (1) {
        printf("\n-- Visitantes --\n");
        printf("1. Total de personas en el parque\n");
        printf("2. Registrar salida de persona\n");
        printf("3. Listar visitantes en el parque\n");
        printf("4. Ingresar visitante manualmente\n");
        printf("0. Volver\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        if (opcion == 1) total_personas_parque();
        else if (opcion == 2) registrar_salida();
        else if (opcion == 3) listar_visitantes_en_parque();
        else if (opcion == 4) ingresar_visitante_manual();
        else if (opcion == 0) break;
    }
}

/* ─────────────────────────────────────────────
   MAIN
   ───────────────────────────────────────────── */

int main() {
    cargar_zonas("C:\\Users\\seban\\OneDrive\\Desktop\\IBCLandia\\zonas.csv");
    cargar_atracciones("C:\\Users\\seban\\OneDrive\\Desktop\\IBCLandia\\atracciones.csv");
    cargar_visitantes("C:\\Users\\seban\\OneDrive\\Desktop\\IBCLandia\\persona.csv");

    poblar_personas_y_entradas();
    poblar_filas();

    int opcion;
    while (1) {
        printf("\n=== IBCLandia ===\n");
        printf("1. Gestion de filas\n");
        printf("2. Entradas\n");
        printf("3. Visitantes\n");
        printf("4. Atracciones\n");
        printf("5. Zonas\n");
        printf("6. Ingresos del dia\n");
        printf("7. Reporte de cierre del dia\n");
        printf("0. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opcion);
        if (opcion == 1) menu_filas();
        else if (opcion == 2) menu_entradas();
        else if (opcion == 3) menu_visitantes();
        else if (opcion == 4) menu_atracciones();
        else if (opcion == 5) menu_zonas();
        else if (opcion == 6) calcular_ingresos();
        else if (opcion == 7) reporte_cierre_dia();
        else if (opcion == 0) break;
    }

    return 0;
}