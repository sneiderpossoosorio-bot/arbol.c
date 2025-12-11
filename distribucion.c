/*
 * SISTEMA LOGISTICO - PUERTO DE DISTRIBUCION DE ALIMENTOS BUENAVENTURA
 * 
 * Este programa gestiona el inventario de productos perecederos organizados por
 * fecha de vencimiento usando un arbol AVL balanceado. Cada nodo del arbol contiene
 * una cola FIFO de pedidos de despacho.
 * 
 * Estructura de datos hibrida:
 * - Arbol AVL: Organiza lotes por fecha de vencimiento (AAAAMMDD)
 * - Cola FIFO: Gestiona pedidos de despacho dentro de cada lote
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* Constantes de configuracion */
#define MAX_NAME 64      // Longitud maxima del nombre del producto
#define MAX_DEST 64      // Longitud maxima del nombre del destino
#define MIN_YEAR 2000    // Anio minimo valido para fechas
#define MAX_YEAR 2100    // Anio maximo valido para fechas
#define ARCHIVO_DATOS "inventario.dat"  // Archivo para persistencia

/**
 * Estructura Order: Representa un pedido de despacho en la cola FIFO
 * 
 * Campos:
 * - nombre_destino: Nombre del destino del pedido (ej: "Nuqui", "Guapi")
 * - cantidad_solicitada: Cantidad de productos solicitados en el pedido
 * - siguiente: Puntero al siguiente pedido en la cola (estructura FIFO)
 */
typedef struct Order {
    char nombre_destino[MAX_DEST];   // Nombre del destino segun especificación
    int cantidad_solicitada;          // Cantidad solicitada segun especificación
    struct Order *siguiente;          // Puntero al siguiente pedido segun especificación
} Order;

/**
 * Estructura Node: Representa un nodo del arbol AVL (un lote de productos)
 * 
 * Campos:
 * - fecha_vencimiento: Fecha de vencimiento en formato AAAAMMDD (clave del arbol)
 * - producto: Nombre del producto almacenado en este lote
 * - stock_total: Cantidad total de productos disponibles en el lote
 * - cabeza_pedidos: Puntero al primer pedido en la cola FIFO (head de la cola)
 * - tail: Puntero al ultimo pedido en la cola FIFO (para insercion eficiente O(1))
 * - left, right: Punteros a los hijos izquierdo y derecho del arbol AVL
 * - height: Altura del nodo en el arbol (para balanceo AVL)
 */
typedef struct Node {
    int fecha_vencimiento;            // Fecha de vencimiento AAAAMMDD (clave del arbol)
    char producto[MAX_NAME];          // Nombre del producto
    int stock_total;                  // Stock total disponible segun especificacion
    Order *cabeza_pedidos;            // Cabeza de la cola FIFO (primer pedido)
    Order *tail;                      // Cola de la cola FIFO (ultimo pedido, para eficiencia)
    struct Node *left, *right;       // Hijos izquierdo y derecho del arbol AVL
    int height;                       // Altura del nodo para balanceo AVL
} Node;

/* ============================================================================
 * UTILIDADES DEL ARBOL AVL
 * ============================================================================ */

/**
 * Funcion: max
 * Descripcion: Retorna el maximo entre dos enteros
 * Parametros: a, b - dos numeros enteros
 * Retorna: El mayor de los dos numeros
 */
int max(int a, int b) { 
    return (a > b) ? a : b; 
}

/**
 * Funcion: height
 * Descripcion: Obtiene la altura de un nodo en el arbol AVL
 * Parametros: n - puntero al nodo (puede ser NULL)
 * Retorna: La altura del nodo, o 0 si el nodo es NULL
 */
int height(Node *n) { 
    return n ? n->height : 0; 
}

/* ============================================================================
 * VALIDACION DE FECHAS
 * ============================================================================ */

/**
 * Funcion: convertir_fecha_a_int
 * Descripcion: Convierte fecha en formato DD/MM/YYYY a AAAAMMDD
 * Parametros: dia, mes, anio - componentes de la fecha
 * Retorna: Fecha en formato AAAAMMDD, o -1 si es invalida
 * 
 * Esta funcion permite ingresar fechas en formato mas amigable (DD MM YYYY)
 * y las convierte al formato interno usado por el arbol AVL (AAAAMMDD).
 * 
 * Ejemplo: convertir_fecha_a_int(4, 12, 2025) retorna 20251204
 */
