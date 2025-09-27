NAME = test
MULTIPART_TEST = multipart_test

# Default test (change SRC to point to desired test file)
SRC = ./test.cpp \
		../http/http_request.cpp \
		../http/http_response.cpp \

# Multipart form data test
MULTIPART_SRC = ./MultipartFormData_unit_test.cpp \
				../http/http_request.cpp \
				../http/http_response.cpp \

OBJ = $(SRC:.cpp=.o)
MULTIPART_OBJ = $(MULTIPART_SRC:.cpp=.o)

CC = c++
FLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(FLAGS) -o $(NAME) $(OBJ)

# Build multipart test
multipart: $(MULTIPART_TEST)

$(MULTIPART_TEST): $(MULTIPART_OBJ)
	$(CC) $(FLAGS) -o $(MULTIPART_TEST) $(MULTIPART_OBJ)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

# Run multipart tests
test-multipart: $(MULTIPART_TEST)
	./$(MULTIPART_TEST)

clean:
	rm -f $(OBJ) $(MULTIPART_OBJ)

fclean: clean
	rm -f $(NAME) $(MULTIPART_TEST)

re: fclean all

.PHONY: all clean fclean re multipart test-multipart