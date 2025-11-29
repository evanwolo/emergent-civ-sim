#include "utils/EventLog.h"
#include <sstream>
#include <iomanip>

void EventLog::init(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (log_file_.is_open()) {
        log_file_.close();
    }
    
    log_file_.open(filepath, std::ios::out | std::ios::trunc);
    if (!log_file_.is_open()) {
        throw std::runtime_error("Failed to open event log file: " + filepath);
    }
    
    // Write CSV header
    log_file_ << "tick,event_type,agent_id,region_id,magnitude,details\n";
    file_initialized_ = true;
}

void EventLog::logEvent(std::uint64_t tick, EventType type, std::uint32_t agent_id,
                        std::uint32_t region_id, const std::string& details, double magnitude) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    events_.emplace_back(tick, type, agent_id, region_id, details, magnitude);
    
    // Write to file immediately for real-time analysis
    if (file_initialized_) {
        log_file_ << tick << ","
                  << eventTypeToString(type) << ","
                  << agent_id << ","
                  << region_id << ","
                  << std::fixed << std::setprecision(4) << magnitude << ","
                  << "\"" << details << "\"\n";
    }
}

void EventLog::logBirth(std::uint64_t tick, std::uint32_t agent_id, std::uint32_t region_id,
                        std::uint32_t parent_id) {
    std::ostringstream details;
    details << "parent=" << parent_id;
    logEvent(tick, EventType::BIRTH, agent_id, region_id, details.str(), 1.0);
}

void EventLog::logDeath(std::uint64_t tick, std::uint32_t agent_id, std::uint32_t region_id, int age) {
    std::ostringstream details;
    details << "age=" << age;
    logEvent(tick, EventType::DEATH, agent_id, region_id, details.str(), 1.0);
}

void EventLog::logTrade(std::uint64_t tick, std::uint32_t from_region, std::uint32_t to_region,
                        int good_type, double volume, double price) {
    std::ostringstream details;
    details << "to=" << to_region << ";good=" << good_type 
            << ";volume=" << std::fixed << std::setprecision(2) << volume
            << ";price=" << std::fixed << std::setprecision(4) << price;
    logEvent(tick, EventType::TRADE, 0, from_region, details.str(), volume * price);
}

void EventLog::logMovementFormed(std::uint64_t tick, std::uint32_t movement_id,
                                 std::uint32_t region_id, std::size_t member_count) {
    std::ostringstream details;
    details << "movement_id=" << movement_id << ";members=" << member_count;
    logEvent(tick, EventType::MOVEMENT_FORMED, 0, region_id, details.str(), 
             static_cast<double>(member_count));
}

void EventLog::logSystemChange(std::uint64_t tick, std::uint32_t region_id,
                               const std::string& old_system, const std::string& new_system) {
    std::ostringstream details;
    details << "from=" << old_system << ";to=" << new_system;
    logEvent(tick, EventType::SYSTEM_CHANGE, 0, region_id, details.str(), 1.0);
}

void EventLog::logMigration(std::uint64_t tick, std::uint32_t agent_id,
                           std::uint32_t from_region, std::uint32_t to_region) {
    std::ostringstream details;
    details << "from=" << from_region << ";to=" << to_region;
    logEvent(tick, EventType::MIGRATION, agent_id, to_region, details.str(), 1.0);
}

void EventLog::logHardshipCrisis(std::uint64_t tick, std::uint32_t region_id, double hardship_level) {
    std::ostringstream details;
    details << "hardship=" << std::fixed << std::setprecision(3) << hardship_level;
    logEvent(tick, EventType::HARDSHIP_CRISIS, 0, region_id, details.str(), hardship_level);
}

void EventLog::logDevelopmentMilestone(std::uint64_t tick, std::uint32_t region_id, double development_level) {
    std::ostringstream details;
    details << "development=" << std::fixed << std::setprecision(2) << development_level;
    logEvent(tick, EventType::DEVELOPMENT_MILESTONE, 0, region_id, details.str(), development_level);
}

void EventLog::exportCSV(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open export file: " + filepath);
    }
    
    // Write header
    out << "tick,event_type,agent_id,region_id,magnitude,details\n";
    
    // Write events
    for (const auto& event : events_) {
        out << event.tick << ","
            << eventTypeToString(event.type) << ","
            << event.agent_id << ","
            << event.region_id << ","
            << std::fixed << std::setprecision(4) << event.magnitude << ","
            << "\"" << event.details << "\"\n";
    }
}

void EventLog::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.flush();
    }
}

void EventLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
}

std::vector<Event> EventLog::getEventsByType(EventType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Event> result;
    
    for (const auto& event : events_) {
        if (event.type == type) {
            result.push_back(event);
        }
    }
    
    return result;
}

std::vector<Event> EventLog::getEventsByTickRange(std::uint64_t start_tick, std::uint64_t end_tick) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Event> result;
    
    for (const auto& event : events_) {
        if (event.tick >= start_tick && event.tick <= end_tick) {
            result.push_back(event);
        }
    }
    
    return result;
}

std::string EventLog::eventTypeToString(EventType type) const {
    switch (type) {
        case EventType::BIRTH: return "BIRTH";
        case EventType::DEATH: return "DEATH";
        case EventType::TRADE: return "TRADE";
        case EventType::MOVEMENT_FORMED: return "MOVEMENT_FORMED";
        case EventType::MOVEMENT_DISBANDED: return "MOVEMENT_DISBANDED";
        case EventType::IDEOLOGY_SHIFT: return "IDEOLOGY_SHIFT";
        case EventType::ECONOMIC_CRISIS: return "ECONOMIC_CRISIS";
        case EventType::SYSTEM_CHANGE: return "SYSTEM_CHANGE";
        case EventType::MIGRATION: return "MIGRATION";
        case EventType::CULTURAL_CLUSTER_SPLIT: return "CULTURAL_CLUSTER_SPLIT";
        case EventType::HARDSHIP_CRISIS: return "HARDSHIP_CRISIS";
        case EventType::DEVELOPMENT_MILESTONE: return "DEVELOPMENT_MILESTONE";
        default: return "UNKNOWN";
    }
}
