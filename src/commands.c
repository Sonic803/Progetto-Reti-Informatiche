#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
//#include <openssl/evp.h>
#include <stdarg.h>
#include <time.h>

#include "commands.h"
#include "utils.h"
#include "utils_socket.h"
#include "game.h"
#include "defines.h"

#define DIGEST_SIZE 32 // Dimensione del digest di sha256

struct Command com;       // Comando ricevuto dal client
enum State state = LOGIN; // Stato del programma
struct Room_info          // Struttura che contiene le informazioni sulle stanze per il comando help, start
{
    int id;
    char *name;
} *rooms = NULL;

int nrooms = 0;     // Numero di stanze disponibili
char *username = NULL; // Username dell'utente loggato

// Init

// Inizializza l'array rooms per il comando help, start
void init_rooms()
{
    // Inizializzo le stanze
    // Basta leggere il file rooms/available.txt
    FILE *f = fopen("data/rooms/available.txt", "r");
    if (f == NULL)
    {
        log_error("File data/rooms/available.txt non trovato");
        exit(1);
    }
    rooms = malloc(sizeof(struct Room_info));
    char buffer[BUFFER_SIZE];
    int id;
    nrooms = 0;
    while (fscanf(f, "%d %s", &id, buffer) != EOF)
    {
        rooms = realloc(rooms, sizeof(struct Room_info) * (nrooms + 1));
        rooms[nrooms].id = id;
        rooms[nrooms].name = malloc(strlen(buffer) + 1);
        strcpy(rooms[nrooms].name, buffer);
        nrooms++;
    }
    fclose(f);
}

// Send

// Comando principale per inviare la risposta al client, invia anche lo stato del programma
void send_response(char *response)
{
    log_debug("Invio risposta %s\n", response);
    // Response string
    send_string(response);

    // Program status
    // 0 = login/lobby
    // 1 = lobby
    // 2 = game
    // 3 = write/enigma
    // 4 = end

    switch (state)
    {
    case LOGIN:
        send_int(0);
        break;
    case LOBBY:
        send_int(1);
        // send username
        send_string(username);
        break;
    case GAME:
        send_int(2);
        send_time_tokens();
        break;
    case WRITE:
    case ENIGMA:
        send_int(3);
        break;
    case END:
        send_int(4);
        break;
    default:
        log_error("Stato non riconosciuto\n");
        break;
    }
}

// Invia il tempo rimasto e i tokens rimasti, vengono inviati solo se lo stato è GAME
void send_time_tokens()
{
    send_int(get_time());
    send_int(get_tokens());
}

// Exec

// Esegue il comando ricevuto dal client
void exec_command()
{
    log_debug("Ricevuto %s da eseguire come comando", com.args[0]);

    char buffer[BUFFER_SIZE];
    buffer[0] = '\0';

    if (state == ENIGMA) // Se sono in stato ENIGMA, devo prendere la risposta e controllare se è corretta
    {
        enigma(buffer);
    }
    else if (state == WRITE) // Se sono in stato WRITE, devo scrivere sulla bacheca la stringa ricevuta
    {
        write_bacheca(buffer);
    }
    else if (strcmp(com.args[0], "signup") == 0)
    {
        signup(buffer);
    }
    else if (strcmp(com.args[0], "login") == 0)
    {
        login(buffer);
    }
    else if (strcmp(com.args[0], "exit") == 0)
    {
        exit_command(buffer);
    }
    else if (strcmp(com.args[0], "help") == 0)
    {
        help(buffer, 0);
    }
    else if (strcmp(com.args[0], "start") == 0)
    {
        start(buffer);
    }
    else if (strcmp(com.args[0], "logout") == 0)
    {
        logout(buffer);
    }
    else if (strcmp(com.args[0], "bacheca") == 0)
    {
        bacheca(buffer);
    }
    else if (strcmp(com.args[0], "look") == 0)
    {
        look(buffer);
    }
    else if (strcmp(com.args[0], "take") == 0)
    {
        take(buffer);
    }
    else if (strcmp(com.args[0], "use") == 0)
    {
        use(buffer);
    }
    else if (strcmp(com.args[0], "objs") == 0)
    {
        objs(buffer);
    }
    else if (strcmp(com.args[0], "end") == 0)
    {
        end_command(buffer);
    }
    else
    {
        log_game("Comando non esistente");
        help(buffer, 1);
    }
    send_response(buffer);
}

