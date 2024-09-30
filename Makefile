CC := gcc
OPTIONS = -Wall -Werror -g -pthread -std=gnu11 #-lssl -lcrypto

# Directories
SRC_DIR := src
OBJ_DIR := obj
DEP_DIR := dep

# Source files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
DEP_FILES := $(patsubst $(SRC_DIR)/%.c,$(DEP_DIR)/%.d,$(SRC_FILES))

SERVER_FILES := obj/server.o obj/utils.o obj/commands.o obj/utils_socket.o obj/game.o obj/server_socket.o
CLIENT_FILES := obj/client.o obj/utils.o obj/client_socket.o obj/utils_socket.o

all: server client

server: $(SERVER_FILES)
	$(CC) $(OPTIONS) -o $@ $^ 

client: $(CLIENT_FILES)
	$(CC) $(OPTIONS) -o $@ $^

# Include dependency files
ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

# Rule to generate object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEP_DIR)/%.d
	@mkdir -p $(DEP_DIR)
	$(CC) $(OPTIONS) -M $< | sed 's|\($*\)\.o *:|$(OBJ_DIR)/\1.o $@:|' > $(DEP_DIR)/$*.d
	@mkdir -p $(OBJ_DIR)
	$(CC) $(OPTIONS) -c $< -o $@

# Rule to generate dependency files
$(DEP_DIR)/%.d: ;

# Clean rule
clean:
	rm -rf $(OBJ_DIR) $(DEP_DIR) server client

# Phony targets
.PHONY: all clean