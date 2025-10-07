#pragma once
#include <string>
#include <cstdio>
#include <mutex>

namespace cvp {
class Logger {
public:
    template <typename... Args>
    static void info(const char* fmt, Args... args) {
        std::lock_guard<std::mutex> lk(mu_);
        fprintf(stdout, (std::string("[I] ") + fmt + "\n").c_str(), args...);
    }

    template <typename... Args>
    static void warn(const char* fmt, Args... args) {
        std::lock_guard<std::mutex> lk(mu_);
        fprintf(stdout, (std::string("[W] ") + fmt + "\n").c_str(), args...);
    }

    template <typename... Args>
    static void error(const char* fmt, Args... args) {
        std::lock_guard<std::mutex> lk(mu_);
        fprintf(stderr, (std::string("[E] ") + fmt + "\n").c_str(), args...);
    }

private:
    static inline std::mutex mu_{};
};
}