int convertir_fecha_a_int(int dia, int mes, int anio) {
    // Validar rango del anio
    if (anio < MIN_YEAR || anio > MAX_YEAR) return -1;
    
    // Validar rango del mes
    if (mes < 1 || mes > 12) return -1;
    
    // Validar rango basico del dia
    if (dia < 1 || dia > 31) return -1;
    
    // Validar dias por mes (considera febrero con 29 dias para anios bisiestos)
    int dias_mes[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (dia > dias_mes[mes - 1]) return -1;
    
    // Convertir a formato AAAAMMDD
    return anio * 10000 + mes * 100 + dia;
}

/**
 * Funcion: formatear_fecha
 * Descripcion: Convierte fecha AAAAMMDD a string legible DD/MM/YYYY
 * Parametros: fecha - fecha en formato AAAAMMDD
 * Retorna: String con formato DD/MM/YYYY (buffer estatico)
 * 
 * Esta funcion mejora la visualizacion de fechas mostrandolas en formato
 * mas legible para el usuario. Usa un buffer estatico para evitar problemas
 * de memoria.
 * 
 * Ejemplo: formatear_fecha(20251204) retorna "04/12/2025"
 */
char* formatear_fecha(int fecha) {
    static char buffer[12];
    int anio = fecha / 10000;
    int mes = (fecha / 100) % 100;
    int dia = fecha % 100;
    sprintf(buffer, "%02d/%02d/%04d", dia, mes, anio);
    return buffer;
}

/**
 * Funcion: validar_fecha
 * Descripcion: Valida que una fecha en formato AAAAMMDD sea correcta
 * Parametros: fecha - fecha en formato AAAAMMDD (ej: 20251204)
 * Retorna: true si la fecha es valida, false en caso contrario
 * 
 * Validaciones realizadas:
 * - Anio entre MIN_YEAR y MAX_YEAR
 * - Mes entre 1 y 12
 * - Dia valido segun el mes (considera anios bisiestos basicamente)
 */
bool validar_fecha(int fecha) {
    // Extraer anio, mes y dia del formato AAAAMMDD
    int anio = fecha / 10000;
    int mes = (fecha / 100) % 100;
    int dia = fecha % 100;
    
    // Validar rango del anio
    if (anio < MIN_YEAR || anio > MAX_YEAR) return false;
    
    // Validar rango del mes
    if (mes < 1 || mes > 12) return false;
    
    // Validar rango basico del dia
    if (dia < 1 || dia > 31) return false;
    
    // Validacion de dias por mes (considera febrero con 29 dias para anios bisiestos)
    int dias_mes[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (dia > dias_mes[mes - 1]) return false;
    
    return true;
}

/* ============================================================================
 * CREACION Y MANIPULACION DE NODOS
 * ============================================================================ */

/**
 * Funcion: newNode
 * Descripcion: Crea un nuevo nodo del arbol AVL con los datos proporcionados
 * Parametros:
 *   - fecha: Fecha de vencimiento en formato AAAAMMDD
 *   - producto: Nombre del producto
 *   - stock: Cantidad inicial de stock
 * Retorna: Puntero al nuevo nodo creado, o NULL si hay error de memoria
 * 
 * Nota: El nodo se inicializa con altura 1 y cola FIFO vacia
 */
Node* newNode(int fecha, const char *producto, int stock) {
    // Asignar memoria para el nuevo nodo
    Node *n = (Node*)malloc(sizeof(Node));
    if (!n) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nodo.\n");
        return NULL;
    }
    
    // Inicializar campos del nodo
    n->fecha_vencimiento = fecha;
    strncpy(n->producto, producto, MAX_NAME-1);
    n->producto[MAX_NAME-1] = '\0';  // Asegurar terminacion de cadena
    n->stock_total = stock;
    
    // Inicializar cola FIFO vacia
    n->cabeza_pedidos = n->tail = NULL;
    
    // Inicializar hijos del arbol como NULL
    n->left = n->right = NULL;
    
    // Altura inicial de un nodo hoja es 1
    n->height = 1;
    
    return n;
}

/* ============================================================================
 * ROTACIONES DEL ARBOL AVL
 * ============================================================================ */

/**
 * Funcion: rightRotate
 * Descripcion: Realiza una rotacion simple a la derecha (caso LL)
 * Parametros: y - nodo desbalanceado que sera rotado
 * Retorna: Nueva raiz del subarbol despues de la rotacion
 * 
 * Caso: Desbalance hacia la izquierda (left-left)
 * Estructura antes:      Estructura despues:
 *        y                    x
 *       / \                  / \
 *      x   T3    =>        T1   y
 *     / \                      / \
 *    T1  T2                  T2  T3
 */
Node* rightRotate(Node *y) {
    Node *x = y->left;        // x es el hijo izquierdo de y
    Node *T2 = x->right;      // T2 es el subarbol derecho de x
    
    // Realizar la rotacion
    x->right = y;             // y se convierte en hijo derecho de x
    y->left = T2;             // T2 se convierte en hijo izquierdo de y
    
    // Actualizar alturas (primero y, luego x porque y depende de x)
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;
    
    return x;  // x es la nueva raiz
}

/**
 * Funcion: leftRotate
 * Descripcion: Realiza una rotacion simple a la izquierda (caso RR)
 * Parametros: x - nodo desbalanceado que sera rotado
 * Retorna: Nueva raiz del subarbol despues de la rotacion
 * 
 * Caso: Desbalance hacia la derecha (right-right)
 * Estructura antes:      Estructura despues:
 *        x                    y
 *       / \                  / \
 *      T1  y      =>        x   T3
 *         / \              / \
 *        T2  T3          T1  T2
 */
Node* leftRotate(Node *x) {
    Node *y = x->right;        // y es el hijo derecho de x
    Node *T2 = y->left;       // T2 es el subarbol izquierdo de y
    
    // Realizar la rotacion
    y->left = x;              // x se convierte en hijo izquierdo de y
    x->right = T2;            // T2 se convierte en hijo derecho de x
    
    // Actualizar alturas (primero x, luego y porque y depende de x)
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;
    
    return y;  // y es la nueva raiz
}

/**
 * Funcion: getBalance
 * Descripcion: Calcula el factor de balance de un nodo (diferencia de alturas)
 * Parametros: n - puntero al nodo
 * Retorna: Factor de balance (altura_izq - altura_der)
 * 
 * Valores posibles:
 * - 0: Árbol balanceado
 * - > 1: Desbalanceado hacia la izquierda (necesita rotacion derecha)
 * - < -1: Desbalanceado hacia la derecha (necesita rotacion izquierda)
 */
int getBalance(Node *n) {
    if (!n) return 0;
    return height(n->left) - height(n->right);
}

/* ============================================================================
 * OPERACIONES DE COLA FIFO (First In, First Out)
 * ============================================================================ */

/**
 * Funcion: enqueue_order
 * Descripcion: Agrega un nuevo pedido al final de la cola FIFO de un nodo
 * Parametros:
 *   - node: Nodo del arbol AVL donde se agregará el pedido
 *   - destino: Nombre del destino del pedido
 *   - cantidad: Cantidad solicitada en el pedido
 * Retorna: true si se agregó correctamente, false si hay error
 * 
 * Complejidad: O(1) gracias al uso del puntero tail
 */
bool enqueue_order(Node *node, const char *destino, int cantidad) {
    if (!node) return false;
    
    // Crear nuevo pedido
    Order *o = (Order*)malloc(sizeof(Order));
    if (!o) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el pedido.\n");
        return false;
    }
    
    // Inicializar datos del pedido
    strncpy(o->nombre_destino, destino, MAX_DEST-1);
    o->nombre_destino[MAX_DEST-1] = '\0';  // Asegurar terminacion de cadena
    o->cantidad_solicitada = cantidad;
    o->siguiente = NULL;
    
    // Agregar al final de la cola FIFO
    if (!node->cabeza_pedidos) {
        // Cola vacia: el nuevo pedido es tanto cabeza como cola
        node->cabeza_pedidos = node->tail = o;
    } else {
        // Cola no vacia: agregar despues del último elemento
        node->tail->siguiente = o;
        node->tail = o;  // Actualizar puntero tail
    }
    
    return true;
}

/**
 * Funcion: count_orders
 * Descripcion: Cuenta cuántos pedidos hay en la cola FIFO de un nodo
 * Parametros: node - puntero al nodo del arbol
 * Retorna: Número de pedidos en la cola (0 si está vacia o node es NULL)
 * 
 * Complejidad: O(n) donde n es el numero de pedidos en la cola
 */
