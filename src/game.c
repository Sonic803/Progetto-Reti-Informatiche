#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "game.h"
#include "commands.h"
#include "utils.h"
#include "server_socket.h"
#include "defines.h"

struct Object
{
    int id;                              // Id dell'oggetto
    char *name;                          // Nome dell'oggetto
    char *description_locked;            // Descrizione quando bloccato
    char *description_unlocked;          // Descrizione quando sbloccato
    char *object_activation_description; // Descrizione di quando l'oggetto si attiva
    char *enigma;                        // Enigma dell'oggetto
    char *enigma_solution;               // Soluzione dell'enigma
    bool locked;                         // Se true non usabile
    bool hidden;                         // Se true non visibile
    bool takeable;                       // Se true si può prendere
    int *unlock_with;                    // Array di id degli oggetti che devono essere activated per sbloccarlo
    int l_unlock_with;                   // Lunghezza di unlocked_with
    int *unhidden_with;                  // Array di id degli oggetti che devono essere activated per unhiddenarlo
    int l_unhidden_with;                 // Lunghezza di unhiddens_with
    int usable_with;                     // Se -2 allora l'oggetto non è utilizzabile e si attiva quando sbloccato.
                                         // Se diverso da -2 allora l'oggetto è takeable e può essere utilizzato da solo se
                                         // usable_with=-1 oppure con un oggetto con id=usable_with, si attiva con l'utilizzo
    int tries;                           // Tentativi rimanenti per risolvere l'enigma
    int tokens;                          // Numero di token che l'oggetto dà,
                                         // Se l'oggetto può essere raccolto li dà allora, altrimenti quando viene visto per la prima volta
    int time;                            // Quantità di secondi che l'oggetto conferisce, valgono le stesse regole di tokens
    bool rewarded;                       // True se raccolto, o se visto e non takeable, usato per token e time
    bool taken;                          // True se già presente nell'inventario
    bool activated;                      // True se l'oggetto è stato già attivato, usato unlock e unhidden
};

struct Location
{
    int id;            // Id della location
    char *name;        // Nome della location
    char *description; // Descrizione della location
};

struct Room
{
    int id;                     // Id della stanza
    char *name;                 // Nome della stanza
    char *description;          // Descrizione della stanza
    char *end_Description;      // Descrizione della vittoria
    struct Location *locations; // Array di location
    int l_locations;            // Lunghezza di locations
    struct Object *objects;     // Array di oggetti
    int l_objects;              // Lunghezza di objects
    int max_tokens;             // Somma dei token di tutti gli oggetti
};

struct Player
{
    int *inventory;     // Array di id degli oggetti nell'inventario
    int l_inventory;    // Lunghezza di inventory
    int tokens;         // Numero di token raccolti
    int time;           // Durata massima della partita, con aggiunta di tempo bonus
    time_t start_time;  // Tempo di inizio della partita
    int current_enigma; // Id dell'enigma corrente
};

enum load_type // Quale sezione del file si sta leggendo
{
    ROOM,
    LOCATIONS,
    OBJECTS,
    NONE
};

struct Room room;
struct Player player;

extern enum State state;

// Init

// Inizializza l'object
void init_object(struct Object *object)
{
    object->id = -1;
    object->name = NULL;
    object->description_locked = NULL;
    object->description_unlocked = NULL;
    object->object_activation_description = NULL;
    object->enigma = NULL;
    object->enigma_solution = NULL;
    object->locked = false;
    object->hidden = false;
    object->takeable = false;
    object->unlock_with = NULL;
    object->l_unlock_with = 0;
    object->unhidden_with = NULL;
    object->l_unhidden_with = 0;
    object->usable_with = -2;
    object->tries = -1;
    object->tokens = 0;
    object->time = 0;
    object->rewarded = false;
    object->taken = false;
    object->activated = false;
}
// Inizializza la room
void init_room(struct Room *room)
{
    room->id = -1;
    room->name = NULL;
    room->description = NULL;
    room->end_Description = NULL;
    room->locations = malloc(sizeof(struct Location));
    room->l_locations = 0;
    room->objects = malloc(sizeof(struct Object));
    room->l_objects = 0;
    room->max_tokens = 0;
}

// Inizializza il player
void init_player(struct Player *player)
{
    player->inventory = malloc(sizeof(int));
    player->l_inventory = 0;
    player->tokens = 0;
    player->time = 0;
    player->start_time = time(NULL);
    player->current_enigma = -1;
}

