#include "PortScanner.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

PortScanner::PortScanner(std::string target, int startPort, int endPort, int timeoutMs, int threadCount)
    : m_targetHost(std::move(target)),
      m_startPort(startPort),
      m_endPort(endPort),
      m_timeoutMs(timeoutMs),
      m_threadCount(threadCount),
      m_totalPorts(endPort - startPort + 1) {
    for (int p = m_startPort; p <= m_endPort; ++p) {
        m_portQueue.push(p);
    }
}

bool PortScanner::resolveHost() {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(m_targetHost.c_str(), nullptr, &hints, &res);
    if (status != 0 || res == nullptr) {
        std::cerr << "[-] Error resolving target: " << gai_strerror(status) << "\n";
        return false;
    }

    char ipStr[INET_ADDRSTRLEN];
    auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
    m_targetIp = std::string(ipStr);

    freeaddrinfo(res);
    return true;
}

std::string PortScanner::identifyCommonService(int port) const {
    switch (port) {
        case 20:
        case 21: return "FTP";
        case 22: return "SSH";
        case 23: return "Telnet";
        case 25: return "SMTP";
        case 53: return "DNS";
        case 80: return "HTTP";
        case 110: return "POP3";
        case 139: return "NetBIOS";
        case 143: return "IMAP";
        case 443: return "HTTPS";
        case 445: return "SMB";
        case 3306: return "MySQL";
        case 3389: return "RDP";
        case 5432: return "PostgreSQL";
        case 8080: return "HTTP-Proxy";
        case 8443: return "HTTPS-Alt";
        default: return "Unknown";
    }
}

bool PortScanner::checkPort(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sock);
        return false;
    }

    sockaddr_in targetAddr{};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(port);
    inet_pton(AF_INET, m_targetIp.c_str(), &targetAddr.sin_addr);

    int res = connect(sock, reinterpret_cast<struct sockaddr*>(&targetAddr), sizeof(targetAddr));
    if (res < 0) {
        if (errno != EINPROGRESS) {
            close(sock);
            return false;
        }

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        struct timeval tv{};
        tv.tv_sec = m_timeoutMs / 1000;
        tv.tv_usec = (m_timeoutMs % 1000) * 1000;

        int selRes = select(sock + 1, nullptr, &writeSet, nullptr, &tv);
        if (selRes > 0) {
            int sockError = 0;
            socklen_t len = sizeof(sockError);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockError, &len) < 0 || sockError != 0) {
                close(sock);
                return false;
            }
            close(sock);
            return true;
        } else {
            close(sock);
            return false;
        }
    }

    close(sock);
    return true;
}

void PortScanner::worker() {
    while (true) {
        int currentPort = -1;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_portQueue.empty()) {
                return;
            }
            currentPort = m_portQueue.front();
            m_portQueue.pop();
        }

        bool open = checkPort(currentPort);
        if (open) {
            std::lock_guard<std::mutex> lock(m_resultsMutex);
            m_results.push_back({currentPort, true, identifyCommonService(currentPort)});
        }

        m_scannedCount++;
    }
}

void PortScanner::run() {
    if (!resolveHost()) {
        std::cerr << "[-] Scan aborted: Host resolution failed.\n";
        return;
    }

    std::cout << "[+] Target: " << m_targetHost << " (" << m_targetIp << ")\n";
    std::cout << "[+] Port Range: " << m_startPort << " - " << m_endPort << "\n";
    std::cout << "[+] Threads: " << m_threadCount << " | Timeout: " << m_timeoutMs << " ms\n";
    std::cout << "[*] Starting scan...\n";

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> pool;
    pool.reserve(m_threadCount);

    for (int i = 0; i < m_threadCount; ++i) {
        pool.emplace_back(&PortScanner::worker, this);
    }

    for (auto& t : pool) {
        if (t.joinable()) {
            t.join();
        }
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    std::cout << "[*] Scan completed in " << std::fixed << std::setprecision(2) 
              << elapsed.count() << " seconds.\n\n";
}

void PortScanner::printResults() const {
    std::vector<ScanResult> sortedResults = m_results;
    std::sort(sortedResults.begin(), sortedResults.end(), [](const ScanResult& a, const ScanResult& b) {
        return a.port < b.port;
    });

    std::cout << "========================================\n";
    std::cout << " PORT     STATE   SERVICE               \n";
    std::cout << "========================================\n";

    if (sortedResults.empty()) {
        std::cout << " No open ports found.\n";
    } else {
        for (const auto& res : sortedResults) {
            std::cout << " " << std::left << std::setw(8) << (std::to_string(res.port) + "/tcp")
                      << " " << std::setw(7) << "open"
                      << " " << res.serviceName << "\n";
        }
    }
    std::cout << "========================================\n";
}
