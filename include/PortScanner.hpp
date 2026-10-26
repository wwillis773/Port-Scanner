#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>

struct ScanResult {
    int port;
    bool isOpen;
    std::string serviceName;
};

class PortScanner {
public:
    PortScanner(std::string target, int startPort, int endPort, int timeoutMs, int threadCount);
    ~PortScanner() = default;

    void run();
    void printResults() const;

private:
    std::string m_targetHost;
    std::string m_targetIp;
    int m_startPort;
    int m_endPort;
    int m_timeoutMs;
    int m_threadCount;

    std::queue<int> m_portQueue;
    std::mutex m_queueMutex;

    std::vector<ScanResult> m_results;
    std::mutex m_resultsMutex;

    std::atomic<int> m_scannedCount{0};
    int m_totalPorts{0};

    bool resolveHost();
    void worker();
    bool checkPort(int port);
    std::string identifyCommonService(int port) const;
};