// Special States

// Passa a game.c la risposta all'enigma dell'utente
void enigma(char *buffer)
{
    log_game("Risposta all'enigma");
    if (com.nargs != 1)
    {
        log_error("Errore ENIGMA: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    solve_enigma(buffer, com.args[0]);
    state = GAME;
}

// Scrive sulla bacheca la stringa ricevuta
void write_bacheca(char *buffer)
{
    log_game("Write sulla bacheca");
    if (com.nargs != 1)
    {
        log_error("Errore WRITE: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    // open file data/bacheca/bacheca.txt
    FILE *f = fopen("data/bacheca/bacheca.txt", "a");
    if (f == NULL)
    {
        log_error("File data/bacheca/bacheca.txt non trovato");
        strcpy(buffer, "La bacheca non è disponibile");
    }

    // write com.args[0] on file
    fprintf(f, "\033[0;33m%s\033[0m: %s\n", username, com.args[0]);
    fclose(f);
    log_debug("Messaggio scritto sulla bacheca\n");
    state = LOBBY;
    strcpy(buffer, "Messaggio scritto sulla bacheca");
}

// Common Commands

// Comando per chiedere informazioni sui comandi.
// In base allo stato e al tipo di help richiesto, invia un messaggio diverso
void help(char *buffer, enum Help_type type)
{
    if (com.nargs == 2 && type == NORMAL)
    {
        help_command(buffer);
    }
    else
    {
        if (state == LOGIN)
            help_login(buffer, type);
        else if (state == LOBBY)
            help_lobby(buffer, type);
        else if (state == GAME)
            help_game(buffer, type);
    }
}

// Comando per chiudere il programma, cambia lo stato a END
void exit_command(char *buffer)
{
    log_game("Comando exit");
    if (state != LOGIN && state != LOBBY)
    {
        log_error("Errore EXIT: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    state = END;
    if (com.nargs != 1)
    {
        log_error("Errore EXIT: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    strcpy(buffer, "Connessione terminata");
}

// Login Commands

// Comando per registrarsi, crea un file con il nome dell'utente e la password hashata
void signup(char *buffer)
{
    log_game("Comando signup");
    if (state != LOGIN)
    {
        log_error("Errore SIGNUP: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 3)
    {
        log_error("Errore SIGNUP: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    if (!check_username())
    {
        log_error("Errore SIGNUP: Username non valido");
        strcpy(buffer, "Username non valido");
        return;
    }

    char path[BUFFER_SIZE * 2];
    sprintf(path, "data/login/%s.txt", com.args[1]);
    FILE *file = fopen(path, "r");
    if (file != NULL)
    {
        log_error("Errore SIGNUP: Username già esistente");
        strcpy(buffer, "Username già esistente");
        return;
    }
    else
    {
        char digest[DIGEST_SIZE*2+1];
        char salt[17];
        srand(time(NULL));
        for (int i = 0; i < 16; i++)
        {
            salt[i] = (char)(rand() % 26 + 'a');
        }
        salt[16]=0;
        hash(com.args[2], digest,salt);

        file = fopen(path, "w");
        fprintf(file, "%s %s\n", salt, digest);
        fclose(file);
    }

    strcpy(buffer, "SIGNUP CORRECTLY");
}

// Comando per fare il login, controlla se l'utente esiste e se la password è corretta
void login(char *buffer)
{
    log_game("Comando login");
    if (state != LOGIN)
    {
        log_error("Errore LOGIN: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 3)
    {
        log_error("Errore LOGIN: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    if (!check_username())
    {
        log_error("Errore LOGIN: Username non valido");
        strcpy(buffer, "Username non valido");
        return;
    }
    char path[BUFFER_SIZE * 2];
    sprintf(path, "data/login/%s.txt", com.args[1]);
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        log_error("Errore LOGIN: Username non esistente");
        strcpy(buffer, "Username non esistente");
        return;
    }
    else
    {
        char password[DIGEST_SIZE*2+1];
        char salt[17];
        //read string salt
        fscanf(file, "%s %s", salt, password);
        fclose(file);

        char digest[DIGEST_SIZE*2+1];
        hash(com.args[2], digest,salt);


        if (strcmp(digest,password) != 0)
        {
            log_error("Errore LOGIN: Password errata");
            strcpy(buffer, "Password errata");
            return;
        }
        else
        {
            log_game("Utente %s loggato correttamente", com.args[1]);
            username = malloc(strlen(com.args[1]) + 1);
            strcpy(username, com.args[1]);
            state = LOBBY;
            help(buffer, LOGIN_CORRECTLY);
            return;
        }
    }
}

// Lobby Commands

// Comando per iniziare una partita, cambia lo stato a GAME, carica la stanza e invia la descrizione
void start(char *buffer)
{
    log_game("Comando start");
    if (state != LOBBY)
    {
        log_error("Errore START: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 2)
    {
        log_error("Errore START: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    int room = convert_to_int(com.args[1]);
    if (room == -1)
    {
        log_error("Errore START: Argomento non numerico");
        help(buffer, ERROR);
        return;
    }
    for (int i = 0; i < nrooms; i++)
    {
        if (rooms[i].id == room)
        {
            log_debug("STARTING CORRECTLY %d\n", room);
            state = GAME;
            load_game(room);
            strcpy(buffer, "STARTING ROOM\n\n");
            get_room_description(buffer + strlen(buffer));
            return;
        }
    }
    log_error("Errore START: Room non disponibile");
    strcpy(buffer, "Room non disponibile");
}

// Comando per fare il logout, cambia lo stato a LOGIN
void logout(char *buffer)
{
    log_game("Comando logout");
    if (state != LOBBY)
    {
        log_error("Errore LOGOUT: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 1)
    {
        log_error("Errore LOGOUT: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    free_username();
    state = LOGIN;
    log_debug("LOGOUT CORRECTLY\n");
    help(buffer, NORMAL);
}

// Comando per vedere o scrivere sulla bacheca.
// Se si passa il parametro write cambia lo stato a WRITE, se non si passa il parametro stampa la bacheca
void bacheca(char *buffer)
{
    log_game("Comando bacheca");
    if (state != LOBBY)
    {
        log_error("Errore BACHECA: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs > 2)
    {
        log_error("Errore BACHECA: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }

    if (com.nargs == 1)
    {
        char line[BUFFER_SIZE];
        // open file data/bacheca/bacheca.txt
        FILE *f = fopen("data/bacheca/bacheca.txt", "r");
        if (f == NULL)
        {
            log_error("File data/bacheca/bacheca.txt non trovato");
            strcpy(buffer, "La bacheca non è disponibile o vuota");
            return;
        }
        // read file to buffer
        while (fgets(line, BUFFER_SIZE, f) != NULL)
        {
            // inverse order of lines
            //char temp[BUFFER_SIZE];
            //strcpy(temp, buffer);
            //strcpy(buffer, line);
            strcat(buffer, line);
        }
    }
    else
    {
        if (strcmp(com.args[1], "write") == 0)
        {
            strcpy(buffer, "Scrivi il messaggio da mettere sulla bacheca");
            state = WRITE;
            log_debug("Stato cambiato in WRITE\n");
        }
        else
        {
            log_error("Errore BACHECA: Argomento non riconosciuto");
            help(buffer, ERROR);
        }
    }
}

// Game Commands

// Comando usato per guardare la stanza, una location o un oggetto
void look(char *buffer)
{
    log_game("Comando look");
    if (state != GAME)
    {
        log_error("Errore LOOK: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs == 3)
    {
        log_error("Errore LOOK: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs == 1)
    {
        get_room_description(buffer);
    }
    else if (com.nargs == 2)
    {
        look_object_location(buffer, com.args[1]);
    }
    if (buffer[0] == '\0') // Se l'oggetto o la location non esiste
    {
        log_error("Errore LOOK: Oggetto o location non esistenti");
        strcpy(buffer, "Non vedi quello che cerchi");
    }
}

// Comando usato per interagire con un oggetto se bloccato da un enigma, o per prendere un oggetto se sbloccato e takeable
void take(char *buffer)
{
    log_game("Comando take");
    if (state != GAME)
    {
        log_error("Errore LOOK: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 2)
    {
        log_error("Errore TAKE: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    bool enigma = take_object(buffer, com.args[1]);
    if (enigma)
    {
        state = ENIGMA;
    }
}

// Comando per usare un oggetto che si ha nell'inventario, da solo o con un altro oggetto presente nella stanza
void use(char *buffer)
{
    log_game("Comando use");
    if (state != GAME)
    {
        log_error("Errore LOOK: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs < 2)
    {
        log_error("Errore USE: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs == 2)
    {
        com.args[2][0] = 0;
    }
    use_object(buffer, com.args[1], com.args[2]);
}

// Comando per stampare l'inventario
void objs(char *buffer)
{
    log_game("Comando objs");
    if (state != GAME)
    {
        log_error("Errore LOOK: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 1)
    {
        log_error("Errore OBJS: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    get_inventory(buffer);
}

// Comando per terminare la partita e disconnettersi
void end_command(char *buffer)
{
    log_game("Comando end");
    if (state != GAME)
    {
        log_error("Errore END: Comando non disponibile in questo stato");
        help(buffer, ERROR);
        return;
    }
    if (com.nargs != 1)
    {
        log_error("Errore END: Numero di argomenti errato");
        help(buffer, ERROR);
        return;
    }
    strcpy(buffer, "Terminando la partita");
    end_1_second();
}

// Help

// Comando help <comando>, invia la descrizione del comando richiesto
void help_command(char *buffer)
{
    log_game("Comando help <comando>");
    char **avaible_commands;
    char **description;
    int n;
    if (state == LOGIN)
    {
        char *avaible_commands_lobby[] = {"login", "signup", "exit", "help"};
        char *description_lobby[] = {"Comando per fare il login, ha bisogno di <username> <password> come parametri, entrambi devono essere alfanumerici",
                                     "Comando per fare il signup, ha bisogno di <username> <password> come parametri, entrambi devono essere alfanumerici",
                                     "Comando per chiudere il programma",
                                     "Comando per chiedere informazioni su un comando"};
        avaible_commands = avaible_commands_lobby;
        description = description_lobby;
        n = 4;
    }
    else if (state == LOBBY)
    {
        char *avaible_commands_lobby[] = {"start", "logout", "bacheca", "exit", "help"};
        char *description_lobby[] = {"Comando per iniziare una partita, ha bisogno di <room> come parametro, deve essere un numero intero positivo, ed essere una delle room disponibili",
                                     "Comando per fare il logout",
                                     "Comando per la bacheca, se si passa il parametro write si può scrivere sulla bacheca altrmenti si legge",
                                     "Comando per chiudere il programma",
                                     "Comando per chiedere informazioni su un comando"};
        avaible_commands = avaible_commands_lobby;
        description = description_lobby;
        n = 5;
    }
    else if (state == GAME)
    {
        char *avaible_commands_game[] = {"look", "take", "use", "objs", "end", "help"};
        char *description_game[] = {"Comando per guardare la stanza, una location o un oggetto, se usato senza parametri mostra la stanza, se usato con una location o un oggetto mostra la descrizione di quella location o oggetto",
                                    "Comando per interagire con un oggetto, ha bisogno di <object> come parametro, deve essere un oggetto presente nella stanza.\nSe l'oggetto è sbloccato e può essere preso, lo prende.\nSe bloccato da un enigma, mostra l'enigma",
                                    "Comando per utilizzare un oggetto che si ha nell'inventario, si può usare da solo o con un secondo oggetto presente nella stanza",
                                    "Comando per mostrare gli oggetti che si hanno nell'inventario",
                                    "Comando per terminare la partita",
                                    "Comando per chiedere informazioni su un comando"};
        avaible_commands = avaible_commands_game;
        description = description_game;
        n = 6;
    }
    bool found = false;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(avaible_commands[i], com.args[1]) == 0)
        {
            found = 1;
            strcpy(buffer, description[i]);
            break;
        }
    }
    if (found == false)
    {
        help(buffer, ERROR);
    }
}

// Comando help se lo stato è LOGIN, invia la lista dei comandi disponibili
void help_login(char *buffer, enum Help_type type)
{
    log_game("Comando help_login");
    char help_text[] = "Comandi disponibili:\n"
                       "  signup <username> <password>\n"
                       "  login <username> <password>\n"
                       "  exit\n"
                       "  help [comando]";
    if (type == ERROR)
    {
        strcpy(buffer, "Comando non riconosciuto\n\n");
    }
    strcat(buffer, help_text);
}

// Comando help se lo stato è LOBBY, invia la lista dei comandi disponibili
void help_lobby(char *buffer, enum Help_type type)
{
    log_game("Comando help_lobby");
    char help_text[] = "Comandi disponibili:\n"
                       "  start <room>\n"
                       "  logout\n"
                       "  bacheca [write]\n"
                       "  exit\n"
                       "  help [comando]\n\n"
                       "  rooms disponibili:\n";
    if (type == ERROR)
    {
        strcpy(buffer, "Comando non riconosciuto\n\n");
    }
    if (type == LOGIN_CORRECTLY)
    {
        strcpy(buffer, "Login effettuato correttamente\n\n");
    }
    strcat(buffer, help_text);
    for (int i = 0; i < nrooms; i++)
    {
        char room[BUFFER_SIZE];
        sprintf(room, "    %d %s", rooms[i].id, rooms[i].name);
        strcat(buffer, room);
        if (i != nrooms - 1)
        {
            strcat(buffer, "\n");
        }
    }
}

// Comando help se lo stato è GAME, invia la lista dei comandi disponibili
void help_game(char *buffer, enum Help_type type)
{
    log_game("Comando help_game");
    char help_text[] = "Comandi disponibili:\n"
                       "  look [location | object]\n"
                       "  take <object>\n"
                       "  use <object1> [object2]\n"
                       "  objs\n"
                       "  end\n"
                       "  help [command]\n";
    if (type == ERROR)
    {
        strcpy(buffer, "Comando non riconosciuto\n\n");
    }
    strcat(buffer, help_text);
}

// Utils

// Controlla se l'username è alfanumerico
bool check_username()
{
    char *username = com.args[1];
    while (*username != '\0')
    {
        if (!isalnum(*username))
        {
            return false;
        }
        username++;
    }
    return true;
}

// Fa l'hash della password usando sha256
void hash(char *password, char *digest, char* salt)
{
    // Openssl per fare l'hash della password
    //EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    //const EVP_MD *md = EVP_sha256();

    //EVP_DigestInit_ex(mdctx, md, NULL);
    //EVP_DigestUpdate(mdctx, password, strlen(password));
    //EVP_DigestFinal_ex(mdctx, digest, NULL);
    //EVP_MD_CTX_free(mdctx);

    // Si usa sha256sum
    char command[BUFFER_SIZE];
    sprintf(command, "echo -n %s%s | sha256sum | awk '{print $1}'", salt,password);
    FILE *fp = popen(command, "r");
    fscanf(fp, "%s", digest);
    pclose(fp);
}

void log_game(const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    if (username != NULL)
    {
        printf("\033[0;33m");
        printf("%s: ", username);
        printf("\033[0m");
    }else{
        printf("\033[0;33m");
        printf("???: ");
        printf("\033[0m");
    }
    vprintf(fmt, ap);
    printf("\n");
    fflush(stdout);
    va_end(ap);
}

// Free

// Libera la memoria allocata per l'username, viene chiamata quando si fa il logout o quando si chiude il programma
void free_username()
{

    free(username);
    username = NULL;
}

// Libera la memoria allocata per le stanze, viene chiamata quando si fa exit dal server
void free_rooms()
{

    for (int i = 0; i < nrooms; i++)
    {
        free(rooms[i].name);
    }
    free(rooms);
}