int count_orders(Node *node) {
    int c = 0;
    Order *p = node ? node->cabeza_pedidos : NULL;
    while (p) { 
        c++; 
        p = p->siguiente; 
    }
    return c;
}

/**
 * Funcion: free_orders
 * Descripcion: Libera toda la memoria de la cola FIFO de pedidos
 * Parametros: head - puntero al primer pedido de la cola
 * 
 * Esta función es crítica para evitar fugas de memoria. Debe llamarse
 * antes de eliminar un nodo del arbol AVL.
 */
void free_orders(Order *head) {
    Order *p = head;
    while (p) {
        Order *tmp = p;        // Guardar referencia al nodo actual
        p = p->siguiente;     // Avanzar al siguiente
        free(tmp);            // Liberar memoria del nodo actual
    }
}

/* ============================================================================
 * OPERACIONES DE INSERCIÓN EN EL ARBOL AVL
 * ============================================================================ */

/**
 * Funcion: searchNode
 * Descripcion: Busca un nodo en el arbol AVL por su fecha de vencimiento
 * Parametros:
 *   - root: Raíz del arbol donde buscar
 *   - fecha: Fecha de vencimiento a buscar (formato AAAAMMDD)
 * Retorna: Puntero al nodo encontrado, o NULL si no existe
 * 
 * Complejidad: O(log n) donde n es el numero de nodos en el arbol
 */
Node* searchNode(Node *root, int fecha) {
    if (!root) return NULL;  // Árbol vacio o llegamos a una hoja
    
    // Comparar fechas
    if (fecha == root->fecha_vencimiento) 
        return root;  // Encontrado
    else if (fecha < root->fecha_vencimiento) 
        return searchNode(root->left, fecha);   // Buscar en subarbol izquierdo
    else 
        return searchNode(root->right, fecha);  // Buscar en subarbol derecho
}

/**
 * Funcion: insertAVL
 * Descripcion: Inserta un nuevo lote en el arbol AVL manteniendo el balanceo
 * Parametros:
 *   - node: Raíz del subarbol donde insertar
 *   - fecha: Fecha de vencimiento (clave única)
 *   - producto: Nombre del producto
 *   - stock: Cantidad inicial de stock
 * Retorna: Nueva raiz del subarbol despues de la inserción y balanceo
 * 
 * Reglas de negocio:
 * - NO permite fechas duplicadas (segun especificación)
 * - Mantiene el arbol balanceado despues de cada inserción
 * - Complejidad: O(log n) para inserción y balanceo
 */
Node* insertAVL(Node *node, int fecha, const char *producto, int stock) {
    // Caso base: crear nuevo nodo si llegamos a una hoja
    if (!node) {
        Node *nuevo = newNode(fecha, producto, stock);
        if (!nuevo) return NULL;  // Error de memoria
        return nuevo;
    }
    
    // Insertar recursivamente segun la fecha
    if (fecha < node->fecha_vencimiento) {
        // Insertar en subarbol izquierdo (fechas mas antiguas)
        node->left = insertAVL(node->left, fecha, producto, stock);
        if (!node->left) return NULL;  // Error en inserción
    }
    else if (fecha > node->fecha_vencimiento) {
        // Insertar en subarbol derecho (fechas mas futuras)
        node->right = insertAVL(node->right, fecha, producto, stock);
        if (!node->right) return NULL;  // Error en inserción
    }
    else {
        // FECHA DUPLICADA: segun especificación, no se procesan duplicados
        printf("ERROR: Ya existe un lote con la fecha %d. No se puede insertar duplicado.\n", fecha);
        return node;  // Retornar sin cambios
    }

    // Actualizar altura del nodo actual
    node->height = 1 + max(height(node->left), height(node->right));
    
    // Obtener factor de balance
    int balance = getBalance(node);

    // CASO LL: Desbalance hacia la izquierda-izquierda
    // Solución: Rotación simple a la derecha
    if (balance > 1 && fecha < node->left->fecha_vencimiento) 
        return rightRotate(node);
    
    // CASO RR: Desbalance hacia la derecha-derecha
    // Solución: Rotación simple a la izquierda
    if (balance < -1 && fecha > node->right->fecha_vencimiento) 
        return leftRotate(node);
    
    // CASO LR: Desbalance hacia la izquierda-derecha
    // Solución: Rotación doble (izquierda en hijo, luego derecha en raiz)
    if (balance > 1 && fecha > node->left->fecha_vencimiento) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    
    // CASO RL: Desbalance hacia la derecha-izquierda
    // Solución: Rotación doble (derecha en hijo, luego izquierda en raiz)
    if (balance < -1 && fecha < node->right->fecha_vencimiento) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    
    // Árbol ya está balanceado
    return node;
}

/* ============================================================================
 * OPERACIONES DE BÚSQUEDA EN EL ARBOL AVL
 * ============================================================================ */

/**
 * Funcion: minValueNode
 * Descripcion: Encuentra el nodo con la fecha de vencimiento mas proxima (mínima)
 * Parametros: node - raiz del subarbol donde buscar
 * Retorna: Puntero al nodo con la fecha mínima, o NULL si el arbol está vacio
 * 
 * Esta función es crítica para la funcionalidad de "Registrar Pedido de Despacho",
 * ya que siempre se debe usar el lote mas proximo a vencer (FIFO por fecha).
 * 
 * Complejidad: O(log n) en el peor caso, O(1) si el minimo está en la raiz
 */
Node* minValueNode(Node *node) {
    Node *current = node;
    if (!current) return NULL;
    
    // En un BST, el minimo siempre está en el extremo izquierdo
    while (current->left) 
        current = current->left;
    
    return current;
}

/* ============================================================================
 * OPERACIONES DE ELIMINACIÓN EN EL ARBOL AVL
 * ============================================================================ */

/**
 * Funcion: deleteNode
 * Descripcion: Elimina un nodo del arbol AVL por su fecha de vencimiento
 * Parametros:
 *   - root: Raíz del subarbol donde eliminar
 *   - fecha: Fecha de vencimiento del nodo a eliminar
 * Retorna: Nueva raiz del subarbol despues de la eliminación y balanceo
 * 
 * IMPORTANTE: Esta función libera TODA la memoria asociada al nodo, incluyendo
 * su cola FIFO de pedidos. Esto es crítico para evitar fugas de memoria.
 * 
 * Complejidad: O(log n) para búsqueda + O(log n) para balanceo = O(log n)
 */
