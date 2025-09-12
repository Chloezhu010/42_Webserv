# 测试Makefile
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g
NAME = test_http_response

SRCS = test_http_response.cpp \
       ../http/http_response.cpp \
       ../http/http_request.cpp \
       ../client/client_connection.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: $(NAME)
	./$(NAME)

.PHONY: all clean fclean re test