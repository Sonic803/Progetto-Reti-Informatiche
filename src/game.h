struct Object;
struct Room;
struct Player;

// Init

void init_object(struct Object *object);
void init_room(struct Room *room);
void init_player(struct Player *player);

// Load

void load_game(int room_number);

// Get

void get_room_description(char *buffer);
int get_object_id(char *object_name);
void get_inventory(char *buffer);
int get_time();
int get_tokens();

// Commands

void look_object_location(char *buffer, char *object_location);
bool take_object(char *buffer, char *object_name);
void use_object(char *buffer, char *object_name, char *second_object_name);
bool solve_enigma(char *buffer, char *enigma_solution);

// Commands Utils
void achieve_object(int id);
void unlock_object(int id, char *buffer);
void activate_object(int id, char *buffer);
void add_object(int id);

// Utils

int parse_int_array(char *string, int **array);
void read_description(FILE *file, char *description);
int is_in_inventory(char *object_name);

// End

void end_time();
void end_game();
void end_1_second();

// Free

void free_game();