Node* deleteNode(Node* root, int fecha) {
    if (!root) return root;  // Caso base: arbol vacio
    
    // Buscar el nodo a eliminar recursivamente
    if (fecha < root->fecha_vencimiento) 
        root->left = deleteNode(root->left, fecha);
    else if (fecha > root->fecha_vencimiento) 
        root->right = deleteNode(root->right, fecha);
    else {
        // NODO ENCONTRADO: este es el nodo a eliminar
        
        // PASO CRÍTICO: Liberar la cola FIFO antes de eliminar el nodo
        // Esto previene fugas de memoria (requisito de la rúbrica)
        free_orders(root->cabeza_pedidos);
        
        // CASO 1: Nodo sin hijos o con un solo hijo
        if (!root->left || !root->right) {
            Node *temp = root->left ? root->left : root->right;
            
            if (!temp) {
                // Sin hijos: simplemente liberar el nodo
                free(root);
                return NULL;
            } else {
                // Un hijo: copiar todos los campos del hijo al nodo actual
                // Esto preserva la cola FIFO del hijo
                root->fecha_vencimiento = temp->fecha_vencimiento;
                strncpy(root->producto, temp->producto, MAX_NAME-1);
                root->producto[MAX_NAME-1] = '\0';
                root->stock_total = temp->stock_total;
                root->cabeza_pedidos = temp->cabeza_pedidos;  // Preservar cola del hijo
                root->tail = temp->tail;
                root->left = temp->left;
                root->right = temp->right;
                root->height = temp->height;
                
                // IMPORTANTE: Limpiar punteros del hijo antes de liberarlo
                // para evitar que se libere la cola dos veces
                temp->cabeza_pedidos = NULL;
                temp->tail = NULL;
                free(temp);
            }
        } else {
            // CASO 2: Nodo con dos hijos
            // Estrategia: Reemplazar con el sucesor en orden (minimo del subarbol derecho)
            
            Node *temp = minValueNode(root->right);
            
            // Copiar datos del sucesor al nodo actual
            root->fecha_vencimiento = temp->fecha_vencimiento;
            strncpy(root->producto, temp->producto, MAX_NAME-1);
            root->producto[MAX_NAME-1] = '\0';
            root->stock_total = temp->stock_total;
            
            // Clonar la cola FIFO del sucesor (ya liberamos la cola actual arriba)
            free_orders(root->cabeza_pedidos);
            root->cabeza_pedidos = NULL; 
            root->tail = NULL;
            
            // Clonar todos los pedidos del sucesor
            Order *p = temp->cabeza_pedidos;
            while (p) {
                if (!enqueue_order(root, p->nombre_destino, p->cantidad_solicitada)) {
                    fprintf(stderr, "Advertencia: Error al clonar algunos pedidos.\n");
                }
                p = p->siguiente;
            }
            
            // Eliminar el sucesor del subarbol derecho
            root->right = deleteNode(root->right, temp->fecha_vencimiento);
        }
    }
    
    if (!root) return root;  // Si el arbol quedó vacio
    
    // PASO CRÍTICO: Rebalancear el arbol despues de la eliminación
    root->height = 1 + max(height(root->left), height(root->right));
    int balance = getBalance(root);

    // CASO LL: Desbalance hacia la izquierda-izquierda
    if (balance > 1 && getBalance(root->left) >= 0) 
        return rightRotate(root);
    
    // CASO LR: Desbalance hacia la izquierda-derecha
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    
    // CASO RR: Desbalance hacia la derecha-derecha
    if (balance < -1 && getBalance(root->right) <= 0) 
        return leftRotate(root);
    
    // CASO RL: Desbalance hacia la derecha-izquierda
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    
    return root;
}

/* ============================================================================
 * OPERACIONES DE BÚSQUEDA Y CANCELACIÓN DE PEDIDOS
 * ============================================================================ */

/**
 * Funcion: findNode
 * Descripcion: Wrapper para buscar un nodo por fecha (alias de searchNode)
 * Parametros:
 *   - root: Raíz del arbol donde buscar
 *   - fecha: Fecha de vencimiento a buscar
 * Retorna: Puntero al nodo encontrado, o NULL si no existe
 */
Node* findNode(Node *root, int fecha) {
    return searchNode(root, fecha);
}

/**
 * Funcion: cancel_order_in_node
 * Descripcion: Cancela un pedido específico de la cola FIFO de un nodo
 * Parametros:
 *   - node: Nodo del arbol que contiene la cola donde buscar
 *   - destino: Nombre del destino del pedido a cancelar
 *   - cantidad: Cantidad exacta del pedido a cancelar
 * Retorna: 1 si se encontró y eliminó el pedido, 0 si no se encontró
 * 
 * Funcionalidad:
 * - Busca el pedido por destino Y cantidad (ambos deben coincidir)
 * - Elimina el pedido de la cola FIFO
 * - Restaura el stock del lote (suma la cantidad cancelada)
 * - Libera la memoria del pedido eliminado
 * 
 * Complejidad: O(n) donde n es el numero de pedidos en la cola
 */
int cancel_order_in_node(Node *node, const char *destino, int cantidad) {
    if (!node || !node->cabeza_pedidos) return 0;
    
    Order *prev = NULL;           // Puntero al pedido anterior
    Order *cur = node->cabeza_pedidos;  // Puntero al pedido actual
    
    // Buscar el pedido en la cola
    while (cur) {
        // Verificar si este es el pedido a cancelar (debe coincidir destino Y cantidad)
        if (strcmp(cur->nombre_destino, destino) == 0 && cur->cantidad_solicitada == cantidad) {
            // PEDIDO ENCONTRADO: eliminarlo de la cola
            
            if (!prev) {
                // Es el primer pedido: actualizar cabeza
                node->cabeza_pedidos = cur->siguiente;
                if (node->tail == cur) node->tail = NULL;  // Si era el único, cola queda vacia
            } else {
                // No es el primer pedido: enlazar anterior con siguiente
                prev->siguiente = cur->siguiente;
                if (node->tail == cur) node->tail = prev;  // Si era el último, actualizar tail
            }
            
            // Restaurar el stock del lote (sumar la cantidad cancelada)
            node->stock_total += cantidad;
            
            // Liberar memoria del pedido eliminado
            free(cur);
            return 1;  // Éxito
        }
        
        // Avanzar al siguiente pedido
        prev = cur;
        cur = cur->siguiente;
    }
    
    return 0;  // Pedido no encontrado
}

