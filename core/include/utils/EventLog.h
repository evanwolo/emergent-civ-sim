#ifndef EVENTLOG_H
#define EVENTLOG_H

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <mutex>

// Event types for tracking simulation dynamics
enum class EventType {
    BIRTH,
    DEATH,
    TRADE,
    MOVEMENT_FORMED,
    MOVEMENT_DISBANDED,
    IDEOLOGY_SHIFT,
    ECONOMIC_CRISIS,
    SYSTEM_CHANGE
};

// Individual event record
struct Event {
    std::uint64_t tick;
    EventType type;
    std::uint32_t agent_id;      // Primary agent involved (0 for region-level events)
    std::uint32_t region_id;     // Region where event occurred
    std::string details;         // JSON or CSV formatted details
    double magnitude;            // Event intensity/size
    
    Event(std::uint64_t t, EventType et, std::uint32_t aid, std::uint32_t rid, 
          const std::string& det, double mag)
        : tick(t), type(et), agent_id(aid), region_id(rid), details(det), magnitude(mag) {}
};

// Event logging system for simulation analysis
class EventLog {
public:
    EventLog() = default;
    
    // Initialize with output file path
    void init(const std::string& filepath);
    
    // Log an event (thread-safe)
    void logEvent(std::uint64_t tick, EventType type, std::uint32_t agent_id, 
                  std::uint32_t region_id, const std::string& details, double magnitude);
    
    // Convenience methods for common events
    void logBirth(std::uint64_t tick, std::uint32_t agent_id, std::uint32_t region_id, 
                  std::uint32_t parent_id);
    void logDeath(std::uint64_t tick, std::uint32_t agent_id, std::uint32_t region_id, int age);
    void logTrade(std::uint64_t tick, std::uint32_t from_region, std::uint32_t to_region, 
                  int good_type, double volume, double price);
    void logMovementFormed(std::uint64_t tick, std::uint32_t movement_id, 
                          std::uint32_t region_id, std::size_t member_count);
    void logSystemChange(std::uint64_t tick, std::uint32_t region_id, 
                        const std::string& old_system, const std::string& new_system);
    
    // Export to CSV file
    void exportCSV(const std::string& filepath) const;
    
    // Flush pending writes
    void flush();
    
    // Clear event buffer
    void clear();
    
    // Get event count
    std::size_t size() const { return events_.size(); }
    
    // Get events by type
    std::vector<Event> getEventsByType(EventType type) const;
    
    // Get events in tick range
    std::vector<Event> getEventsByTickRange(std::uint64_t start_tick, std::uint64_t end_tick) const;
    
private:
    std::vector<Event> events_;
    std::ofstream log_file_;
    mutable std::mutex mutex_;  // Thread-safe logging (mutable allows locking in const methods)
    bool file_initialized_ = false;
    
    std::string eventTypeToString(EventType type) const;
};

#endif // EVENTLOG_H
