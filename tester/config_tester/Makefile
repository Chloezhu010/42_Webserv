# Makefile for testing initialize module
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I../configparser -I../client

# 源文件
TEST_SRCS = test_initialize.cpp
INIT_SRCS = ../configparser/initialize.cpp ../configparser/configparser.cpp ../configparser/configdisplay.cpp ../client/client_connection.cpp

# 目标文件
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
INIT_OBJS = $(INIT_SRCS:.cpp=.o)

# 可执行文件
TARGET = test_initialize

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(TEST_OBJS) $(INIT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(TEST_OBJS) $(INIT_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test: $(TARGET)
	@echo "Running initialize module tests..."
	@./$(TARGET)

clean:
	rm -f $(TEST_OBJS) $(INIT_OBJS) $(TARGET)
	rm -f *.conf  # 清理测试生成的配置文件

# 调试版本
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# 详细测试（包含内存检查）
test-verbose: $(TARGET)
	@echo "Running tests with valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# 性能测试
test-performance: $(TARGET)
	@echo "Running performance tests..."
	@time ./$(TARGET)