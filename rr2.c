#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>

struct nodo
{
    int dato;
    struct nodo *anterior;
    struct nodo *siguiente;
};

struct cola
{
    struct nodo *ultimo;
    struct nodo *primero;
};

typedef struct
{
    int pid;
    int termino;
} datos_hilo;

void mover_disco(int n, char origen, char destino, char auxiliar);
struct nodo *crearNodo(int dato1);
void crearCola(int dato, struct cola *miCola);
void meter(int dato1, struct cola *miCola);
int sacar(struct cola *miCola);
void roundRobin(struct cola *miCola);
void *funcionHilo(void *arg);

int main()
{
    struct cola miCola;
    crearCola(-1000, &miCola);

    pid_t pid1, pid2, pid3;

    pid1 = fork(); // Crea el primer hijo

    if (pid1 == 0)
    { // Código para el primer hijo
        kill(getpid(), SIGSTOP);
        while (1)
        {
            printf("\nCorriendo Hanoi\n");
            printf("Numero de discos = 4\n");
            mover_disco(4, 'A', 'C', 'B');
            printf("\nHe terminado\n");
            exit(1);
        }
    }
    else
    {
        pid2 = fork();
        if (pid2 == 0)
        { // Código para el segundo hijo
            kill(getpid(), SIGSTOP);
            while (1)
            {
                printf("\nSoy el segundo hijo\n");
                sleep(1);
            }
        }
        else
        {
            pid3 = fork();
            if (pid3 == 0)
            { // Código para el tercer hijo
                kill(getpid(), SIGSTOP);
                while (1)
                {
                    printf("\nSoy el tercer hijo\n");
                    sleep(1);
                }
            }
            else
            { // Código para el padre
                printf("Soy el proceso padre con PID: %d\n", getpid());
                printf("\nEl pid del primer hijo es: %i", pid1);
                printf("\nEl pid del segundo hijo es: %i", pid2);
                printf("\nEl pid del tercer hijo es: %i", pid3);
                printf("\n");

                meter(pid1, &miCola);
                meter(pid2, &miCola);
                meter(pid3, &miCola);
                roundRobin(&miCola);
            }
        }
    }

    return 0;
}

void mover_disco(int n, char origen, char destino, char auxiliar)
{
    if (n == 1)
    {
        printf("Mover disco 1 de %c a %c\n", origen, destino);
        usleep(500000); // Pausa de medio segundo
        return;
    }
    mover_disco(n - 1, origen, auxiliar, destino);
    printf("Mover disco %d de %c a %c\n", n, origen, destino);
    usleep(500000); // Pausa de medio segundo
    mover_disco(n - 1, auxiliar, destino, origen);
}

struct nodo *crearNodo(int dato1)
{
    struct nodo *nuevo = (struct nodo *)malloc(sizeof(struct nodo));
    nuevo->anterior = NULL;
    nuevo->siguiente = NULL;
    nuevo->dato = dato1;
    return nuevo;
}

void crearCola(int dato, struct cola *miCola)
{
    miCola->ultimo = crearNodo(dato);
    miCola->primero = miCola->ultimo;
    miCola->primero->siguiente = miCola->primero;
    miCola->primero->anterior = miCola->primero;
}

void meter(int dato1, struct cola *miCola)
{
    struct nodo *nuevo = crearNodo(dato1);
    if (miCola->primero == miCola->ultimo)
    { // No hay nodos en la cola
        miCola->primero->siguiente = nuevo;
        nuevo->anterior = miCola->primero;
        nuevo->siguiente = miCola->primero;
        miCola->primero->anterior = nuevo;
        miCola->ultimo = nuevo;
        // printf("El dato ingresado es: %i", miCola->ultimo->dato);
    }
    else
    { // Hay un nodo o mas nodos en la cola
        nuevo->siguiente = miCola->primero;
        miCola->primero->anterior = nuevo;
        nuevo->anterior = miCola->ultimo;
        miCola->ultimo->siguiente = nuevo;
        miCola->ultimo = miCola->ultimo->siguiente;
        // printf("El dato ingresado es: %i", miCola->ultimo->dato);
    }
}

int sacar(struct cola *miCola)
{
    int dato;
    if (miCola->primero == miCola->ultimo)
    { // No hay nodos en la cola
        printf("No hay nodos en la cola para sacar");
        return -10000;
    }
    else if (miCola->primero->siguiente->siguiente == miCola->primero)
    { // Hay un nodo en la cola
        struct nodo *basura = miCola->primero->siguiente;
        miCola->primero->siguiente = miCola->primero;
        miCola->primero->anterior = miCola->primero;
        miCola->ultimo = miCola->primero;
        dato = basura->dato;
        free(basura);
        return dato;
    }
    else
    { // Hay mas de un nodo en la cola
        struct nodo *basura = miCola->primero->siguiente;
        miCola->primero->siguiente = miCola->primero->siguiente->siguiente;
        miCola->primero->siguiente->anterior = miCola->primero;
        dato = basura->dato;
        free(basura);
        return dato;
    }
}

void roundRobin(struct cola *miCola)
{
    datos_hilo datos;
    int i = 0;
    int quantum = 5; // 5 segundos
    int eliminado;   // Guarda el pid del proceso que se esta ejecutando

    while (1)
    {
        datos.termino = 0; // Es igual a 1 si un proceso termina
        eliminado = sacar(miCola);
        if (eliminado == -10000) // Si ya no existen procesos se termina el algoritmo
        {
            break;
        } 
        kill(eliminado, SIGCONT); //Manda una senial para que se ejecute
        datos.pid = eliminado;
        pthread_t miHilo;
        pthread_create(&miHilo, NULL, funcionHilo, (void *)&datos);
        for (i = 0; i < quantum * 4 && datos.termino == 0; i++) // Si el hilo ya termino, el proceso padre ya no esperara al hijo
        {
            usleep(250000); // Espera un cuarto de segundo
        }

        if (datos.termino == 0)
        { // Si el proceso ya termino no regresa a la cola
            kill(eliminado, SIGSTOP); //Manda una senial para que se pause
            meter(eliminado, miCola);
        }
    }
}

void *funcionHilo(void *arg)
{
    datos_hilo *datos = (datos_hilo *)arg;
    int pid = datos->pid;
    while (1)
    {
        int status;
        int result = waitpid(pid, &status, WNOHANG); 

        if (result != 0)
        {
            // El proceso hijo ha terminado
            datos->termino = 1;
            pthread_exit(NULL);
        }
        waitpid(pid, &status, WUNTRACED); // Esperar a que el hijo sea detenido
        if (WIFSTOPPED(status))
        {
            pthread_exit(NULL);
        }
    }

    return NULL;
}