// Load

// Carica il gioco, legge da file tutti i dati necessari dal file della room
void load_game(int room_number)
{
    log_debug("Caricamento gioco\n");
    enum load_type tipo = NONE;
    char line[BUFFER_SIZE];

    char path[128];
    sprintf(path, "data/rooms/%d.txt", room_number);
    FILE *file = fopen(path, "r");

    if (file == NULL)
    {
        log_error("Errore apertura file\n");
        exit(0);
    }

    int i_locations = -1;
    int i_objects = -1;

    init_room(&room);
    init_player(&player);

    while (fgets(line, sizeof(line), file))
    {
        // If line starts with '- ' it's a comment so skip it
        if (strlen(line) >= 2 && line[0] == '-' && line[1] == ' ')
        {
            continue;
        }
        if (strcmp(line, "ROOM\n") == 0)
        {
            tipo = ROOM;
            continue;
        }
        if (strcmp(line, "LOCATIONS\n") == 0)
        {
            tipo = LOCATIONS;
            continue;
        }
        if (strcmp(line, "OBJECTS\n") == 0)
        {
            tipo = OBJECTS;
            continue;
        }
        if (strcmp(line, "END\n") == 0)
        {
            tipo = NONE;
            continue;
        }
        switch (tipo)
        {
        case ROOM:
            // if line starts with 'Room ' than get the room id
            if (strncmp(line, "Room ", 5) == 0)
            {
                room.id = atoi(line + 5);
                log_debug("Room: %d\n", room.id);
            }
            else if (strcmp(line, "Room_Name:\n") == 0)
            {
                read_description(file, line);
                log_debug("Room_Name: %s\n", line);
                room.name = strdup(line);
            }
            else if (strcmp(line, "Room_Description:\n") == 0)
            {
                read_description(file, line);
                log_debug("Room_Description: %s\n", line);
                room.description = strdup(line);
            }
            else if (strcmp(line, "End_Description:\n") == 0)
            {
                line[0] = '\n';
                read_description(file, line+1);
                log_debug("End_Description: %s\n", line);
                room.end_Description = strdup(line);
            }
            else if (strncmp(line, "Time ", 5) == 0)
            {
                int time = atoi(line + 5);
                log_debug("Time: %d\n", time);
                player.time = time;
            }
            else if (strcmp(line, "\n") == 0)
            {
                // Do nothing
                ;
            }
            else
            {
                log_error("Errore ROOM: Formato file errato, stringa errata: %s\n", line);
                exit(0);
            }
            break;
        case LOCATIONS:
            if (strncmp(line, "Location ", 9) == 0)
            {
                int id = atoi(line + 9);
                log_debug("Location: %d\n", id);
                i_locations++;
                if (i_locations != id)
                {
                    log_error("Errore LOCATIONS: Formato file errato, id location errato: %d\n, dovrebbe essere %n", id, i_locations);
                    exit(0);
                }
                room.locations = realloc(room.locations, sizeof(struct Location) * (i_locations + 1));
                room.locations[i_locations].id = id;
            }
            else if (strcmp(line, "Location_Name:\n") == 0)
            {
                read_description(file, line);
                log_debug("Location_Name: %s\n", line);
                room.locations[i_locations].name = strdup(line);
            }
            else if (strcmp(line, "Location_Description:\n") == 0)
            {
                read_description(file, line);
                log_debug("Location_Description: %s\n", line);
                room.locations[i_locations].description = strdup(line);
            }
            else if (strcmp(line, "\n") == 0)
            {
                // Do nothing
                ;
            }
            else
            {
                log_error("Errore LOCATIONS: Formato file errato, stringa errata: %s\n", line);
                exit(0);
            }
            break;
        case OBJECTS:

            if (strncmp(line, "Object ", 7) == 0)
            {
                int id = atoi(line + 7);
                log_debug("Object: %d\n", id);
                i_objects++;
                if (i_objects != id)
                {
                    log_error("Errore OBJECTS: Formato file errato, id object errato: %d\n, dovrebbe essere %n", id, i_objects);
                    exit(0);
                }
                room.objects = realloc(room.objects, sizeof(struct Object) * (i_objects + 1));
                init_object(&room.objects[i_objects]);
                room.objects[i_objects].id = id;
            }
            else if (strcmp(line, "Object_Name:\n") == 0)
            {
                read_description(file, line);
                log_debug("Object_Name: %s\n", line);
                room.objects[i_objects].name = strdup(line);
            }
            else if (strcmp(line, "Object_Description_Locked:\n") == 0)
            {
                read_description(file, line);
                log_debug("Object_Description_Locked: %s\n", line);
                room.objects[i_objects].description_locked = strdup(line);
            }
            else if (strcmp(line, "Object_Description_Unlocked:\n") == 0)
            {
                read_description(file, line);
                log_debug("Object_Description_Unlocked: %s\n", line);
                room.objects[i_objects].description_unlocked = strdup(line);
            }
            else if (strcmp(line, "Object_Activation_Description:\n") == 0)
            {
                read_description(file, line);
                log_debug("Object_Activation_Description: %s\n", line);
                room.objects[i_objects].object_activation_description = strdup(line);
            }
            else if (strcmp(line, "Object_Enigma:\n") == 0)
            {
                read_description(file, line);
                log_debug("Object_Enigma: %s\n", line);
                room.objects[i_objects].enigma = strdup(line);
            }
            else if (strcmp(line, "Object_Enigma_Solution:\n") == 0)
            {
                read_description(file, line);
                log_debug("Object_Enigma_Solution: %s\n", line);
                room.objects[i_objects].enigma_solution = strdup(line);
            }
            else if (strcmp(line, "Locked\n") == 0)
            {
                log_debug("Locked\n");
                room.objects[i_objects].locked = true;
            }
            else if (strcmp(line, "Unhidden_with:\n") == 0)
            {
                fgets(line, sizeof(line), file);
                // Line of type "1 2 3 4 5\n", insert into array
                // using strtok
                log_debug("Unhidden_with: %s\n", line);
                room.objects[i_objects].l_unhidden_with = parse_int_array(line, &room.objects[i_objects].unhidden_with);
            }
            else if (strcmp(line, "Hidden\n") == 0)
            {
                log_debug("Hidden\n");
                room.objects[i_objects].hidden = true;
            }

            else if (strcmp(line, "Unlock_with:\n") == 0)
            {
                fgets(line, sizeof(line), file);
                // Line of type "1 2 3 4 5\n", insert into array
                // using strtok
                log_debug("Unlock_with: %s\n", line);
                room.objects[i_objects].l_unlock_with = parse_int_array(line, &room.objects[i_objects].unlock_with);
            }

            else if (strcmp(line, "Takeable\n") == 0)
            {
                log_debug("Takeable\n");
                room.objects[i_objects].takeable = true;
            }

            else if (strcmp(line, "Use_with:\n") == 0)
            {
                fgets(line, sizeof(line), file);
                // Line of type "1 2 3 4 5\n", insert into array
                // using strtok
                log_debug("Use_with: %s\n", line);
                room.objects[i_objects].usable_with = atoi(line);
            }
            else if (strncmp(line, "Enigma_Tries ", 13) == 0)
            {
                int tries = atoi(line + 13);
                log_debug("Enigma_Tries: %d\n", tries);
                room.objects[i_objects].tries = tries;
            }
            else if (strncmp(line, "Tokens ", 7) == 0)
            {
                int tokens = atoi(line + 7);
                log_debug("Tokens: %d\n", tokens);
                room.objects[i_objects].tokens = tokens;
                room.max_tokens += tokens;
            }
            else if (strncmp(line, "Time ", 5) == 0)
            {
                int time = atoi(line + 5);
                log_debug("Time: %d\n", time);
                room.objects[i_objects].time = time;
            }
            else if (strcmp(line, "\n") == 0)
            {
                // Do nothing
                ;
            }
            else
            {
                log_error("Errore OBJECTS: Formato file errato, stringa errata: %s\n", line);
                exit(0);
            }
            break;
        case NONE:
            // Do nothing
            break;
        }
    }
    fclose(file);
    room.l_locations = i_locations + 1;
    room.l_objects = i_objects + 1;
    // Register an alarm
    alarm(player.time);
    // Register a signal handler
    signal(SIGALRM, end_time);
    log_debug("Caricamento gioco completato\n");
}

