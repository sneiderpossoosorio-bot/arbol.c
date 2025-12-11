

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
*/
typedef struct Order {
    char nombre_destino[MAX_DEST];   // Nombre del destino segun especificación
    int cantidad_solicitada;          // Cantidad solicitada segun especificación
    struct Order *siguiente;          // Puntero al siguiente pedido segun especificación
} Order;

/**
 * Estructura Node: Representa un nodo del arbol AVL (un lote de productos)
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


int max(int a, int b) { 
    return (a > b) ? a : b; 
}


int height(Node *n) { 
    return n ? n->height : 0; 
}


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


char* formatear_fecha(int fecha) {
    static char buffer[12];
    int anio = fecha / 10000;
    int mes = (fecha / 100) % 100;
    int dia = fecha % 100;
    sprintf(buffer, "%02d/%02d/%04d", dia, mes, anio);
    return buffer;
}


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


int getBalance(Node *n) {
    if (!n) return 0;
    return height(n->left) - height(n->right);
}


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


int count_orders(Node *node) {
    int c = 0;
    Order *p = node ? node->cabeza_pedidos : NULL;
    while (p) { 
        c++; 
        p = p->siguiente; 
    }
    return c;
}


void free_orders(Order *head) {
    Order *p = head;
    while (p) {
        Order *tmp = p;        // Guardar referencia al nodo actual
        p = p->siguiente;     // Avanzar al siguiente
        free(tmp);            // Liberar memoria del nodo actual
    }
}


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


Node* minValueNode(Node *node) {
    Node *current = node;
    if (!current) return NULL;
    
    // En un BST, el minimo siempre está en el extremo izquierdo
    while (current->left) 
        current = current->left;
    
    return current;
}


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


Node* findNode(Node *root, int fecha) {
    return searchNode(root, fecha);
}


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


void limpiar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


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
