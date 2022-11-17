#pragma once
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>


#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION LogDuration UNIQUE_VAR_NAME_PROFILE

using namespace std;

class LogDuration
{
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;
    
    LogDuration() = default;
    
    explicit LogDuration(const std::string& operation_name, ostream& os = cerr) : operation_name_(operation_name), os_(os) {}
    
    ~LogDuration()
    {
        using namespace std::chrono;
        using namespace std::literals;
        
        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        if (!operation_name_.empty()) { std::cerr << operation_name_ << ": "s; }
        os_ << "Operation time: " << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    const std::string operation_name_;
    ostream& os_  = cerr;
};