// Get

// Copia la descrizione della room nel buffer
void get_room_description(char *buffer)
{
    // copy room description into buffer
    strcpy(buffer, room.description);
}

// Se l'oggetto è presente nella room ne restituisce l'id altrimenti -1
int get_object_id(char *object_name)
{
    for (int i = 0; i < room.l_objects; i++)
    {
        if (strcmp(room.objects[i].name, object_name) == 0)
        {
            return room.objects[i].id;
        }
    }
    return -1;
}

// Scrive gli oggetti dell'inventario nel buffer
void get_inventory(char *buffer)
{
    buffer[0] = '\0';
    for (int i = 0; i < player.l_inventory; i++)
    {
        strcat(buffer, room.objects[player.inventory[i]].name);
        if (i != player.l_inventory - 1)
        {
            strcat(buffer, "\n");
        }
    }
}

// Restituisce il numero di secondi rimanenti
int get_time()
{
    int remaining_time = player.time - (int)(time(NULL) - player.start_time);
    return remaining_time;
}

// Restituisce il numero di token del giocatore
int get_tokens()
{
    return player.tokens;
}

// Commands

// Azione look object/location, scrive la descrizione nel buffer se presente e in caso di achievement aggiunge i token e il tempo
void look_object_location(char *buffer, char *object_location)
{
    // find if there is an object or a location with the name object_location
    // if there is an object with that name, copy the description into buffer (using the correct one)
    // if there is a location with that name, copy the description into buffer
    buffer[0] = '\0';
    // print number of objects and locations
    for (int i = 0; i < room.l_objects; i++)
    {
        if (room.objects[i].hidden)
        {
            continue;
        }
        if (strcmp(room.objects[i].name, object_location) == 0)
        {
            log_debug("Sembrerebbe un %s", room.objects[i].name);
            if (room.objects[i].locked)
            {
                strcpy(buffer, room.objects[i].description_locked);
            }
            else
            {
                if (!room.objects[i].takeable)
                {
                    achieve_object(room.objects[i].id);
                }
                strcpy(buffer, room.objects[i].description_unlocked);
            }
            return;
        }
    }
    for (int i = 0; i < room.l_locations; i++)
    {
        if (strcmp(room.locations[i].name, object_location) == 0)
        {
            log_debug("Sembrerebbe un %s", room.locations[i].name);

            strcpy(buffer, room.locations[i].description);
            return;
        }
    }
}

