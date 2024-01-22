// TODO: A veces los tres ejecutivos se ejecutan antes que todos los clientes haciendo que el ultimo no sea atendido. 
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define CLIENTES 3 
#define SALA_ESPERA 2 
#define VENTANILLAS 3

typedef struct clienteReg{
    int id_cliente;
    pthread_mutex_t candado_cliente;
} Cliente;

pthread_t hilos_cliente[CLIENTES];
pthread_t hilos_ventanilla[VENTANILLAS];

pthread_mutex_t toma_turno, atiende_turno, ventanilla, contador, front, pon;
pthread_mutex_t ventana[VENTANILLAS];
pthread_mutex_t termina[VENTANILLAS];

sem_t semaforo_sala;

Cliente cola[SALA_ESPERA];
int frente = 0;
int posterior = 0;

int contador_clientes_atendidos = 0;
int numero_ventanilla;

void cliente(void * ptr);
void ejecutivo(void * ptr);
void inicializa_candados(pthread_mutex_t* candados, int tamano, int i){
    for (int i = 0; i < tamano; i++){    
        pthread_mutex_init(&candados[i], NULL);
        if (i == 0)
            pthread_mutex_unlock(&candados[i]);
        else 
            pthread_mutex_lock(&candados[i]);
    }

}

int main(){
    // semaforo
    sem_init(&semaforo_sala, 0, SALA_ESPERA);

    pthread_mutex_init(&toma_turno, NULL);
    pthread_mutex_init(&atiende_turno, NULL);
    pthread_mutex_init(&ventanilla, NULL);
    pthread_mutex_init(&contador, NULL);
    pthread_mutex_init(&front, NULL);
    pthread_mutex_init(&pon, NULL);
    
    pthread_mutex_unlock(&toma_turno);
    pthread_mutex_unlock(&atiende_turno);
    pthread_mutex_unlock(&ventanilla);
    pthread_mutex_unlock(&contador);
    pthread_mutex_unlock(&pon);
    pthread_mutex_lock(&front);

    inicializa_candados(ventana, VENTANILLAS, 1);
    inicializa_candados(termina, VENTANILLAS, 1);

 for (int i = 0; i < SALA_ESPERA; i++){    
        pthread_mutex_init(&cola[i].candado_cliente, NULL);
        pthread_mutex_lock(&cola[i].candado_cliente);
    }   
    // Creamos los hilos de los clientes
    for (int i = 0; i < CLIENTES; i++) 
        pthread_create(&hilos_cliente[i], NULL, (void*)&cliente, (void*) i);
    
    for (int i = 0; i < VENTANILLAS; i++)
        pthread_create(&hilos_ventanilla[i], NULL, (void*)&ejecutivo,(void *) i);

    for (int j = 0; j < CLIENTES; j++)
        pthread_join(hilos_cliente[j], (void*)NULL);

    for (int j = 0; j < VENTANILLAS; j++)
        pthread_join(hilos_ventanilla[j], (void*)NULL);


    return 0;
}

int ret_stat(){
    int res;
    pthread_mutex_lock(&pon);
    res = contador_clientes_atendidos;
    pthread_mutex_unlock(&pon);
   
   return res;
}

void incr(){
    pthread_mutex_lock(&contador);
        contador_clientes_atendidos++;
    pthread_mutex_unlock(&contador);
}

void cliente(void * ptr){

    int this_id = (int) ptr;
    int this_posterior;
    int this_ventanilla;
    
    sem_wait(&semaforo_sala);
        pthread_mutex_lock(&toma_turno);
            this_posterior = posterior;
            cola[this_posterior].id_cliente = this_id;
            posterior = (posterior + 1)%SALA_ESPERA;
        pthread_mutex_unlock(&toma_turno);
        pthread_mutex_unlock(&front);

        printf("HERE\n");
        // Aqui se bloquea el ultimo cliente
        pthread_mutex_lock(&cola[this_posterior].candado_cliente);
        // printf("POST %d ID %d\n", this_posterior, this_id);
        this_ventanilla = numero_ventanilla;

        pthread_mutex_unlock(&ventana[this_ventanilla]);
        sem_post(&semaforo_sala);
        // printf("CLIENTE: %d VENTANILLA: %d ATEND: %d\n", this_id, this_ventanilla, ret_stat());

        pthread_mutex_lock(&termina[this_ventanilla]);
}


void ejecutivo(void * ptr){
    int this_ventanilla = (int) ptr;
    int this_frente; 
    int this_id;


    while (1){
        pthread_mutex_lock(&front);
        pthread_mutex_lock(&atiende_turno);
            this_frente = frente;
            this_id = cola[this_frente].id_cliente;
            frente = (frente + 1)%SALA_ESPERA;
       pthread_mutex_unlock(&atiende_turno);
        
        pthread_mutex_lock(&ventanilla);
            numero_ventanilla = this_ventanilla;
        pthread_mutex_unlock(&ventanilla);

        pthread_mutex_unlock(&cola[this_frente].candado_cliente);
        pthread_mutex_lock(&ventana[this_ventanilla]);

        printf("VENTANILLA: %d CLIENTE: %d\n", this_ventanilla, this_id);
        // incr();

        pthread_mutex_unlock(&termina[this_ventanilla]);
    }
}

