// TODO: Formarse en la cola
// TODO: Sacar a un cliente de la cola
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

// Constantes
#define TAMANO_COLA 4 

// ID de la cola
#define COLA 0

// Etiquetas
#define TURNO 111 // Peticion del cliente por un lugar en la cola 
#define ATIENDE 116 // Peticion del ejecutivo por un cliente 
#define FORMADO 113 // Confirmacion al cliente de que se ha formado
#define CLIENTE 115 // Confirmacion al ejecutivo de que se le ha asignado un cliente 
#define REPITE 114 // El cliente/ejecutivo debe volver a formarse
#define VENTANILLA 117 // Cuando el ejecutivo envia su numero de  ventanilla a un cliente
#define FIN_VENTANILLA 118 // Si la ventanilla ha terminado de atendier a un cliente
#define FIN_COLA 119 // Si la ventanilla ha salido del ciclo
#define FIN 666 // Si el ejecutivo debe detenerse 

void cola(int id, int size){
    int posterior, frente;
    int contador_clientes_atendidos;
    int lugares_libres; // Lugares libres en la cola
    int lugares_ocupados;
    int peticion; // Peticion de los clientes o los servidores
    int formados[TAMANO_COLA]; // Arreglo con los id's de los clientes formados
    int source; // Auxiliar para guardar el id del emisor del mensaje
    int flag[3] = {0,0,0};
    
    MPI_Status reporte;

    lugares_libres = TAMANO_COLA; 
    lugares_ocupados = 0;
    frente = 0; posterior = 0;

    contador_clientes_atendidos = 0;
    printf("TOTAL de CLIENTES: %d\n", size - 4);
    while (flag[0]*flag[1]*flag[2] == 0){
        MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &reporte);
        source = reporte.MPI_SOURCE;

        switch (reporte.MPI_TAG){
            case TURNO:
               // Si hay espacio se forma en la cola, si no se envia un mensaje indicando al cliente que vuelva a intentarlo
               if (lugares_libres > 0){
                    MPI_Send(&posterior, 1, MPI_INT, source, FORMADO, MPI_COMM_WORLD);
                    formados[posterior] = source;
                    posterior = (posterior + 1)%TAMANO_COLA;
                    lugares_libres--;
                    lugares_ocupados++;
                } else {
                    MPI_Send(&posterior, 1, MPI_INT, source, REPITE, MPI_COMM_WORLD);
                }
                break;
            case ATIENDE:
                // Si hay alguien en la cola y aun no se han atendido a todos, algun ejecutivo los puede atender; 
                // si no hay nadie en la cola y aun no se ha atendido a todos, se le pide al ejecutivo que lo vuelva a intentar
                // si no hay nadie en la cola y se han atendido a todos se le pide al ejecutivo que se detenga
                if (lugares_ocupados > 0 && contador_clientes_atendidos < size - 4){
                    MPI_Send(&formados[frente], 1, MPI_INT, source, CLIENTE, MPI_COMM_WORLD);
                    frente = (frente + 1)%TAMANO_COLA;
                    lugares_libres++;
                    lugares_ocupados--;
                } else if (lugares_ocupados <= 0 && contador_clientes_atendidos < size - 4){
                    MPI_Send(&frente, 1, MPI_INT, source, REPITE, MPI_COMM_WORLD);
                } else if (lugares_ocupados <= 0 && contador_clientes_atendidos >= size - 4){
                    MPI_Send(&frente, 1, MPI_INT, source, FIN, MPI_COMM_WORLD);
                } 
                break;
            case FIN_VENTANILLA:
                // Si el ejecutivo ha terminado de atender al cliente 
                contador_clientes_atendidos++;
                break;
            case FIN_COLA:
                flag[source - 1] = 1;
                break;
        } 
    }
}
    

void ejecutivo(int id, int size){
    int flag;
    int id_cliente;
    int recibido;
    int clientes_atendidos;
    MPI_Status reporte, reporte_flag;
    
    flag = 0;
    clientes_atendidos = 0;
    // Hace falta un while aqui para que vuelta a solicitar otro cliente mientras no se hayan atendido al total de clientes
    // Envia un primer intento a la cola para atender; si no se le asigna un cliente, vuelve a intentarlo
    while(1){
        MPI_Send(&id, 1, MPI_INT, COLA, ATIENDE, MPI_COMM_WORLD);
        while(1){
            MPI_Recv(&id_cliente, 1, MPI_INT, COLA, MPI_ANY_TAG, MPI_COMM_WORLD, &reporte);
            if (reporte.MPI_TAG == CLIENTE){
                clientes_atendidos++;
                break;
            } else if (reporte.MPI_TAG == FIN){
                flag = 1;
                break;
            } else{
                MPI_Send(&id, 1, MPI_INT, COLA, ATIENDE, MPI_COMM_WORLD);
            }
        }
        if (flag != 0)
            break;
        // En esta parte se comunica el ejecutivo con el cliente asignado
        MPI_Send(&id, 1, MPI_INT, id_cliente, VENTANILLA, MPI_COMM_WORLD);
        printf("Soy EJECUTIVO: %d atiendo al CLIENTE: %d\n", id, id_cliente);    
        // Ahora el ejecutivo notifica a la cola que ha terminado de atender al cliente
        MPI_Send(&id, 1, MPI_INT, COLA, FIN_VENTANILLA, MPI_COMM_WORLD);
    }
    // printf("Termine %d\n", id);
    MPI_Send(&id, 1, MPI_INT, COLA, FIN_COLA, MPI_COMM_WORLD);
    sleep(1);
    printf("Soy EJECUTIVO %d, atendi %d CLIENTES\n", id, clientes_atendidos);
}

void cliente(int id, int size){
    int id_ventanilla;
    int flag;
    MPI_Status reporte;
    
    flag = 0;

    MPI_Send(&id, 1, MPI_INT, COLA, TURNO, MPI_COMM_WORLD);

    while(flag == 0){    
        MPI_Recv(&flag, 1, MPI_INT, COLA, MPI_ANY_TAG, MPI_COMM_WORLD, &reporte);
        if (reporte.MPI_TAG == FORMADO)
            flag = 1;
        else
            MPI_Send(&id, 1, MPI_INT, COLA, TURNO, MPI_COMM_WORLD);
    }
    // En esta parte se comunica el cliente con la ventanilla asignada
    MPI_Recv(&id_ventanilla, 1, MPI_INT, MPI_ANY_SOURCE, VENTANILLA, MPI_COMM_WORLD, &reporte);
    printf("Soy CLIENTE: %d, me atiende EJECUTIVO: %d\n", id, id_ventanilla);   
}

int main (int argc, char** argv){
    int id, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (id == COLA){
        cola(id, size);
    } else if (id > 0 && id < 4){
        // Los ejecutivos son los procesos 1,2,3
        ejecutivo(id,size);
    } else {
        // Hay (size - 1) - 4 clientes
        cliente(id, size);
    }

    MPI_Finalize();
    return 0;

}