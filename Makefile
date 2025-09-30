NAME = webserv

# Source directories
SRC_DIR = ./src
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/objs

# Source files with new paths
SRC = $(SRC_DIR)/main/server.cpp \
	  $(SRC_DIR)/configparser/configparser.cpp \
	  $(SRC_DIR)/configparser/initialize.cpp \
	  $(SRC_DIR)/configparser/configdisplay.cpp \
	  $(SRC_DIR)/http/http_response.cpp \
	  $(SRC_DIR)/http/http_request.cpp \
	  $(SRC_DIR)/client/client_connection.cpp \
	  $(SRC_DIR)/cgi/cgi_handler.cpp \
	  $(SRC_DIR)/cgi/cgi_environment.cpp \
	  $(SRC_DIR)/cgi/cgi_process.cpp \
	  $(SRC_DIR)/cgi/cgi_response.cpp

# Object files in build directory
OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
CC = c++

# Debug vs Release flags
DEBUG_FLAGS = -g -O0 -Wall -Wextra -Werror -std=c++98 -DDEBUG
RELEASE_FLAGS = -O2 -Wall -Wextra -Werror -std=c++98 -DNDEBUG

# Default to debug build
FLAGS = $(DEBUG_FLAGS)

all: $(NAME)

# Create build directories and compile
$(NAME): $(OBJ)
	@echo "Linking $(NAME)..."
	$(CC) $(FLAGS) -o $(BUILD_DIR)/$(NAME) $(OBJ)
	@echo "Build complete: $(BUILD_DIR)/$(NAME)"
	@ln -sf $(BUILD_DIR)/$(NAME) $(NAME)

# Compile object files into build/objs/ preserving directory structure
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(FLAGS) -c $< -o $@

clean:
	@echo "Cleaning object files..."
	rm -rf $(BUILD_DIR)/objs
	@echo "Clean complete."

fclean: clean
	@echo "Removing executable..."
	rm -f $(BUILD_DIR)/$(NAME) $(NAME)
	rm -rf $(BUILD_DIR)
	@echo "Full clean complete."

re: fclean all

# Debug and Release targets
debug: FLAGS = $(DEBUG_FLAGS)
debug: re

release: FLAGS = $(RELEASE_FLAGS)
release: re

# Development utility targets
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(NAME) config/default.conf

valgrind-simple: debug
	valgrind --leak-check=full ./$(NAME) config/default.conf

lldb: debug
	lldb ./$(NAME) -- config/default.conf

run: all
	./$(NAME) config/default.conf

# Show compiled files info
info:
	@echo "Executable: $(BUILD_DIR)/$(NAME)"
	@echo "Object files directory: $(OBJ_DIR)"
	@echo "Source files:"
	@echo "$(SRC)" | tr ' ' '\n'

.PHONY: all clean fclean re debug release valgrind valgrind-simple lldb run info