/* ============================================================================
 * REPORTES Y LIBERACIÓN DE MEMORIA
 * ============================================================================ */

/**
 * Funcion: mostrar_pedidos
 * Descripcion: Muestra todos los pedidos de un lote de forma organizada en tabla
 * Parametros: node - nodo del arbol con los pedidos a mostrar
 * 
 * Esta funcion mejora la visualizacion de los pedidos mostrandolos en una
 * tabla con bordes que incluye numero de pedido, destino y cantidad.
 * Facilita la lectura y seleccion de pedidos a cancelar.
 */
void mostrar_pedidos(Node *node) {
    if (!node || !node->cabeza_pedidos) {
        printf("  No hay pedidos pendientes.\n");
        return;
    }
    
    Order *p = node->cabeza_pedidos;
    int num = 1;
    printf("  Pedidos pendientes:\n");
    printf("  +-----+----------------------+--------------+\n");
    printf("  | No. | Destino              | Cantidad     |\n");
    printf("  +-----+----------------------+--------------+\n");
    
    while (p) {
        printf("  | %-3d | %-20s | %12d |\n", num++, p->nombre_destino, p->cantidad_solicitada);
        p = p->siguiente;
    }
    
    printf("  +-----+----------------------+--------------+\n");
}

/**
 * Funcion: inorder_report
 * Descripcion: Genera un reporte del inventario usando recorrido In-Order
 * Parametros: root - raiz del arbol AVL
 * 
 * El recorrido In-Order garantiza que los lotes se muestren ordenados
 * desde la fecha mas proxima a vencer hasta la mas lejana.
 * 
 * Complejidad: O(n) donde n es el numero de nodos en el arbol
 */
void inorder_report(Node *root) {
    if (!root) return;
    
    // Recorrer subarbol izquierdo (fechas mas antiguas primero)
    inorder_report(root->left);
    
    // Procesar nodo actual: mostrar informacion del lote

    printf("LOTE: %s\n", root->producto);

    
    printf("Fecha de vencimiento: %s\n", formatear_fecha(root->fecha_vencimiento));
    printf("Stock disponible: %d\n", root->stock_total);
    printf("Pedidos pendientes: %d\n", count_orders(root));
    mostrar_pedidos(root);
    
    // Recorrer subarbol derecho (fechas mas futuras despues)
    inorder_report(root->right);
}

/**
 * Funcion: free_tree
 * Descripcion: Libera toda la memoria del arbol AVL y sus colas FIFO
 * Parametros: root - raiz del arbol a liberar
 * 
 * Esta función es CRÍTICA para evitar fugas de memoria. Debe llamarse
 * antes de terminar el programa. Libera en orden post-order para evitar
 * acceder a memoria ya liberada.
 * 
 * Orden de liberación:
 * 1. Liberar subarbol izquierdo
 * 2. Liberar subarbol derecho
 * 3. Liberar cola FIFO del nodo
 * 4. Liberar el nodo mismo
 */
void free_tree(Node *root) {
    if (!root) return;
    
    // Liberar recursivamente los subarboles
    free_tree(root->left);
    free_tree(root->right);
    
    // Liberar la cola FIFO del nodo (CRÍTICO para evitar fugas de memoria)
    free_orders(root->cabeza_pedidos);
    
    // Finalmente, liberar el nodo
    free(root);
}

/* ============================================================================
 * UTILIDADES DE ENTRADA/SALIDA
 * ============================================================================ */

/**
 * Funcion: read_line
 * Descripcion: Lee una línea de texto desde stdin y elimina el salto de línea
 * Parametros:
 *   - buffer: Buffer donde almacenar la línea leída
 *   - size: Tamanio maximo del buffer
 * 
 * Esta función es útil para leer nombres de productos y destinos que pueden
 * contener espacios.
 */
void read_line(char *buffer, int size) {
    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';  // En caso de error, dejar buffer vacio
        return;
    }
    size_t ln = strlen(buffer);
    if (ln > 0 && buffer[ln - 1] == '\n') {
        buffer[ln - 1] = '\0';  // Eliminar el salto de línea
    }
}

/**
 * Funcion: limpiar_buffer
 * Descripcion: Limpia el buffer de entrada estándar hasta encontrar un salto de línea
 * 
 * Esta función es necesaria despues de usar scanf() para evitar que caracteres
 * residuales interfieran con lecturas posteriores.
 */
void limpiar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ============================================================================
 * PERSISTENCIA EN ARCHIVO
 * ============================================================================ */

/**
 * Funcion: guardar_arbol
 * Descripcion: Guarda el arbol AVL completo en un archivo binario
 * Parametros: root - raiz del arbol a guardar, filename - nombre del archivo
 * Retorna: true si se guardo correctamente, false en caso contrario
 * 
 * Esta funcion implementa la persistencia de datos guardando todo el arbol AVL
 * y sus colas FIFO en un archivo binario. Usa recorrido pre-order para guardar
 * los nodos y marca con -1 los nodos NULL.
 * 
 * Formato del archivo:
 * - Para cada nodo: fecha (int), producto (char[MAX_NAME]), stock (int)
 * - Numero de pedidos (int), seguido de cada pedido (destino + cantidad)
 * - Marcador -1 para nodos NULL
 */
bool guardar_arbol(Node *root, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return false;
    
    // Funcion auxiliar recursiva para guardar en pre-order
    void guardar_nodo(Node *n) {
        if (!n) {
            int marcador = -1;  // Marcador de fin de nodo
            fwrite(&marcador, sizeof(int), 1, f);
            return;
        }
        
        // Guardar datos del nodo
        fwrite(&n->fecha_vencimiento, sizeof(int), 1, f);
        fwrite(n->producto, sizeof(char), MAX_NAME, f);
        fwrite(&n->stock_total, sizeof(int), 1, f);
        
        // Guardar cola FIFO: primero el numero de pedidos
        int num_pedidos = count_orders(n);
        fwrite(&num_pedidos, sizeof(int), 1, f);
        
        // Luego cada pedido (destino + cantidad)
        Order *p = n->cabeza_pedidos;
        while (p) {
            fwrite(p->nombre_destino, sizeof(char), MAX_DEST, f);
            fwrite(&p->cantidad_solicitada, sizeof(int), 1, f);
            p = p->siguiente;
        }
        
        // Guardar subarboles recursivamente (pre-order)
        guardar_nodo(n->left);
        guardar_nodo(n->right);
    }
    
    guardar_nodo(root);
    fclose(f);
    return true;
}

