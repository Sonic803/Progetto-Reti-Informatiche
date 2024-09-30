void recv_command();
void gioco();
void handle_cancel_padre(void *arg);
void handle_cancel_figlio(void *arg);
void handle_cancel(void *arg);
int get_childs();
void *server(void *porta);
void print_help_server_commands();
int main(int argc, char *argv[]);