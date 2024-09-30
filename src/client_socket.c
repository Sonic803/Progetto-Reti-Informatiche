#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "client_socket.h"
#include "utils.h"

int sd; // Socket variabile globale

/* Si connette al server e salva il socket descriptor in sd */
void connect_to_server(char *ip, int port)
{
    int ret;
    struct sockaddr_in srv_addr;

    // Creazione socket
    ret = socket(AF_INET, SOCK_STREAM, 0);

    if (ret < 0)
    {
        log_error("Errore in fase di creazione del socket");

        exit(-1);
    }
    sd = ret;

    // Creazione indirizzo del server
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &srv_addr.sin_addr);

    // Connessione 
    ret = connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (ret < 0)
    {
        log_error("Errore in fase di connessione");

        exit(-1);
    }

    // Clear screen
    system("clear");
    log_debug("Connessione effettuata\n");
}

/* Si disconnette dal server */
void close_connection()
{
    close(sd);
}