NAME = webserv
SRC = ./main/server.cpp \
	  ./configparser/configparser.cpp \
	  ./configparser/initialize.cpp \
	  ./configparser/configdisplay.cpp \
	  ./http/http_response.cpp \
	  ./http/http_request.cpp \
	  ./client/client_connection.cpp \
	  ./cgi/cgi_handler.cpp \
	  ./cgi/cgi_environment.cpp \
	  ./cgi/cgi_process.cpp \
	  ./cgi/cgi_response.cpp
OBJ = $(SRC:.cpp=.o)
CC = c++

# Debug vs Release flags
DEBUG_FLAGS = -g -O0 -Wall -Wextra -Werror -std=c++98 -DDEBUG
RELEASE_FLAGS = -O2 -Wall -Wextra -Werror -std=c++98 -DNDEBUG

# Default to debug build
FLAGS = $(DEBUG_FLAGS)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(FLAGS) -o $(NAME) $(OBJ)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Debug and Release targets
debug: FLAGS = $(DEBUG_FLAGS)
debug: re

release: FLAGS = $(RELEASE_FLAGS)
release: re

# Development utility targets
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(NAME) nginx1.conf

valgrind-simple: debug
	valgrind --leak-check=full ./$(NAME) nginx1.conf

lldb: debug
	lldb ./$(NAME) -- nginx1.conf

.PHONY: all clean fclean re debug release valgrind lldb