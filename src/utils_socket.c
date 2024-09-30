#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "utils_socket.h"
#include "utils.h"

extern int sd;  // Il socket

// Invia un intero
void send_int(int msg)
{
    int ret;
    uint16_t lmsg;
    lmsg = htons(msg);
    ret = send(sd, (void *)&lmsg, sizeof(uint16_t), 0);
    if (ret != sizeof(uint16_t))
    {
        log_error("Errore in fase di invio di int\n");
        pthread_exit(NULL);
    }
}

// Riceve un intero
void recv_int(int *msg)
{
    int ret;
    uint16_t lmsg;
    ret = recv(sd, (void *)&lmsg, sizeof(uint16_t), 0);
    if (ret != sizeof(uint16_t))
    {
        if (ret == 0)
            log_error("La connessione è stata chiusa\n");
        else
            log_error("Errore in fase di ricezione intero\n");
        pthread_exit(NULL);
    }
    *msg = ntohs(lmsg);
}

// Invia una stringa
void send_string(char *msg)
{
    int ret, len;
    uint16_t lmsg;
    len = strlen(msg) + 1;
    lmsg = htons(len);
    ret = send(sd, (void *)&lmsg, sizeof(uint16_t), 0);
    if (ret != sizeof(uint16_t))
    {
        log_error("Errore in fase di invio stringa\n");
        pthread_exit(NULL);
    }

    ret = send(sd, (void *)msg, len, 0);
    if (ret != len)
    {
        log_error("Errore in fase di invio stringa\n");
        pthread_exit(NULL);
    }
}

// Riceve una stringa
void recv_string(char *msg, int maxlen)
{
    int ret, len;
    uint16_t lmsg;
    ret = recv(sd, (void *)&lmsg, sizeof(uint16_t), 0);
    if (ret != sizeof(uint16_t))
    {
        if (ret == 0)
            log_error("La connessione è stata chiusa\n");
        else
            log_error("Errore in fase di ricezione stringa: Non ho ricevuto la dimensione correttamente\n");
        pthread_exit(NULL);
    }
    len = ntohs(lmsg);
    if (len > maxlen)
    {
        log_error("Errore in fase di ricezione stringa: Buffer richiesto troppo grande \n");
        pthread_exit(NULL);
    }
    ret = recv(sd, (void *)msg, len, 0);
    if (ret != len)
    {
        log_error("Errore in fase di ricezione stringa: Ricevuti meno caratteri del previsto \n");
        pthread_exit(NULL);
    }
    // Controlla che il messaggio ricevuto sia terminato correttamente
    if (msg[len - 1] != '\0')
    {
        log_error("Errore in fase di ricezione stringa: Stringa non terminata correttamente \n");
        pthread_exit(NULL);
    }
}