// Azione take object, in caso di enigma copia la sua descrizione nel buffer.
// Se l'oggetto è takeable, non bloccato lo aggiunge all'inventario
bool take_object(char *buffer, char *object_name)
{
    for (int i = 0; i < room.l_objects; i++)
    {
        if (strcmp(room.objects[i].name, object_name) == 0)
        {
            log_debug("Sembrerebbe un %s", room.objects[i].name);
            if (room.objects[i].hidden)
            {
                log_debug("Non puoi prendere un oggetto nascosto");
                strcpy(buffer, "Non c'è nessun oggetto con quel nome");
                return 0;
            }
            if (room.objects[i].locked)
            {
                if (room.objects[i].enigma != NULL)
                {
                    sprintf(buffer, "Oggetto **%s** bloccato. Devi risolvere l'enigma!\n", room.objects[i].name);
                    strcat(buffer, room.objects[i].enigma);
                    player.current_enigma = room.objects[i].id;
                    return 1;
                }
                else
                {
                    log_debug("Non puoi prendere un oggetto bloccato");
                    strcpy(buffer, "L'oggetto è bloccato");
                    return 0;
                }
            }
            else
            {
                if (!room.objects[i].takeable)
                {
                    log_debug("Non puoi prendere un oggetto non prendibile");
                    strcpy(buffer, "Non puoi prendere questo oggetto");
                    return 0;
                }
                if (room.objects[i].taken)
                {
                    log_debug("Già preso questo oggetto");
                    strcpy(buffer, "Hai già preso questo oggetto");
                    return 0;
                }

                log_debug("Hai raccolto", room.objects[i].name);
                add_object(room.objects[i].id);
                achieve_object(room.objects[i].id);
                sprintf(buffer, "Hai raccolto %s", room.objects[i].name);
                return 0;
            }
        }
    }
    log_debug("Non c'è nessun oggetto con quel nome");
    strcpy(buffer, "Non c'è nessun oggetto con quel nome");
    return 0;
}

