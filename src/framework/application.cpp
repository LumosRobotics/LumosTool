#include "application.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace Lumos
{

    // Constructor
    ApplicationBase::ApplicationBase()
        : state_(ApplicationState::CREATED), metadata_(), stats_(), last_error_("")
    {
    }

    ApplicationBase::ApplicationBase(const std::string &name, const std::string &version)
        : state_(ApplicationState::CREATED), metadata_(), stats_(), last_error_("")
    {
        metadata_.name = name;
        metadata_.version = version;
    }

    ApplicationBase::~ApplicationBase()
    {
        // Ensure cleanup happens if not already done
        if (state_ == ApplicationState::INITIALIZED || state_ == ApplicationState::RUNNING)
        {
            LogWarning("Application destroyed without proper shutdown, calling DeInit()");
            Shutdown();
        }
    }

    // Framework lifecycle management
    void ApplicationBase::Initialize()
    {
        if (state_ != ApplicationState::CREATED && state_ != ApplicationState::STOPPED)
        {
            LogWarning("Initialize() called in invalid state, ignoring");
            return;
        }

        try
        {
            LogInfo("Initializing application: " + metadata_.name);

            state_ = ApplicationState::INITIALIZED;

            // Call user implementation
            Init();

            stats_.init_count++;

            LogInfo("Application initialized successfully");
        }
        catch (const std::exception &e)
        {
            SetError(std::string("Exception during Init(): ") + e.what());
            LogError("Initialization failed: " + last_error_);
        }
        catch (...)
        {
            SetError("Unknown exception during Init()");
            LogError("Initialization failed: " + last_error_);
        }
    }

    void ApplicationBase::Execute()
    {
        if (state_ != ApplicationState::INITIALIZED && state_ != ApplicationState::RUNNING)
        {
            if (!HasError())
            {
                LogError("Execute() called before Initialize() or after Shutdown()");
            }
            return;
        }

        if (state_ == ApplicationState::INITIALIZED)
        {
            state_ = ApplicationState::RUNNING;
        }

        try
        {
            // Measure execution time
            uint64_t start_time = GetCurrentTimeUs();

            // Call user implementation
            Step();

            uint64_t end_time = GetCurrentTimeUs();
            uint64_t step_time = end_time - start_time;

            // Update statistics
            stats_.step_count++;
            UpdateStepTiming(step_time);
        }
        catch (const std::exception &e)
        {
            SetError(std::string("Exception during Step(): ") + e.what());
            LogError("Step execution failed: " + last_error_);
        }
        catch (...)
        {
            SetError("Unknown exception during Step()");
            LogError("Step execution failed: " + last_error_);
        }
    }

    void ApplicationBase::Shutdown()
    {
        if (state_ == ApplicationState::STOPPED)
        {
            LogWarning("Shutdown() called on already stopped application");
            return;
        }

        try
        {
            LogInfo("Shutting down application: " + metadata_.name);

            // Call user implementation
            DeInit();

            state_ = ApplicationState::STOPPED;
            stats_.deinit_count++;

            LogInfo("Application shut down successfully");

            // Print statistics if any steps were executed
            if (stats_.step_count > 0)
            {
                LogInfo("Application statistics:");
                LogInfo("  Total steps: " + std::to_string(stats_.step_count));
                LogInfo("  Average step time: " + std::to_string(stats_.GetAverageStepTimeUs()) + " us");
                LogInfo("  Min step time: " + std::to_string(stats_.min_step_time_us) + " us");
                LogInfo("  Max step time: " + std::to_string(stats_.max_step_time_us) + " us");
            }
        }
        catch (const std::exception &e)
        {
            SetError(std::string("Exception during DeInit(): ") + e.what());
            LogError("Shutdown failed: " + last_error_);
        }
        catch (...)
        {
            SetError("Unknown exception during DeInit()");
            LogError("Shutdown failed: " + last_error_);
        }
    }

    // Error handling
    void ApplicationBase::SetError(const std::string &error_msg)
    {
        last_error_ = error_msg;
        state_ = ApplicationState::ERROR;
        stats_.error_count++;
    }

    void ApplicationBase::ClearError()
    {
        last_error_.clear();
        if (state_ == ApplicationState::ERROR)
        {
            state_ = ApplicationState::STOPPED;
        }
    }

    // Logging helpers
    void ApplicationBase::LogInfo(const std::string &message)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::cout << "[" << std::put_time(std::localtime(&time), "%H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << metadata_.name << "] [INFO] " << message << std::endl;
    }

    void ApplicationBase::LogWarning(const std::string &message)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::cout << "[" << std::put_time(std::localtime(&time), "%H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << metadata_.name << "] [WARN] " << message << std::endl;
    }

    void ApplicationBase::LogError(const std::string &message)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::cerr << "[" << std::put_time(std::localtime(&time), "%H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << metadata_.name << "] [ERROR] " << message << std::endl;
    }

    // Internal helpers
    void ApplicationBase::UpdateStepTiming(uint64_t step_time_us)
    {
        stats_.total_step_time_us += step_time_us;

        if (step_time_us > stats_.max_step_time_us)
        {
            stats_.max_step_time_us = step_time_us;
        }

        if (step_time_us < stats_.min_step_time_us)
        {
            stats_.min_step_time_us = step_time_us;
        }
    }

    uint64_t ApplicationBase::GetCurrentTimeUs() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

} // namespace Lumos