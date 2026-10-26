#include "PortScanner.hpp"
#include <iostream>
#include <string>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <host> [start_port] [end_port] [threads] [timeout_ms]\n"
              << "Default values:\n"
              << "  start_port: 1\n"
              << "  end_port  : 1024\n"
              << "  threads   : 50\n"
              << "  timeout_ms: 500\n\n"
              << "Examples:\n"
              << "  " << progName << " 127.0.0.1 1 1024 100 200\n"
              << "  " << progName << " scanme.nmap.org 20 100 50 400\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string target = argv[1];
    int startPort = (argc >= 3) ? std::stoi(argv[2]) : 1;
    int endPort   = (argc >= 4) ? std::stoi(argv[3]) : 1024;
    int threads   = (argc >= 5) ? std::stoi(argv[4]) : 50;
    int timeoutMs = (argc >= 6) ? std::stoi(argv[5]) : 500;

    if (startPort < 1 || endPort > 65535 || startPort > endPort) {
        std::cerr << "[-] Error: Invalid port range (1 - 65535).\n";
        return 1;
    }

    if (threads < 1 || threads > 1000) {
        std::cerr << "[-] Error: Thread count must be between 1 and 1000.\n";
        return 1;
    }

    PortScanner scanner(target, startPort, endPort, timeoutMs, threads);
    scanner.run();
    scanner.printResults();

    return 0;
}
