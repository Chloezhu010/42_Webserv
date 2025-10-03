#include <iostream>
#include <csignal>
#include "../configparser/initialize.hpp"

// Global server pointer for signal handling
WebServer* g_server = NULL;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << std::endl;
    if (g_server && g_server->isRunning()) {
        std::cout << "Gracefully shutting down server..." << std::endl;
        g_server->stop();
    }
    // Don't call exit(), let program exit naturally to trigger destructors
}

void setupSignalHandlers() {
    // Handle common termination signals
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination request
    signal(SIGQUIT, signalHandler);  // Quit signal

    // Ignore SIGPIPE (broken pipe), handle in code
    signal(SIGPIPE, SIG_IGN);
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <config_file>" << std::endl;
    std::cout << "Example: " << programName << " config/webserv.conf" << std::endl;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        std::cerr << "❌ Error: Invalid number of arguments" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Setup signal handlers for graceful shutdown
    setupSignalHandlers();

    try {
        // Create WebServer instance
        WebServer server;
        g_server = &server; // Set global pointer for signal handler

        std::cout << "🚀 Starting WebServer..." << std::endl;
        std::cout << "📁 Config file: " << argv[1] << std::endl;

        // Initialize server with config file
        if (!server.initialize(argv[1])) {
            std::cerr << "❌ Failed to initialize server with config file: "
                      << argv[1] << std::endl;
            std::cerr << "Error details: " << server.getLastError() << std::endl;
            return 1;
        }

        std::cout << "✅ Server initialized successfully" << std::endl;

        // Start server
        if (!server.start()) {
            std::cerr << "❌ Failed to start server" << std::endl;
            std::cerr << "Error details: " << server.getLastError() << std::endl;
            return 1;
        }

        std::cout << "🌟 WebServer is running!" << std::endl;
        std::cout << "Press Ctrl+C to stop server" << std::endl;

        // Keep server running
        // Note: You need to implement the actual event loop in WebServer class
        // This would be a run() method that handles client connections
        while (server.isRunning()) {
            // This is where your main event loop would be
            // Now we just run to prevent busy waiting
            server.run();

            // You may want to add run() or handleEvents() method to WebServer
            // Using select/epoll to handle client connections
        }

    } catch (const std::exception& e) {
        std::cerr << "❌ Caught exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown error occurred" << std::endl;
        return 1;
    }

    std::cout << "👋 Server shutdown complete" << std::endl;
    return 0;
}