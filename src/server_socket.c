#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "server_socket.h"
#include "utils.h"

int sd;

// Crea il socket principale sul quale il server ascolta i nuovi client
void create_main_socket(int port)
{
    int ret, reuse_addr = 1;
    struct sockaddr_in my_addr;

    // Creazione socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    // Metto il socket in modalità 'riuso',altrimenti a volte il server non riparte correttamente dopo averlo chiuso
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));
    if (sd < 0)
    {
        log_error("Errore in fase di creazione del socket");
        pthread_exit(NULL);
    }

    /* Creazione indirizzo di bind */
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    /* Aggancio del socket */
    ret = bind(sd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0)
    {
        log_error("Errore in fase di bind");
        pthread_exit(NULL);
    }
    ret = listen(sd, 8);
    if (ret < 0)
    {
        log_error("Errore in fase di listen");
        pthread_exit(NULL);
    }
}

// Crea il socket figlio sul quale il gioco si svolge
int create_child_socket()
{
    struct sockaddr_in cl_addr;
    unsigned int len = sizeof(cl_addr);
    // Accetto nuove connessioni
    int ret = accept(sd, (struct sockaddr *)&cl_addr, &len);
    if (ret < 0)
    {
        log_error("Errore in fase di accept");
        pthread_exit(NULL);
    }
    return ret;
}

// Chiude il socket del processo
void close_connection()
{
    close(sd);
}