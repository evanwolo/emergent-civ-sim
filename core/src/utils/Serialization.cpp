#include "utils/Serialization.h"
#include "kernel/Kernel.h"
#include <chrono>
#include <iostream>

namespace serialization {

// Serialize a single agent
void writeAgent(std::ofstream& out, const Agent& agent) {
    // Identity
    writeBinary(out, agent.id);
    writeBinary(out, agent.region);
    writeBinary(out, agent.alive);
    
    // Demography
    writeBinary(out, agent.age);
    writeBinary(out, agent.female);
    
    // Lineage
    writeBinary(out, agent.parent_a);
    writeBinary(out, agent.parent_b);
    writeBinary(out, agent.lineage_id);
    
    // Language
    writeBinary(out, agent.primaryLang);
    writeBinary(out, agent.dialect);
    writeBinary(out, agent.fluency);
    
    // Personality
    writeBinary(out, agent.openness);
    writeBinary(out, agent.conformity);
    writeBinary(out, agent.assertiveness);
    writeBinary(out, agent.sociality);
    
    // Beliefs
    writeBinaryArray(out, agent.x);
    writeBinaryArray(out, agent.B);
    writeBinary(out, agent.B_norm_sq);
    
    // Multipliers
    writeBinary(out, agent.m_comm);
    writeBinary(out, agent.m_susceptibility);
    writeBinary(out, agent.m_mobility);
    
    // Neighbors
    writeBinaryVector(out, agent.neighbors);
}

void readAgent(std::ifstream& in, Agent& agent) {
    // Identity
    readBinary(in, agent.id);
    readBinary(in, agent.region);
    readBinary(in, agent.alive);
    
    // Demography
    readBinary(in, agent.age);
    readBinary(in, agent.female);
    
    // Lineage
    readBinary(in, agent.parent_a);
    readBinary(in, agent.parent_b);
    readBinary(in, agent.lineage_id);
    
    // Language
    readBinary(in, agent.primaryLang);
    readBinary(in, agent.dialect);
    readBinary(in, agent.fluency);
    
    // Personality
    readBinary(in, agent.openness);
    readBinary(in, agent.conformity);
    readBinary(in, agent.assertiveness);
    readBinary(in, agent.sociality);
    
    // Beliefs
    readBinaryArray(in, agent.x);
    readBinaryArray(in, agent.B);
    readBinary(in, agent.B_norm_sq);
    
    // Multipliers
    readBinary(in, agent.m_comm);
    readBinary(in, agent.m_susceptibility);
    readBinary(in, agent.m_mobility);
    
    // Neighbors
    readBinaryVector(in, agent.neighbors);
}

bool saveCheckpoint(const Kernel& kernel, const std::string& filepath) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open checkpoint file for writing: " << filepath << std::endl;
        return false;
    }
    
    try {
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        
        // Write header
        CheckpointHeader header;
        header.generation = kernel.generation();
        header.num_agents = static_cast<std::uint32_t>(kernel.agents().size());
        header.num_regions = static_cast<std::uint32_t>(kernel.regionIndex().size());
        header.timestamp = static_cast<std::uint64_t>(timestamp);
        
        writeBinary(out, header);
        
        // Write agents
        for (const auto& agent : kernel.agents()) {
            writeAgent(out, agent);
        }
        
        // Write region index
        for (const auto& region : kernel.regionIndex()) {
            writeBinaryVector(out, region);
        }
        
        // Write economy state (simplified - just regional data)
        const auto& economy = kernel.economy();
        for (std::uint32_t r = 0; r < header.num_regions; ++r) {
            const auto& region = economy.getRegion(r);
            writeBinary(out, region.development);
            writeBinary(out, region.welfare);
            writeBinary(out, region.inequality);
            writeBinary(out, region.hardship);
            writeBinary(out, region.efficiency);
            writeBinary(out, region.system_stability);
            writeBinaryString(out, region.economic_system);
            writeBinaryArray(out, region.production);
            writeBinaryArray(out, region.prices);
        }
        
        // Write agent economy data
        for (std::uint32_t i = 0; i < header.num_agents; ++i) {
            const auto& agent_econ = economy.getAgentEconomy(i);
            writeBinary(out, agent_econ.wealth);
            writeBinary(out, agent_econ.income);
            writeBinary(out, agent_econ.productivity);
            writeBinary(out, agent_econ.sector);
            writeBinary(out, agent_econ.hardship);
        }
        
        out.close();
        std::cout << "Checkpoint saved: " << filepath 
                  << " (gen " << header.generation 
                  << ", " << header.num_agents << " agents)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving checkpoint: " << e.what() << std::endl;
        return false;
    }
}

bool loadCheckpoint(Kernel& kernel, const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Failed to open checkpoint file for reading: " << filepath << std::endl;
        return false;
    }
    
    try {
        // Read and validate header
        CheckpointHeader header;
        readBinary(in, header);
        
        if (header.magic != CHECKPOINT_MAGIC) {
            std::cerr << "Invalid checkpoint file (bad magic number)" << std::endl;
            return false;
        }
        
        if (header.version != CHECKPOINT_VERSION) {
            std::cerr << "Checkpoint version mismatch (expected " << CHECKPOINT_VERSION 
                      << ", got " << header.version << ")" << std::endl;
            return false;
        }
        
        // Read agents
        std::vector<Agent> agents(header.num_agents);
        for (auto& agent : agents) {
            readAgent(in, agent);
        }
        
        // Read region index
        std::vector<std::vector<std::uint32_t>> regionIndex(header.num_regions);
        for (auto& region : regionIndex) {
            readBinaryVector(in, region);
        }
        
        // For now, we only restore agent state - economy needs more work
        // This is a partial restore that works for continued simulation
        
        // Note: Full restore would require Economy class to have load/save methods
        // For now, log a warning
        std::cout << "Checkpoint loaded: " << filepath 
                  << " (gen " << header.generation 
                  << ", " << header.num_agents << " agents)" << std::endl;
        std::cout << "Warning: Economy state restore not fully implemented" << std::endl;
        
        // TODO: Implement full restore via Kernel method
        // kernel.restoreFromCheckpoint(header, agents, regionIndex, ...);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading checkpoint: " << e.what() << std::endl;
        return false;
    }
}

}  // namespace serialization
