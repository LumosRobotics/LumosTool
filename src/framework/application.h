#pragma once

#include <string>
#include <cstdint>

namespace Lumos
{

    enum class ApplicationState {
        CREATED,      // Application created but not initialized
        INITIALIZED,  // Init() called successfully
        RUNNING,      // Step() being called
        STOPPED,      // DeInit() called
        ERROR         // Error state
    };

    struct ApplicationMetadata {
        std::string name;
        std::string version;
        uint32_t rate_hz;        // Desired execution rate in Hz (0 = event-driven)
        uint8_t priority;        // Priority level (0-255, higher = more important)

        ApplicationMetadata()
            : name("UnnamedApp")
            , version("1.0.0")
            , rate_hz(10)
            , priority(128)
        {}
    };

    struct ApplicationStats {
        uint64_t init_count;           // Number of times Init() was called
        uint64_t step_count;           // Number of times Step() was called
        uint64_t deinit_count;         // Number of times DeInit() was called
        uint64_t error_count;          // Number of errors encountered
        uint64_t total_step_time_us;   // Total time spent in Step() (microseconds)
        uint64_t max_step_time_us;     // Maximum Step() execution time
        uint64_t min_step_time_us;     // Minimum Step() execution time

        ApplicationStats()
            : init_count(0)
            , step_count(0)
            , deinit_count(0)
            , error_count(0)
            , total_step_time_us(0)
            , max_step_time_us(0)
            , min_step_time_us(UINT64_MAX)
        {}

        double GetAverageStepTimeUs() const {
            return step_count > 0 ? static_cast<double>(total_step_time_us) / step_count : 0.0;
        }
    };

    class ApplicationBase
    {
    public:
        ApplicationBase();
        ApplicationBase(const std::string& name, const std::string& version = "1.0.0");
        virtual ~ApplicationBase();

        // Lifecycle methods - override these in your application
        virtual void Init() = 0;
        virtual void Step() = 0;
        virtual void DeInit() = 0;

        // Framework lifecycle management - DO NOT override these
        void Initialize();   // Called by framework to run Init()
        void Execute();      // Called by framework to run Step()
        void Shutdown();     // Called by framework to run DeInit()

        // Configuration
        void SetName(const std::string& name) { metadata_.name = name; }
        void SetVersion(const std::string& version) { metadata_.version = version; }
        void SetUpdateRate(uint32_t rate_hz) { metadata_.rate_hz = rate_hz; }
        void SetPriority(uint8_t priority) { metadata_.priority = priority; }

        // Getters
        const std::string& GetName() const { return metadata_.name; }
        const std::string& GetVersion() const { return metadata_.version; }
        uint32_t GetUpdateRate() const { return metadata_.rate_hz; }
        uint8_t GetPriority() const { return metadata_.priority; }
        ApplicationState GetState() const { return state_; }
        const ApplicationStats& GetStats() const { return stats_; }

        // State queries
        bool IsInitialized() const { return state_ == ApplicationState::INITIALIZED || state_ == ApplicationState::RUNNING; }
        bool IsRunning() const { return state_ == ApplicationState::RUNNING; }
        bool IsStopped() const { return state_ == ApplicationState::STOPPED; }
        bool HasError() const { return state_ == ApplicationState::ERROR; }

        // Error handling
        void SetError(const std::string& error_msg);
        const std::string& GetLastError() const { return last_error_; }
        void ClearError();

    protected:
        // Helper methods for derived classes
        void LogInfo(const std::string& message);
        void LogWarning(const std::string& message);
        void LogError(const std::string& message);

        // Access to metadata for derived classes
        ApplicationMetadata& GetMetadata() { return metadata_; }

    private:
        ApplicationState state_;
        ApplicationMetadata metadata_;
        ApplicationStats stats_;
        std::string last_error_;

        // Internal helpers
        void UpdateStepTiming(uint64_t step_time_us);
        uint64_t GetCurrentTimeUs() const;
    };

} // namespace Lumos