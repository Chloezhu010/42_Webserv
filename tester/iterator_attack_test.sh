#!/bin/bash

# Iterator Attack Test - Targets the next_it vulnerability in WebServer::run()
# This test attempts to trigger iterator invalidation by forcing connection deletions
# during the event loop while other connections are being processed.

echo "🔥 ITERATOR ATTACK TEST - Targeting next_it vulnerability"
echo "=========================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configuration
SERVER_PORT=8080
TEST_ITERATIONS=50
CONCURRENT_CONNECTIONS=10

echo -e "${YELLOW}Target: WebServer::run() iterator vulnerability${NC}"
echo -e "${YELLOW}Method: Rapid connection creation/termination during event processing${NC}"
echo -e "${YELLOW}Expected: Server crash or hang due to iterator corruption${NC}"
echo ""

# Check if server is running
check_server() {
    nc -z localhost $SERVER_PORT 2>/dev/null
    return $?
}

# Attack 1: Rapid Connect-Disconnect Storm
attack_rapid_disconnect() {
    echo -e "${RED}[ATTACK 1] Rapid Connect-Disconnect Storm${NC}"
    echo "Creating $CONCURRENT_CONNECTIONS simultaneous connections..."

    for i in $(seq 1 $CONCURRENT_CONNECTIONS); do
        {
            # Connect and immediately disconnect to force closeClientConnection()
            timeout 0.1 nc localhost $SERVER_PORT < /dev/null
            echo "Connection $i: rapid disconnect"
        } &
    done

    # Wait for all background jobs
    wait
    echo "Attack 1 completed - checking if server survived..."
    sleep 1
}

# Attack 2: Mixed Valid/Invalid Requests
attack_mixed_requests() {
    echo -e "${RED}[ATTACK 2] Mixed Valid/Invalid Request Storm${NC}"
    echo "Sending mixed valid/invalid requests to trigger different code paths..."

    for i in $(seq 1 $CONCURRENT_CONNECTIONS); do
        {
            if [ $((i % 3)) -eq 0 ]; then
                # Valid request - will go through normal processing
                echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost $SERVER_PORT -w 1
            elif [ $((i % 3)) -eq 1 ]; then
                # Invalid request - triggers error handling
                echo -e "INVALID_METHOD\r\n\r\n" | nc localhost $SERVER_PORT -w 1
            else
                # Partial request then disconnect - triggers recv() error
                echo -n "GET" | nc localhost $SERVER_PORT -w 1
            fi
            echo "Request $i: mixed attack sent"
        } &
    done

    wait
    echo "Attack 2 completed - checking if server survived..."
    sleep 1
}

# Attack 3: Iterator Race Condition
attack_iterator_race() {
    echo -e "${RED}[ATTACK 3] Iterator Race Condition${NC}"
    echo "Attempting to corrupt next_it during event loop processing..."

    # Create connections that will be processed in sequence
    for i in $(seq 1 $CONCURRENT_CONNECTIONS); do
        {
            # Send partial request, wait, then disconnect
            # This should trigger deletion during handleClientRequest()
            (
                echo -n "GET /"
                sleep 0.05  # Small delay to ensure it's in the event loop
                # Connection dies here - should trigger closeClientConnection()
            ) | nc localhost $SERVER_PORT -w 1 &
        } &
    done

    wait
    echo "Attack 3 completed - checking if server survived..."
    sleep 2
}

# Attack 4: Write Buffer Overflow Attack
attack_write_overflow() {
    echo -e "${RED}[ATTACK 4] Write Buffer Overflow Attack${NC}"
    echo "Attempting to trigger send() failure during handleClientResponse()..."

    for i in $(seq 1 $CONCURRENT_CONNECTIONS); do
        {
            # Send valid request but close connection before response
            # This should cause send() to fail in handleClientResponse()
            echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost $SERVER_PORT -w 1 &
        } &
    done

    wait
    echo "Attack 4 completed - checking if server survived..."
    sleep 1
}

# Main attack sequence
main() {
    echo "Starting iterator attack test sequence..."
    echo "Make sure the webserv server is running on port $SERVER_PORT"
    echo ""

    # Check if server is accessible
    if ! check_server; then
        echo -e "${RED}ERROR: Server not accessible on localhost:$SERVER_PORT${NC}"
        echo "Please start the server with: ./webserv"
        exit 1
    fi

    echo -e "${GREEN}Server detected on port $SERVER_PORT${NC}"
    echo ""

    # Run attacks in sequence
    for iteration in $(seq 1 $TEST_ITERATIONS); do
        echo -e "${YELLOW}=== ITERATION $iteration/$TEST_ITERATIONS ===${NC}"

        attack_rapid_disconnect
        if ! check_server; then
            echo -e "${RED}💥 SERVER CRASHED during rapid disconnect attack!${NC}"
            exit 1
        fi

        attack_mixed_requests
        if ! check_server; then
            echo -e "${RED}💥 SERVER CRASHED during mixed request attack!${NC}"
            exit 1
        fi

        attack_iterator_race
        if ! check_server; then
            echo -e "${RED}💥 SERVER CRASHED during iterator race attack!${NC}"
            exit 1
        fi

        attack_write_overflow
        if ! check_server; then
            echo -e "${RED}💥 SERVER CRASHED during write overflow attack!${NC}"
            exit 1
        fi

        echo -e "${GREEN}Iteration $iteration completed - server survived${NC}"
        sleep 0.5
    done

    echo ""
    echo -e "${GREEN}🛡️  All attacks completed - server appears stable${NC}"
    echo "If the server survived all attacks, the iterator vulnerability may be fixed"
    echo "or the attack pattern didn't trigger the specific race condition."
}

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up background processes..."
    jobs -p | xargs kill 2>/dev/null
}

# Set up cleanup on exit
trap cleanup EXIT

# Run the attacks
main