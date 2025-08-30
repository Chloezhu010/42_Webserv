#!/bin/bash
# run_tests.sh

echo "Building and running webserv initialize tests..."
echo "================================================="

# 编译测试
echo "Compiling tests..."
make clean
if ! make test_initialize; then
    echo "❌ Failed to compile initialize tests"
    exit 1
fi

echo -e "\n🔧 Compiling config parser tests..."
if ! c++ -Wall -Wextra -Werror -std=c++98 -I../configparser -o test_config_parser test_config_parser.cpp ../configparser/configparser.cpp ../configparser/configdisplay.cpp; then
    echo "❌ Failed to compile config parser tests"
    exit 1
fi

# 运行测试
echo -e "\n🚀 Running initialize tests..."
./test_initialize
INIT_RESULT=$?

echo -e "\n🚀 Running config parser tests..."
./test_config_parser
PARSER_RESULT=$?

# 清理
echo -e "\n🧹 Cleaning up..."
make clean
rm -f test_config_parser *.conf

# 报告结果
echo -e "\n📊 FINAL RESULTS:"
echo "=================="
if [ $INIT_RESULT -eq 0 ]; then
    echo "✅ Initialize tests: PASSED"
else
    echo "❌ Initialize tests: FAILED"
fi

if [ $PARSER_RESULT -eq 0 ]; then
    echo "✅ Config parser tests: PASSED"
else
    echo "❌ Config parser tests: FAILED"
fi

if [ $INIT_RESULT -eq 0 ] && [ $PARSER_RESULT -eq 0 ]; then
    echo -e "\n🎉 All tests passed!"
    exit 0
else
    echo -e "\n💥 Some tests failed!"
    exit 1
fi