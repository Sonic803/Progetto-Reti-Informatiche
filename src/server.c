#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <wait.h>

#include "server.h"
#include "utils_socket.h"
#include "server_socket.h"
#include "utils.h"
#include "defines.h"
#include "commands.h"
#include "game.h"

extern struct Command com;
extern enum State state;
extern int sd;

int childs = 0;         //Variabile che conta il numero di figli attivi
bool running = false;   //Variabile che indica se il server è attivo
bool isChild = false;   //Variabile che indica se il processo è quello principale o un figlio

// Riceve un comando dal client
void recv_command()
{
    int nargs;
    recv_int(&nargs);
    for (int i = 0; i < nargs; i++)
    {
        recv_string(com.args[i], BUFFER_SIZE);
    }
    com.nargs = nargs;
}

// Funzione del processo figlio che esegue il gioco
void gioco()
{
    while (state != END)
    {
        recv_command();
        exec_command();
    }
}

// Funzione di cleanup del thread padre
void handle_cancel_padre(void *arg)
{
    printf_yellow("Padre si spegne\n");
    close_connection();

    running = false;
}

// Funzione di cleanup del thread figlio
void handle_cancel_figlio(void *arg)
{
    printf_green("Client disconnesso\n");
    close_connection();

    free_username();
    free_game();

    // free the memory of the thread
    pthread_detach(pthread_self());
}

// Funzione di cleanup del thread
void handle_cancel(void *arg)
{
    if (isChild)
    {
        handle_cancel_figlio(arg);
    }
    else
    {
        handle_cancel_padre(arg);
    }
}

// Restituisce il numero di figli attivi
int get_childs()
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        childs--;
    }
    return childs;
}

// Funzione del thread server che accetta le nuove
void *server(void *porta)
{
    pthread_cleanup_push(handle_cancel, NULL);

    int new_sd; // Socket descriptor
    pid_t pid;

    int port = *((int *)porta);

    create_main_socket(port);

    log_info("Server creato sulla porta %d\n", port);

    running = true;

    while (1)
    {
        new_sd = create_child_socket();

        printf_green("Nuovo client connesso\n");

        pid = fork();

        if (pid == 0)
        {
            // Chiusura del listening socket
            close(sd);

            isChild = true;
            sd = new_sd;
            gioco();

            log_debug("Chiusura figlio\n");
            pthread_exit(NULL);
        }
        else
        {
            // Processo padre

            childs++;
            // Chiusura del socket connesso
            close(new_sd);
        }
    }
    pthread_cleanup_pop(0);
}

// Stampa l'help dei comandi del server
void print_help_server_commands()
{
    char help_text[] = "Server Started\n\n"
                       "Digita un comando: \n"
                       "  start <port>      avvia il server di gioco\n"
                       "  stop              termina il server\n"
                       "  status            stampa lo stato del server\n"
                       "  exit              chiude il programma\n\n";
    log_info("%s", help_text);
}

int main(int argc, char *argv[])
{

    // Controllo se è stato passato il parametro -v
    set_verbosity(argc, argv);

    // Leggo da data/rooms/available.txt quali sono le room utilizzabili
    init_rooms();

    int porta, ret;
    porta = get_porta_server(argc, argv);

    pthread_t tid;
    print_help_server_commands();

    printf_green("\nAvviando server sulla porta %d\n\n", porta);

    // Creazione del thread
    ret = pthread_create(&tid, NULL, server, (void *)&porta);

    while (1)
    {
        char input[100];
        char command[100];
        int new_porta;

        fgets(input, 100, stdin);
        command[0] = 0;
        ret = sscanf(input, "%s %d", command, &new_porta);

        if (strcmp(command, "stop") == 0)
        {
            if (!running)

            {
                printf_yellow("\nServer non avviato\n\n");
                continue;
            }

            else if (get_childs() > 0)
            {
                printf_yellow("\nCi sono ancora %d figli, non puoi stoppare il server\n\n", get_childs());
            }
            else
            {
                running = false;
                pthread_cancel(tid);
                pthread_join(tid, NULL);

                printf_green("\nServer stoppato\n\n");
            }
        }
        else if (strcmp(command, "start") == 0 && ret == 2)
        {
            if (running)
            {
                printf_yellow("\nServer già avviato sulla porta %d\n\n", porta);
            }
            else
            {
                printf_green("\nAvviando server sulla porta %d\n\n", new_porta);
                porta = new_porta;
                running = true;
                pthread_create(&tid, NULL, server, (void *)&porta);
            }
        }
        else if (strcmp(command, "status") == 0)
        {
            if (running)
            {
                printf_green("\nServer attivo sulla porta %d\n", porta);
                printf_green("Number of childs: %d\n\n", get_childs());
            }
            else
            {
                printf_green("\nServer non attivo\n\n");
            }
        }
        else if (strcmp(command, "exit") == 0)
        {
            if (running)
            {
                printf_yellow("\nDevi prima stoppare il server\n\n");
            }
            else
            {
                printf_green("\nServer terminato\n\n");
                free_rooms();
                exit(0);
            }
        }
        else
        {
            log_error("Comando non riconosciuto\n");
            print_help_server_commands();
        }
    }
}