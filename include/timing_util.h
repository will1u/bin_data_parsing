#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>      // for std::localtime, std::strftime
#include <iomanip>    // for std::put_time (C++11 onward)


namespace TimingUtil {
    std::string getTime() {
        // 1. Get a time point representing "now"
        auto now = std::chrono::system_clock::now();  

        // 2. Convert to a time_t (seconds since epoch)
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        // 3. Convert to local time (not thread-safe)
        //    Use localtime_r(localTime, &now_c) on POSIX for thread safety, or localtime_s on Windows
        std::tm* localTime = std::localtime(&now_c);

        // 4. Write into an ostringstream
        std::ostringstream oss;
        oss << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S");

        // 5. Return the resulting string
        return oss.str();
    }
}
