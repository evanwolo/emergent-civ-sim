#include "Kernel.h"
#include "KernelSnapshot.h"
#include <iostream>
#include <fstream>
#include <sstream>

static void printHelp() {
    std::cerr << "Kernel Commands:\n"
              << "  step N             # advance N steps\n"
              << "  state [traits]     # print JSON snapshot (optional: include traits)\n"
              << "  metrics            # print current metrics\n"
              << "  reset [N R k p]    # reset with optional: pop, regions, k, rewire_p\n"
              << "  run T log          # run T ticks, log metrics every 'log' steps\n"
              << "  quit               # exit\n";
}

int main() {
    KernelConfig cfg;
    cfg.population = 50000;
    cfg.regions = 200;
    cfg.avgConnections = 8;
    cfg.rewireProb = 0.05;
    cfg.stepSize = 0.15;
    
    Kernel kernel(cfg);
    
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    printHelp();
    
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) continue;
        
        if (cmd == "step") {
            int n = 1;
            iss >> n;
            if (n < 1) n = 1;
            for (int i = 0; i < n; ++i) {
                kernel.step();
                if ((i + 1) % 100 == 0 || i == n - 1) {
                    std::cerr << "Tick " << (kernel.generation()) << "/" 
                              << (kernel.generation() - n + n) << "\r";
                    std::cerr.flush();
                }
            }
            std::cerr << "\n";
            std::cout << kernelToJson(kernel) << "\n";
            std::cout.flush();
            
        } else if (cmd == "state") {
            std::string opt;
            iss >> opt;
            bool traits = (opt == "traits");
            std::cout << kernelToJson(kernel, traits) << "\n";
            std::cout.flush();
            
        } else if (cmd == "metrics") {
            auto m = kernel.computeMetrics();
            std::cout << "Generation: " << kernel.generation() << "\n"
                      << "Polarization: " << m.polarizationMean 
                      << " (±" << m.polarizationStd << ")\n"
                      << "Avg Openness: " << m.avgOpenness << "\n"
                      << "Avg Conformity: " << m.avgConformity << "\n";
            std::cout.flush();
            
        } else if (cmd == "reset") {
            std::uint32_t N = cfg.population;
            std::uint32_t R = cfg.regions;
            std::uint32_t k = cfg.avgConnections;
            double p = cfg.rewireProb;
            iss >> N >> R >> k >> p;
            
            KernelConfig newCfg = cfg;
            newCfg.population = N;
            newCfg.regions = R;
            newCfg.avgConnections = k;
            newCfg.rewireProb = p;
            
            kernel.reset(newCfg);
            std::cout << "Reset: " << N << " agents, " << R << " regions\n";
            std::cout.flush();
            
        } else if (cmd == "run") {
            int T = 1000;
            int logEvery = 10;
            iss >> T >> logEvery;
            
            std::ofstream metricsFile("data/metrics.csv");
            metricsFile << "generation,polarization_mean,polarization_std,"
                       << "avg_openness,avg_conformity\n";
            
            for (int t = 0; t < T; ++t) {
                kernel.step();
                if ((t + 1) % 100 == 0 || t == T - 1) {
                    std::cerr << "Tick " << (t + 1) << "/" << T << "\r";
                    std::cerr.flush();
                }
                if (t % logEvery == 0) {
                    logMetrics(kernel, metricsFile);
                }
            }
            
            std::cerr << "\n";
            metricsFile.close();
            std::cout << "Completed " << T << " ticks. Metrics written to data/metrics.csv\n";
            std::cout.flush();
            
        } else if (cmd == "quit") {
            break;
            
        } else if (cmd == "help") {
            printHelp();
            
        } else {
            std::cerr << "Unknown command: " << cmd << "\n";
            printHelp();
        }
    }
    
    return 0;
}