/**
 * Funcion: cargar_arbol
 * Descripcion: Carga el arbol AVL desde un archivo binario
 * Parametros: filename - nombre del archivo
 * Retorna: Raiz del arbol cargado, o NULL si hay error
 * 
 * Esta funcion implementa la carga de datos desde archivo binario, restaurando
 * todo el arbol AVL y sus colas FIFO. Lee en el mismo orden que se guardo
 * (pre-order) y reconstruye la estructura completa.
 * 
 * Importante: Ajusta el stock del nodo restando las cantidades de los pedidos
 * cargados, ya que el stock guardado es el stock disponible final.
 */
Node* cargar_arbol(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    Node* cargar_nodo(FILE *file) {
        int fecha;
        if (fread(&fecha, sizeof(int), 1, file) != 1) {
            return NULL;
        }
        
        if (fecha == -1) return NULL;  // Marcador de fin de nodo
        
        char producto[MAX_NAME];
        int stock;
        
        // Leer datos del nodo
        if (fread(producto, sizeof(char), MAX_NAME, file) != MAX_NAME ||
            fread(&stock, sizeof(int), 1, file) != 1) {
            return NULL;
        }
        
        // Crear nodo con los datos leidos
        Node *n = newNode(fecha, producto, stock);
        if (!n) {
            return NULL;
        }
        
        // Cargar numero de pedidos
        int num_pedidos;
        if (fread(&num_pedidos, sizeof(int), 1, file) != 1) {
            free(n);
            return NULL;
        }
        
        // Cargar cada pedido y reconstruir la cola FIFO
        for (int i = 0; i < num_pedidos; i++) {
            char destino[MAX_DEST];
            int cantidad;
            if (fread(destino, sizeof(char), MAX_DEST, file) != MAX_DEST ||
                fread(&cantidad, sizeof(int), 1, file) != 1) {
                free_tree(n);
                return NULL;
            }
            enqueue_order(n, destino, cantidad);
            n->stock_total -= cantidad;  // Ajustar stock (restar pedidos pendientes)
        }
        
        // Cargar subarboles recursivamente (pre-order)
        n->left = cargar_nodo(file);
        n->right = cargar_nodo(file);
        
        // Actualizar altura del nodo
        n->height = 1 + max(height(n->left), height(n->right));
        
        return n;
    }
    
    Node *root = cargar_nodo(f);
    fclose(f);
    return root;
}

/* ============================================================================
 * INGRESO MÚLTIPLE DE PRODUCTOS
 * ============================================================================ */

/**
 * Funcion: ingresar_productos_multiples
 * Descripcion: Permite ingresar varios productos en una sola operacion
 * Parametros: root - raiz del arbol (se actualiza)
 * Retorna: Nueva raiz del arbol despues de las inserciones
 * 
 * Esta funcion facilita el ingreso masivo de productos mostrando el progreso
 * y permitiendo ingresar varios lotes de forma consecutiva. Cada producto
 * se valida individualmente y se muestra un mensaje de exito o error.
 * 
 * Caracteristicas:
 * - Muestra progreso (Producto X de Y)
 * - Valida cada entrada individualmente
 * - Omite productos con fechas duplicadas
 * - Permite continuar aunque haya errores en productos individuales
 */
Node* ingresar_productos_multiples(Node *root) {
    int cantidad;
    printf("Cuantos productos desea ingresar? ");
    if (scanf("%d", &cantidad) != 1 || cantidad <= 0) {
        printf("Error: Cantidad invalida.\n");
        limpiar_buffer();
        return root;
    }
    limpiar_buffer();
    
    printf("\n=== INGRESO DE %d PRODUCTOS ===\n\n", cantidad);
    
    for (int i = 0; i < cantidad; i++) {
        printf("--- Producto %d de %d ---\n", i + 1, cantidad);
        
        // Solicitar fecha en formato amigable DD MM año
        int dia, mes, anio;
        printf("Fecha de vencimiento (DD MM año): ");
        if (scanf("%d %d %d", &dia, &mes, &anio) != 3) {
            printf("Error: Formato invalido. Use: DD MM año (ej: 04 12 2025)\n");
            limpiar_buffer();
            continue;
        }
        limpiar_buffer();
        
        // Convertir y validar fecha
        int fecha = convertir_fecha_a_int(dia, mes, anio);
        if (fecha == -1 || !validar_fecha(fecha)) {
            printf("Error: Fecha invalida.\n");
            continue;
        }
        
        // Solicitar nombre del producto
        char producto[MAX_NAME];
        printf("Nombre del producto: ");
        read_line(producto, MAX_NAME);
        if (strlen(producto) == 0) {
            printf("Error: El nombre no puede estar vacio.\n");
            continue;
        }
        
        // Solicitar cantidad (stock)
        int stock;
        printf("Cantidad (stock): ");
        if (scanf("%d", &stock) != 1 || stock <= 0) {
            printf("Error: Cantidad invalida.\n");
            limpiar_buffer();
            continue;
        }
        limpiar_buffer();
        
        // Verificar duplicados e insertar
        if (searchNode(root, fecha)) {
            printf("Ya existe un lote con fecha %s. Se omite.\n", formatear_fecha(fecha));
        } else {
            Node *nuevo_root = insertAVL(root, fecha, producto, stock);
            if (nuevo_root) {
                root = nuevo_root;
                printf("Producto '%s' insertado correctamente.\n", producto);
            } else {
                printf("Error al insertar producto.\n");
            }
        }
        printf("\n");
    }
    
    printf("=== Ingreso completado ===\n");
    return root;
}

/* ============================================================================
 * FUNCIÓN PRINCIPAL - MENÚ INTERACTIVO
 * ============================================================================ */

/**
 * Funcion: main
 * Descripcion: Funcion principal que implementa el menú interactivo del sistema
 * 
 * El programa gestiona un inventario de productos perecederos usando:
 * - Árbol AVL balanceado por fecha de vencimiento
 * - Colas FIFO para pedidos de despacho dentro de cada lote
 * 
 * Opciones del menú:
 * 1. Recepción de mercancía: Insertar nuevo lote en el arbol AVL
 * 2. Registrar pedido de despacho: Agregar pedido al lote mas proximo a vencer
 * 3. Cancelar producto: Eliminar un lote completo del inventario
 * 4. Cancelar pedido: Eliminar un pedido específico de una cola
 * 5. Reporte de estado: Mostrar inventario ordenado por fecha (In-Order)
 * 6. Salir: Liberar memoria y terminar programa
 */
