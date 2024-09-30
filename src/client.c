#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>

#include "utils_socket.h"
#include "client.h"
#include "client_socket.h"
#include "utils.h"
#include "defines.h"

// Struttura per il comando da eseguire,
// contiene il buffer del comando che una volta letto da terminale viene splittato in al massimo 3 argomenti
struct Command
{
    char command_buffer[BUFFER_SIZE];
    char args[3][BUFFER_SIZE];
    int argc;
};

// Struttura per il gioco
// Contiene lo stato del gioco e l'username
struct Game
{
    int time;
    int tokens;
    enum
    {
        LOGIN,
        LOBBY,
        GAME,
        WRITE_ENIGMA,
        END
    } state;
    char *username;
};

struct Game game = {-1, -1, LOGIN, NULL};   // Stato del gioco
struct Command com; // Comando da eseguire

pthread_mutex_t m;  // Mutex per la sincronizzazione tra i thread
pthread_cond_t RISPOSTA; // Condizione per segnalare al reader thread che è arrivata la risposta dal server

// Riceve la risposta e lo stato del gioco dal server
void recv_response()
{

    // Stringa di risposta
    char response_buffer[BUFFER_SIZE];
    recv_string(response_buffer, BUFFER_SIZE);

    printf("%s\n", response_buffer);

    // Program status
    // 0 = login
    // 1 = lobby
    // 2 = game
    // 3 = write/enigma
    // 4 = end

    int state;
    recv_int(&state);

    switch (state)
    {
    case 0:
        game.state = LOGIN;
        log_debug("Stato: login\n");
        break;
    case 1:
        log_debug("Stato: lobby\n");
        game.state = LOBBY;
        recv_string(response_buffer, BUFFER_SIZE);
        free(game.username);
        game.username = malloc(strlen(response_buffer) + 1);
        strcpy(game.username, response_buffer);
        break;
    case 2:
        log_debug("Stato: game\n");
        game.state = GAME;
        recv_int(&game.time);
        recv_int(&game.tokens);
        break;
    case 3:
        game.state = WRITE_ENIGMA;
        log_debug("Stato: write/enigma\n");
        break;
    case 4:
        game.state = END;
        log_debug("Stato: end\n");
        break;
    default:
        log_error("Stato non riconosciuto\n");
        break;
    }
}

// Invia il comando al server
void send_command()
{
    log_debug("Comando con %d args: ", com.argc);
    send_int(com.argc);
    for (int i = 0; i < com.argc; i++)
    {
        send_string(com.args[i]);
        log_debug("%s ", com.args[i]);
    }
    log_debug("\n");
}

// Legge il comando da terminale
void read_command()
{
    // Se il gioco è nello stato WRITE_ENIGMA, allora leggo tutta la riga
    if (game.state == WRITE_ENIGMA)
    {
        printf("\n");
        fgets(com.args[0], BUFFER_SIZE, stdin);
        // remove newline
        if (com.args[0][strlen(com.args[0]) - 1] == '\n')
            com.args[0][strlen(com.args[0]) - 1] = '\0';
        com.argc = 1;
        return;
    }
    int number_of_args = -1;
    while (number_of_args < 0)
    {
        printf("\n");

        // Stampo il prompt
        if (game.state == LOBBY)                // Se in lobby stampo il nome utente
        {
            printf_green("%s ", game.username);
        }
        else if (game.state == GAME)            // Se in gioco stampo il tempo e i token
        {
            printf_red("time %d", game.time);
            printf(" - ");
            printf_yellow("tok %d ", game.tokens);
        }
        printf("> ");
        fgets(com.command_buffer, BUFFER_SIZE, stdin);

        number_of_args = sscanf(com.command_buffer, "%s %s %s", com.args[0], com.args[1], com.args[2]);
    }
    com.argc = number_of_args;
}

// Stampa l'help iniziale
void get_game_info()
{
    strcpy(com.args[0], "help");
    com.argc = 1;
    send_command();
    recv_response();
}

// Funzione del thread reader
void *reader()
{
    while (game.state != END)
    {
        //Leggo il comando da terminale
        read_command();
        // Invio il comando al server
        send_command();
        // Aspetto che il server mi risponda
        pthread_mutex_lock(&m);
        pthread_cond_wait(&RISPOSTA, &m);
        pthread_mutex_unlock(&m);
    }
    pthread_exit(NULL);
}

// Funzione del thread receiver
void *receiver()
{
    // Thread che riceve le risposte dal server
    while (1)
    {
        // Ricevo la risposta dal server
        recv_response();
        if (game.state == END)
        {
            log_debug("Ricevuto messaggio di fine gioco\n");
            pthread_exit(NULL);
        }
        // Segnalo al reader thread che è arrivata la risposta
        pthread_mutex_lock(&m);
        pthread_cond_signal(&RISPOSTA);
        pthread_mutex_unlock(&m);
    }
}

int main(int argc, char *argv[])
{
    // Init mutex
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&RISPOSTA, NULL);

    game.username = NULL;

    // Controllo se è stato passato il parametro -v
    set_verbosity(argc, argv);

    int porta=4242;
    //porta = get_porta_client(argc, argv);

    // Connessione al server
    connect_to_server("127.0.0.1", porta);

    // Prendo l'help dal server
    get_game_info();

    // Spawn dei thread di lettura da terminale e di ricezione dal server
    pthread_t reader_thread, receiver_thread;
    pthread_create(&reader_thread, NULL, reader, NULL);
    pthread_create(&receiver_thread, NULL, receiver, NULL);

    // Aspetto che il server mi dica che ho finito
    pthread_join(receiver_thread, NULL);

    log_debug("Receiver thread terminato\n");

    // Segnalo al reader thread di terminare
    pthread_cancel(reader_thread);
    pthread_join(reader_thread, NULL);

    free(game.username);

    // Cose per fare in modo che se c'era qualcosa che l'utente stava scrivendo, non venga ricopiato sul terminale
    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
