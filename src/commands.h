#include "defines.h"

struct Command
{
    char args[3][BUFFER_SIZE];
    int nargs;
};
enum Help_type
{
    NORMAL,
    ERROR,
    LOGIN_CORRECTLY
};
enum State
{
    LOGIN,
    LOBBY,
    GAME,
    ENIGMA,
    WRITE,
    END
};

// Init

void init_rooms();

// Send

void send_response(char *response);
void send_time_tokens();

// Exec

void exec_command();

// Special States

void enigma(char *buffer);
void write_bacheca(char *buffer);

// Common Commands

void help(char *buffer, enum Help_type type);
void exit_command(char *buffer);

// Login Commands

void signup(char *buffer);
void login(char *buffer);

// Lobby Commands

void start(char *buffer);
void logout(char *buffer);
void bacheca(char *buffer);

// Game Commands

void look(char *buffer);
void take(char *buffer);
void use(char *buffer);
void objs(char *buffer);
void end_command(char *buffer);

// Help

void help_command(char *buffer);
void help_login(char *buffer, enum Help_type type);
void help_lobby(char *buffer, enum Help_type type);
void help_game(char *buffer, enum Help_type type);

// Utils

bool check_username();
void hash(char *password, char *digest, char* salt);
void log_game(const char *fmt, ...);

// Free

void free_username();
void free_rooms();