int main() {
    Node *root = NULL;  // Raíz del arbol AVL (inicialmente vacio)
    int opc = 0;
    
    // Intentar cargar inventario al iniciar
    printf("¿Desea cargar inventario guardado? (s/n): ");
    char respuesta;
    scanf(" %c", &respuesta);
    limpiar_buffer();
    
    if (respuesta == 's' || respuesta == 'S') {
        root = cargar_arbol(ARCHIVO_DATOS);
        if (root) {
            printf("✓ Inventario cargado correctamente.\n");
        } else {
            printf("ℹ No se encontró inventario guardado o el archivo esta vacio.\n");
        }
    }
    
    // Bucle principal del menú
    while (1) {
        // Mostrar menú de opciones
        
        printf("         SISTEMA LOGISTICO - BUENAVENTURA                \n");
      
        printf(" 1. Recepción de mercancia (un producto)                 \n");
        printf(" 2. Recepción multiple de mercancia                      \n");
        printf("  3. Registrar pedido de despacho                        \n");
        printf("  4. Cancelar (Baja) de producto                         \n");
        printf("  5. Cancelar pedido específico                          \n");
        printf(" 6. Reporte de estado                                    \n");
        printf("  7. Guardar inventario                                  \n");
        printf("  8. Cargar inventario                                   \n");
        printf("  9. Salir                                               \n");

        printf("Seleccione opcion: ");
        
        // Leer opcion del usuario
        if (scanf("%d", &opc) != 1) {
            // Error en lectura: limpiar buffer y continuar
            printf("Error: Opcion invalida. Por favor ingrese un numero.\n");
            limpiar_buffer();
            continue;
        }
        limpiar_buffer();  // Limpiar buffer despues de scanf

        // OPCION 1: Recepción de mercancía (un producto)
        if (opc == 1) {
            int dia, mes, anio;
            char producto[MAX_NAME];
            int cantidad;
            
            printf("\n=== RECEPCION DE MERCANCIA ===\n");
            
            // Solicitar fecha en formato amigable
            printf("Fecha de vencimiento (DD MM YYYY, ej: 04 12 2025): ");
            if (scanf("%d %d %d", &dia, &mes, &anio) != 3) {
                printf("Error: Formato invalido. Use: DD MM YYYY\n");
                limpiar_buffer();
                continue;
            }
            limpiar_buffer();
            
            int fecha = convertir_fecha_a_int(dia, mes, anio);
            if (fecha == -1 || !validar_fecha(fecha)) {
                printf("Error: Fecha invalida. Verifique el formato.\n");
                continue;
            }
            
            // Solicitar nombre del producto
            printf("Nombre del producto: ");
            read_line(producto, MAX_NAME);
            if (strlen(producto) == 0) {
                printf("Error: El nombre del producto no puede estar vacio.\n");
                continue;
            }
            
            // Solicitar cantidad (stock) del lote
            printf("Cantidad (stock) del lote: ");
            if (scanf("%d", &cantidad) != 1 || cantidad <= 0) {
                printf("Error: Cantidad debe ser un numero positivo.\n");
                limpiar_buffer();
                continue;
            }
            limpiar_buffer();

            // Verificar si ya existe un lote con esa fecha
            if (searchNode(root, fecha)) {
                printf("⚠ Ya existe un lote con fecha %s. No se procesa.\n", formatear_fecha(fecha));
            } else {
                // Insertar nuevo lote en el arbol AVL
                Node *nuevo_root = insertAVL(root, fecha, producto, cantidad);
                if (nuevo_root) {
                    root = nuevo_root;
                    printf("✓ Lote insertado correctamente.\n");
                } else {
                    printf("✗ Error: No se pudo insertar el lote (error de memoria).\n");
                }
            }
        }
        // OPCION 2: Recepción múltiple de mercancía
        else if (opc == 2) {
            root = ingresar_productos_multiples(root);
        }
        // OPCION 3: Registrar pedido de despacho (Encolar en FIFO del lote mas cercano a vencer)
        else if (opc == 3) {
            // Verificar que haya lotes en inventario
            if (!root) {
                printf("No hay lotes en inventario.\n");
                continue;
            }
            
            // Buscar el lote con fecha mas proxima a vencer (minimo del arbol)
            // Esto garantiza que siempre se use el lote mas antiguo primero (FIFO por fecha)
            Node *lote = minValueNode(root);
            if (!lote) {
                printf("Error: No se pudo encontrar el lote mas proximo a vencer.\n");
                continue;
            }
            
            // Mostrar información del lote seleccionado
            printf("\n=== REGISTRAR PEDIDO DE DESPACHO ===\n");
            printf("Lote seleccionado (fecha mas proxima a vencer):\n");
            printf("  Producto: %s\n", lote->producto);
            printf("  Fecha de vencimiento: %s\n", formatear_fecha(lote->fecha_vencimiento));
            printf("  Stock disponible: %d\n", lote->stock_total);
            
            char destino[MAX_DEST];
            int qty;
            
            // Solicitar destino del pedido
            printf("Ingresar destino del pedido: ");
            read_line(destino, MAX_DEST);
            if (strlen(destino) == 0) {
                printf("Error: El destino no puede estar vacio.\n");
                continue;
            }
            
            // Solicitar cantidad solicitada
            printf("Ingresar cantidad solicitada: ");
            if (scanf("%d", &qty) != 1) {
                printf("Error: Cantidad invalida.\n");
                limpiar_buffer();
                continue;
            }
            limpiar_buffer();
            
            // Validar que la cantidad sea positiva
            if (qty <= 0) {
                printf("Error: La cantidad debe ser positiva.\n");
                continue;
            }
            
            // Verificar disponibilidad de stock
            if (qty > lote->stock_total) {
                printf("Error: Stock insuficiente (stock=%d). No se puede registrar pedido.\n", lote->stock_total);
            } else {
                // Agregar pedido a la cola FIFO del lote
                if (enqueue_order(lote, destino, qty)) {
                    // Descontar stock del lote
                    lote->stock_total -= qty;
                    printf("✓ Pedido encolado correctamente.\n");
                    printf("  Nuevo stock: %d\n", lote->stock_total);
                } else {
                    printf("✗ Error: No se pudo registrar el pedido (error de memoria).\n");
                }
            }
        }
        // OPCION 4: Cancelar (Baja) de producto (Eliminar nodo completo)
        else if (opc == 4) {
            int dia, mes, anio;
            
            printf("\n=== CANCELAR PRODUCTO ===\n");
            
            // Solicitar fecha del lote a eliminar
            printf("Fecha del lote a eliminar (DD MM YYYY): ");
            if (scanf("%d %d %d", &dia, &mes, &anio) != 3) {
                printf("Error: Formato invalido. Use: DD MM YYYY\n");
                limpiar_buffer();
                continue;
            }
            limpiar_buffer();
            
            int fecha = convertir_fecha_a_int(dia, mes, anio);
            if (fecha == -1 || !validar_fecha(fecha)) {
                printf("Error: Fecha invalida.\n");
                continue;
            }
            
            // Verificar que el lote existe
            Node *lote = searchNode(root, fecha);
            if (!lote) {
                printf("✗ No existe lote con fecha %s.\n", formatear_fecha(fecha));
            } else {
                printf("Lote encontrado: %s - %s\n", formatear_fecha(fecha), lote->producto);
                printf("¿Está seguro de eliminar este lote? (s/n): ");
                char confirmar;
                scanf(" %c", &confirmar);
                limpiar_buffer();
                
                if (confirmar == 's' || confirmar == 'S') {
                    // Eliminar el nodo completo (incluye liberar su cola FIFO)
                    root = deleteNode(root, fecha);
                    printf("✓ Lote eliminado correctamente (memoria liberada).\n");
                } else {
                    printf("Operación cancelada.\n");
                }
            }
        }
        // OPCION 5: Cancelar pedido específico en cola (buscar y eliminar)
        else if (opc == 5) {
            int dia, mes, anio;
            
            printf("\n=== CANCELAR PEDIDO ===\n");
            
            // Solicitar fecha del lote donde buscar el pedido
            printf("Fecha del lote donde buscar pedido (DD MM YYYY): ");
            if (scanf("%d %d %d", &dia, &mes, &anio) != 3) {
                printf("Error: Formato invalido. Use: DD MM YYYY\n");
                limpiar_buffer();
                continue;
            }
            limpiar_buffer();
            
            int fecha = convertir_fecha_a_int(dia, mes, anio);
            if (fecha == -1 || !validar_fecha(fecha)) {
                printf("Error: Fecha invalida.\n");
                continue;
            }
            
            // Buscar el lote en el arbol
            Node *n = findNode(root, fecha);
            if (!n) {
                printf("No existe lote con fecha %d.\n", fecha);
                continue;
            }
            
            // Verificar que haya pedidos en la cola
            if (!n->cabeza_pedidos) {
                printf("La cola de pedidos está vacia en ese lote.\n");
                continue;
            }
            
            // Mostrar pedidos disponibles
            printf("\nPedidos disponibles en este lote:\n");
            mostrar_pedidos(n);
            
            char destino[MAX_DEST];
            int cantidad;
            
            // Solicitar destino del pedido a cancelar
            printf("Ingrese destino del pedido a cancelar: ");
            read_line(destino, MAX_DEST);
            if (strlen(destino) == 0) {
                printf("Error: El destino no puede estar vacio.\n");
                continue;
            }
            
            // Solicitar cantidad exacta del pedido a cancelar
            printf("Ingrese cantidad exacta del pedido a cancelar: ");
            if (scanf("%d", &cantidad) != 1 || cantidad <= 0) {
                printf("Error: Cantidad invalida. Debe ser un numero positivo.\n");
                limpiar_buffer();
                continue;
            }
            limpiar_buffer();
            
            // Intentar cancelar el pedido (busca por destino Y cantidad)
            int ok = cancel_order_in_node(n, destino, cantidad);
            if (ok) {
                printf("✓ Pedido eliminado correctamente. Stock restaurado.\n");
            } else {
                printf("✗ No se encontro un pedido con ese destino y cantidad en la cola.\n");
            }
        }
        // OPCION 6: Reporte de estado (In-Order)
        else if (opc == 6) {
            if (!root) {
                printf("\nNo hay lotes en inventario.\n");
            } else {
                // Generar reporte ordenado por fecha (mas proxima a vencer primero)
        
                printf("                   REPORTE DE INVENTARIO                      \n");
                printf("  Ordenado por fecha: mas proxima a vencer → mas lejana       \n");
                inorder_report(root);
                printf("\n");
            }
        }
        // OPCION 7: Guardar inventario
        else if (opc == 7) {
            if (guardar_arbol(root, ARCHIVO_DATOS)) {
                printf("✓ Inventario guardado correctamente en '%s'.\n", ARCHIVO_DATOS);
            } else {
                printf("✗ Error al guardar el inventario.\n");
            }
        }
        // OPCION 8: Cargar inventario
        else if (opc == 8) {
            if (root) {
                printf("⚠ Ya hay datos en memoria. ¿Desea sobrescribir? (s/n): ");
                char respuesta;
                scanf(" %c", &respuesta);
                limpiar_buffer();
                if (respuesta != 's' && respuesta != 'S') {
                    printf("Operación cancelada.\n");
                    continue;
                }
                free_tree(root);
            }
            
            root = cargar_arbol(ARCHIVO_DATOS);
            if (root) {
                printf("✓ Inventario cargado correctamente desde '%s'.\n", ARCHIVO_DATOS);
            } else {
                printf("✗ Error al cargar el inventario o el archivo no existe.\n");
            }
        }
        // OPCION 9: Salir del programa
        else if (opc == 9) {
            printf("\n¿Desea guardar el inventario antes de salir? (s/n): ");
            char respuesta;
            scanf(" %c", &respuesta);
            limpiar_buffer();
            
            if (respuesta == 's' || respuesta == 'S') {
                if (guardar_arbol(root, ARCHIVO_DATOS)) {
                    printf("✓ Inventario guardado.\n");
                } else {
                    printf("✗ Error al guardar.\n");
                }
            }
            
            printf("Saliendo... liberando memoria.\n");
            // CRÍTICO: Liberar toda la memoria antes de terminar
            free_tree(root);
            break;
        }
        // Opción invalida
        else {
            printf("Opcion no valida.\n");
        }
    }
    
    return 0;
}
