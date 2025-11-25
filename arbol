#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Estructura del pasajero (nodo del árbol)
typedef struct Pasajero {
    int documento; 
    char destino[20];
    char tipo_pasaje[20];
    struct Pasajero *izq;
    struct Pasajero *der;
} Pasajero;

// Crear un nuevo nodo
Pasajero* nuevoPasajero(int documento, char destino[], char tipo[]) {
    Pasajero p = (Pasajero)malloc(sizeof(Pasajero));
    p->documento = documento;
    strcpy(p->destino, destino);
    strcpy(p->tipo_pasaje, tipo);
    p->izq = p->der = NULL;
    return p;
}

// Insertar en ABB
Pasajero* insertar(Pasajero *raiz, int documento, char destino[], char tipo[]) {
    if (raiz == NULL) {
        return nuevoPasajero(documento, destino, tipo);
    }
    if (documento < raiz->documento) {
        raiz->izq = insertar(raiz->izq, documento, destino, tipo);
    } else if (documento > raiz->documento) {
        raiz->der = insertar(raiz->der, documento, destino, tipo);
    } else {
        printf("El documento ya existe, no se inserta.\n");
    }
    return raiz;
}

// Recorrido INORDEN
void inorden(Pasajero *r) {
    if (r != NULL) {
        inorden(r->izq);
        printf("Doc: %d | Destino: %s | Tipo: %s\n", r->documento, r->destino, r->tipo_pasaje);
        inorden(r->der);
    }
}

// Recorrido PREORDEN
void preorden(Pasajero *r) {
    if (r != NULL) {
        printf("Doc: %d | Destino: %s | Tipo: %s\n", r->documento, r->destino, r->tipo_pasaje);
        preorden(r->izq);
        preorden(r->der);
    }
}

// Recorrido POSTORDEN
void postorden(Pasajero *r) {
    if (r != NULL) {
        postorden(r->izq);
        postorden(r->der);
        printf("Doc: %d | Destino: %s | Tipo: %s\n", r->documento, r->destino, r->tipo_pasaje);
    }
}

// Contar nodos
int contar(Pasajero *r) {
    if (r == NULL) return 0;
    return 1 + contar(r->izq) + contar(r->der);
}

// Buscar el menor (para eliminación)
Pasajero* minimo(Pasajero* r) {
    while (r && r->izq != NULL) r = r->izq;
    return r;
}

// Eliminar un pasajero
Pasajero* eliminar(Pasajero *r, int documento) {
    if (r == NULL) return r;

    if (documento < r->documento) {
        r->izq = eliminar(r->izq, documento);
    } else if (documento > r->documento) {
        r->der = eliminar(r->der, documento);
    } else {
        if (r->izq == NULL) {
            Pasajero *temp = r->der;
            free(r);
            return temp;
        } else if (r->der == NULL) {
            Pasajero *temp = r->izq;
            free(r);
            return temp;
        }
        Pasajero *temp = minimo(r->der);
        r->documento = temp->documento;
        strcpy(r->destino, temp->destino);
        strcpy(r->tipo_pasaje, temp->tipo_pasaje);
        r->der = eliminar(r->der, temp->documento);
    }
    return r;
}

// Menú
int main() {
    Pasajero *raiz = NULL;
    int op, documento;
    char destino[20], tipo[20];

    do {
        printf("\n--- MENU TIQUETES ---\n");
        printf("1. Registrar pasajero\n");
        printf("2. Mostrar Inorden\n");
        printf("3. Mostrar Preorden\n");
        printf("4. Mostrar Postorden\n");
        printf("5. Contar pasajeros\n");
        printf("6. Eliminar pasajero\n");
        printf("7. Salir\n");
        printf("Opcion: ");
        scanf("%d", &op);

        switch(op) {
            case 1:
                printf("Documento: ");
                scanf("%d", &documento);
                printf("Destino (Timbiqui/Juanchaco/Tumaco/Guapi): ");
                scanf("%s", destino);
                printf("Tipo (Ida / Ida y Regreso): ");
                scanf("%s", tipo);
                raiz = insertar(raiz, documento, destino, tipo);
                break;

            case 2:
                inorden(raiz);
                break;

            case 3:
                preorden(raiz);
                break;

            case 4:
                postorden(raiz);
                break;

            case 5:
                printf("Total pasajeros: %d\n", contar(raiz));
                break;

            case 6:
                printf("Documento a eliminar: ");
                scanf("%d", &documento);
                raiz = eliminar(raiz, documento);
                break;
        }

    } while(op != 7);

    return 0;
}