// Azione use object
void use_object(char *buffer, char *object_name, char *second_object_name)
{
    // check if object_name is in the inventory

    if (second_object_name[0] != 0)
        log_debug("Use %s with %s\n", object_name, second_object_name);
    else
        log_debug("Use %s\n", object_name);

    int id = is_in_inventory(object_name);
    int id2 = get_object_id(second_object_name);

    if (id == -1)
    {
        sprintf(buffer, "Non hai questo oggetto nell'inventario");
        return;
    }
    if (id2 == -1 && second_object_name[0] != 0)
    {
        sprintf(buffer, "Non c'è nessun oggetto col nome %s", second_object_name);
        return;
    }

    struct Object *Object = &room.objects[id];
    if (Object->usable_with == -1)
    {
        if (id2 == -1)
        {
            if (!Object->activated)
            {
                buffer[0] = '\0';
                activate_object(id, buffer);
            }
            else
            {
                sprintf(buffer, "Hai già usato questo oggetto");
            }
        }
        else
        {
            sprintf(buffer, "%s non sembra funzionare con %s\n", object_name, second_object_name);
        }
    }
    else
    {
        if (Object->usable_with == id2)
        {
            if (!Object->activated)
            {

                buffer[0] = '\0';
                activate_object(id, buffer);
            }
            else
            {
                sprintf(buffer, "Hai già usato questo oggetto");
            }
        }
        else
        {
            if (id2 == -1)
            {
                sprintf(buffer, "%s non sembra funzionare da solo", object_name);
            }
            else
            {
                sprintf(buffer, "%s non sembra funzionare con %s", object_name, second_object_name);
            }
        }
    }
}

