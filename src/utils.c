#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"

bool verbose = false; // Se true stampa i messaggi di debug

// Converte una stringa in intero, restituisce -1 se non è un intero
int convert_to_int(char *str)
{
    int ret = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
        {
            return -1;
        }
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}

// Imposta la variabile verbose in base agli argomenti passati al programma
void set_verbosity(int argc, char *argv[])
{
    verbose = false;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            verbose = true;
        }
    }
    log_debug("Verbose mode attiva\n");
}

// Stampa un messaggio di debug se verbose è true
void log_debug(const char *fmt, ...)
{
    if (!verbose)
    {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    // print grey
    printf("\033[0;37m");
    vprintf(fmt, ap);
    // print reset
    printf("\033[0m");
    fflush(stdout);
    va_end(ap);
}

// Stampa un messaggio di info
void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
}

// Stampa un messaggio di errore
void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    // print red
    fprintf(stderr, "\033[0;31m");
    vfprintf(stderr, fmt, ap);
    // If there was an error, print it
    if (errno != 0)
    {
        fprintf(stderr, " - %s", strerror(errno));
    }
    fprintf(stderr, "\n");
    // print reset
    fprintf(stderr, "\033[0m");
    fflush(stderr);
    va_end(ap);
}

// Stampa l'help del server
void help_server()
{
    char help[] = "Utilizzo: ./server [-v] <porta>\n";
    printf("%s", help);
}

// Stampa l'help del client
void help_client()
{
    char help[] = "Utilizzo: ./client [-v] [porta]\n";
    printf("%s", help);
}

// Prende la porta dal programma dagli argomenti e la restituisce
int get_porta_server(int argc, char *argv[])
{
    int porta;
    char *porta_str = argv[argc - 1];
    // Check int
    if (convert_to_int(porta_str) < 0)
    {
        log_error("Errore porta: Argomento non numerico o non specificato\n");
        help_server();
        exit(127);
    }
    porta = convert_to_int(porta_str);
    return porta;
}

// Prende la porta dal programma dagli argomenti e la restituisce
int get_porta_client(int argc, char *argv[])
{
    int porta;
    char *porta_str = argv[argc - 1];
    // Check int
    if (convert_to_int(porta_str) < 0)
    {
        log_error("Errore porta: Argomento non numerico o non specificato\n");
        help_client();
        exit(127);
    }
    porta = convert_to_int(porta_str);
    return porta;
}

// Stampa una stringa in rosso
void printf_red(char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    printf("\033[0;31m");
    vprintf(str, ap);
    printf("\033[0m");
    fflush(stdout);
}

// Stampa una stringa in verde
void printf_green(char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    printf("\033[0;32m");
    vprintf(str, ap);
    printf("\033[0m");
    fflush(stdout);
}

// Stampa una stringa in giallo
void printf_yellow(char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    printf("\033[0;33m");
    vprintf(str, ap);
    printf("\033[0m");
    fflush(stdout);
}