// Prova a risolvere l'enigma
bool solve_enigma(char *buffer, char *enigma_solution)
{
    if (player.current_enigma == -1)
    {
        log_error("Non c'è nessun enigma da risolvere");
        exit(0);
    }
    if (strcmp(room.objects[player.current_enigma].enigma_solution, enigma_solution) == 0)
    {
        log_debug("Enigma risolto");
        strcpy(buffer, "Hai risolto l'enigma!\n");
        unlock_object(player.current_enigma, buffer);
        // remove last endline
        if (buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = '\0';
        player.current_enigma = -1;
        return true;
    }
    else
    {
        log_debug("Enigma non risolto");
        room.objects[player.current_enigma].tries--;

        if (room.objects[player.current_enigma].tries == 0)
        {
            // Perdi partita
            log_debug("Hai perso la partita");
            end_1_second();
            strcpy(buffer, "Non hai risolto l'enigma e sono finiti i tentativi");
        }
        else
        {
            if (room.objects[player.current_enigma].tries > 0)
                sprintf(buffer, "Non hai risolto l'enigma, ti rimangono %d tentativi", room.objects[player.current_enigma].tries);
            else
                sprintf(buffer, "Non hai risolto l'enigma");
        }

        return false;
    }
}
// Commands Utils

// Se l'oggett non era già stato rewarded prende i token e il tempo extra che esso conferisce.
// In caso fosse l'ultimo oggetto con token finisce la partita
void achieve_object(int id)
{
    if (room.objects[id].rewarded)
    {
        return;
    }
    room.objects[id].rewarded = true;
    player.tokens += room.objects[id].tokens;
    player.time += room.objects[id].time;
    if (player.tokens == room.max_tokens)
    {
        // in 1 second call end_game
        end_1_second();
    }
}

// Sblocca l'oggetto, se non usabile lo attiva
void unlock_object(int id, char *buffer)
{
    room.objects[id].locked = false;
    if (room.objects[id].usable_with == -2) // se l'oggetto non è usabile, quindi quando si sblocca
                                            // è come se venisse usato
    {
        activate_object(id, buffer);
    }
}

// Attiva l'oggetto, in caso in cui altri oggetti venisserò sbloccati/svelati da ciò li sblocca/svela anche
// con reazioni a catena
void activate_object(int id, char *buffer)
{
    room.objects[id].activated = true;
    if (room.objects[id].object_activation_description)
    {
        strcat(buffer, room.objects[id].object_activation_description);
        strcat(buffer, "\n");
    }
    bool done_something = true;
    while (done_something)
    {
        done_something = false;
        for (int i = 0; i < room.l_objects; i++)
        {
            if (room.objects[i].hidden)
            {
                bool all_activated = true;
                for (int j = 0; j < room.objects[i].l_unhidden_with; j++)
                {
                    int obj = room.objects[i].unhidden_with[j];
                    if (!room.objects[obj].activated)
                    {
                        all_activated = false;
                    }
                }
                if (all_activated)
                {
                    room.objects[i].hidden = false;
                    done_something = true;
                }
            }
            else if (room.objects[i].locked)
            {

                if (room.objects[i].enigma != NULL)
                {
                    continue;
                }
                bool all_activated = true;

                for (int j = 0; j < room.objects[i].l_unlock_with; j++)
                {
                    int obj = room.objects[i].unlock_with[j];
                    if (!room.objects[obj].activated)
                    {
                        all_activated = false;
                    }
                }
                if (all_activated)
                {
                    unlock_object(room.objects[i].id, buffer);
                    // remove last endline
                    if (buffer[strlen(buffer) - 1] == '\n')
                        buffer[strlen(buffer) - 1] = '\0';
                    done_something = true;
                }
            }
        }
    }
}

// Aggiunge l'oggetto all'inventario
void add_object(int id)
{
    room.objects[id].taken = true;
    player.inventory = realloc(player.inventory, sizeof(int) * (player.l_inventory + 1));
    player.inventory[player.l_inventory] = id;
    player.l_inventory++;
}

// Utils

// Parsa un array di interi
// Array tipo "1 2 3 4 5"
int parse_int_array(char *string, int **array)
{
    char *token = strtok(string, " ");
    int i = 0;
    *array = malloc(sizeof(int));
    while (token != NULL)
    {
        *array = realloc(*array, sizeof(int) * (i + 1));
        (*array)[i] = atoi(token);
        log_debug("Array[%d]: %d\n", i, (*array)[i]);
        token = strtok(NULL, " ");
        i++;
    }
    return i;
}

// Legge una descrizione da file, legge fino a che non trova una riga vuota
void read_description(FILE *file, char *description)
{
    // Read until you read \n\n
    char line[BUFFER_SIZE];
    description[0] = '\0';
    while (fgets(line, sizeof(line), file))
    {
        log_debug("Line: %s\n", line);
        if (strcmp(line, "\n") == 0)
        {
            break;
        }
        strcat(description, line);
    }
    // remove last \n
    if (description[strlen(description) - 1] == '\n')
    {
        description[strlen(description) - 1] = '\0';
    }
}

// Controlla se un oggetto è nell'inventario, ritorna l'id dell'oggetto se presente, -1 altrimenti
int is_in_inventory(char *object_name)
{
    for (int i = 0; i < player.l_inventory; i++)
    {
        if (strcmp(room.objects[player.inventory[i]].name, object_name) == 0)
        {
            return player.inventory[i];
        }
    }
    return -1;
}

// End

// Viene usata con un timer, quando si attiva controlla se il tempo è veramente finito
// (in caso di tempo extra potrebbe non esserlo), se lo è finisce il gioco altrimenti
// si rilancia quando pensa che il tempo sarà finito
void end_time()
{
    int remaining_time = get_time();
    if (remaining_time <= 0)
    {
        log_debug("Tempo scaduto");
        end_game();
    }
    else
    {
        log_debug("Tempo rimanente: %d", remaining_time);
        alarm(remaining_time);
    }
}

// Fa finire il gioco e avverte il client
void end_game()
{
    log_debug("Fine gioco");
    state = END;
    if (player.tokens == room.max_tokens)
    {
        send_response(room.end_Description);
    }
    else
    {
        char response[128];
        sprintf(response, "\nFinisci la partita con %d token", player.tokens);
        send_response(response);
    }
    pthread_exit(NULL);
}

// Fa finire il gioco tra 1 secondo
void end_1_second()
{

    signal(SIGALRM, end_game);
    alarm(1);
}

// Free

// Libera la memoria allocata per il gioco, deve venire chiamata alla fine del gioco
void free_game()
{
    for (int i = 0; i < room.l_objects; i++)
    {
        free(room.objects[i].name);
        free(room.objects[i].description_locked);
        free(room.objects[i].description_unlocked);
        free(room.objects[i].object_activation_description);
        free(room.objects[i].enigma);
        free(room.objects[i].enigma_solution);
        free(room.objects[i].unlock_with);
        free(room.objects[i].unhidden_with);
    }
    for (int i = 0; i < room.l_locations; i++)
    {
        free(room.locations[i].name);
        free(room.locations[i].description);
    }

    free(room.name);
    free(room.description);
    free(room.end_Description);
    free(room.locations);
    free(room.objects);

    free(player.inventory);
}
